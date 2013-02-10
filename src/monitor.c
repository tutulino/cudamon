#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "device.h"
#include "monitor.h"

static void init_device_info(struct monitor* mon)
{
  NVML_TRY(nvmlSystemGetDriverVersion(mon->driver_version,
                                      sizeof(mon->driver_version)));
  NVML_TRY(nvmlSystemGetNVMLVersion(mon->nvml_version,
                                    sizeof(mon->nvml_version)));

  NVML_TRY(nvmlDeviceGetCount(&mon->dev_count));

  mon->devices = malloc(mon->dev_count * sizeof(struct device));

  for(unsigned i = 0; i < mon->dev_count; ++i) {
    struct device dev;

    NVML_TRY(nvmlDeviceGetHandleByIndex(i, &dev.handle));

    NVML_TRY(nvmlDeviceGetName(dev.handle, dev.name, sizeof(dev.name)));
    NVML_TRY(nvmlDeviceGetSerial(dev.handle, dev.serial, sizeof(dev.serial)));

    NVML_TRY(nvmlDeviceGetPciInfo(dev.handle, &dev.pci));
    NVML_TRY(nvmlDeviceGetMemoryInfo(dev.handle, &dev.memory));

    unsigned long long event_types;
    NVML_TRY(nvmlEventSetCreate(&dev.event_set));
    if(0 == NVML_TRY(nvmlDeviceGetSupportedEventTypes(dev.handle, &event_types))) {
      NVML_TRY(nvmlDeviceRegisterEvents(dev.handle, event_types, dev.event_set));
    } else {
      dev.event_set = NULL;
    }

    mon->devices[i] = dev;
  }
}

static void update_device_info(struct monitor* mon)
{
  // NVML is thread safe, and the order we grab GPU information here
  // doesn't particularly matter, so might as well take advantage of
  // parallelism here.

  unsigned i;

  for(i = 0; i < mon->dev_count; ++i) {
    struct device* dev = &mon->devices[i];

    NVML_TRY(nvmlDeviceGetMemoryInfo(dev->handle, &dev->memory));
    NVML_TRY(nvmlDeviceGetTemperature(dev->handle, NVML_TEMPERATURE_GPU,
                                      &dev->temperature));
    NVML_TRY(nvmlDeviceGetPowerUsage(dev->handle, &dev->power_usage));

    if(dev->event_set != NULL) {
      nvmlEventData_t data;

      NVML_TRY(nvmlEventSetWait(dev->event_set, &data, 1));

      // TODO: Do something with the returned information.
    }
  }

}

struct monitor* monitor_new(void)
{
  struct monitor* mon = malloc(sizeof(struct monitor));

  // Half a second is a decent default
  mon->update_interval = 500;

  mon->active = 1;

  // Fetch the basic info that should hold true throughout execution
  init_device_info(mon);

  return mon;
}

void* monitor_thread(void* ptr)
{
  struct monitor* mon = (struct monitor*) ptr;

  while(mon->active) {
    update_device_info(mon);

    usleep(mon->update_interval * 1000);
  }

  return NULL;
}

void monitor_stop(void)
{

}

void monitor_destroy(struct monitor* mon)
{
  for(unsigned i = 0; i < mon->dev_count; ++i) {
    struct device* dev = &mon->devices[i];

    NVML_TRY(nvmlEventSetFree(dev->event_set));
  }

  free(mon->devices);
  free(mon);
}
