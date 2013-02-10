#pragma once

#ifndef _MONITOR_H
#define _MONITOR_H

struct device {
  nvmlDevice_t handle;

  nvmlPciInfo_t pci;
  nvmlComputeMode_t compute_mode;
  nvmlMemory_t memory;
  nvmlEventSet_t event_set;

  // In Celsius
  unsigned temperature;

  // In milliwatts
  unsigned power_usage;

  char name[NVML_DEVICE_NAME_BUFFER_SIZE];
  char serial[NVML_DEVICE_SERIAL_BUFFER_SIZE];
};

struct monitor {
  // How long should we wait before polling the devices again? In milliseconds.
  unsigned update_interval;
  // Whether or not the monitor should continue running
  int active;

  char driver_version[NVML_SYSTEM_DRIVER_VERSION_BUFFER_SIZE];
  char nvml_version[NVML_SYSTEM_NVML_VERSION_BUFFER_SIZE];

  unsigned dev_count;
  struct device* devices;
};

struct monitor* monitor_new(void);
void* monitor_thread(void* mon);
void monitor_stop(void);
void monitor_destroy(struct monitor*);

#endif /* _MONITOR_H */
