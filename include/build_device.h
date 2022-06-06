
#include "storage_devices.h"

void init_storage_device_list(storage_device_list_t *const device_list);
void init_storage_device(storage_device_t *const device);
int detect_storage_devices(storage_device_list_t *const device_list);
void free_storage_device_list(storage_device_list_t *const device_list);

int alloc_storage_device_list(storage_device_list_t *const device_list, size_t partition_list_size);

void replace_all_chars(char *const string, char delete, char add);
void init_storage_device_selected(storage_selected_t *const disk_selected);
void set_device_capacity_data(storage_device_t *const device);
void find_device_by_serial(storage_device_t *const device, gchar *const serial_no);

int detect_storage_device_type(storage_device_t * const device);

int detect_storage_device_capacity(storage_device_t * const device);

int detect_storage_nvme_short_name(storage_device_t * const device);
int detect_storage_serial_with_usb_adapter(storage_device_t * const device);

void free_partition(partition_t * const partition);
int get_device_capacity_bytes(char const * const device, unsigned long long int * const size_in_bytes, unsigned long long int * const total_sectors, unsigned long long int * const sector_size);
void set_device_state(storage_device_t *const device);

void set_dev_gb(storage_device_t *const device);
