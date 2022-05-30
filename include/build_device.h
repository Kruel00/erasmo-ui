
#include "storage_devices.h"

void init_storage_device_list(storage_device_list_t *const device_list);
void init_storage_device(storage_device_t *const device);
int detect_storage_devices(storage_device_list_t *const device_list);
void free_storage_device_list(storage_device_list_t *const device_list);
void free_storage_device(storage_device_t *const device);
int alloc_storage_device_list(storage_device_list_t *const device_list, size_t partition_list_size);
void free_partition(partition_t *const partition);
void replace_all_chars(char *const string, char delete, char add);

int sg_cmds_open_device(const char * device_name, int read_only, int verbose);