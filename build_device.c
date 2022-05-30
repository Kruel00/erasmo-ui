
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libudev.h>
#include <sys/mount.h>
#include <sys/statvfs.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "build_device.h"



#define bool _Bool
#define false 0
#define true 1

FILE * sg_warnings_strm = NULL;        /* would like to default to stderr */


void init_storage_device_list(storage_device_list_t *const device_list)
{
    device_list->device = NULL;
    device_list->count = 0U;
}

void init_storage_device(storage_device_t *const device)
{

    device->type[0] = '\0';
    device->brand[0] = '\0';
    device->model[0] = '\0';
    device->filename[0] = '\0';
    device->serial_number[0] = '\0';
    device->sectors = 0UL;
    device->block_size = 0U;
}

int detect_storage_devices(storage_device_list_t *const device_list)
{

    if (device_list == NULL)
        return EXIT_FAILURE;

    int ret_val = EXIT_FAILURE;

    free_storage_device_list(device_list);

    // detect storage devices list
    {
        struct udev *u_dev = udev_new();

        if (u_dev != NULL)
        {

            struct udev_enumerate *u_enumerate = udev_enumerate_new(u_dev);

            if (u_enumerate != NULL)
            {

                ret_val = EXIT_SUCCESS;

                udev_enumerate_add_match_subsystem(u_enumerate, "block");
                udev_enumerate_add_match_sysname(u_enumerate, "sd?");
                udev_enumerate_add_match_sysname(u_enumerate, "nvme?n?");
                udev_enumerate_add_match_sysname(u_enumerate, "mmcblk?");

                if (udev_enumerate_scan_devices(u_enumerate) >= 0) {

                    struct udev_list_entry *u_last_entry = udev_enumerate_get_list_entry(u_enumerate);

                    if (u_last_entry != NULL) {

                        struct udev_list_entry *u_entry = NULL;
                        size_t devices_count = 0U;
                        udev_list_entry_foreach(u_entry, u_last_entry)
                        {
                            ++devices_count;
                        }

                        if (alloc_storage_device_list(device_list, devices_count) == EXIT_SUCCESS)
                        {
                            device_list->count = 0U;
                            u_entry = NULL;

                            udev_list_entry_foreach(u_entry, u_last_entry)
                            {
                                const size_t i_dev = device_list->count;
                                strcpy(device_list->device[i_dev].sys_path, udev_list_entry_get_name(u_entry));

                                struct udev_device *u_device = udev_device_new_from_syspath(u_dev, device_list->device[i_dev].sys_path);

                                if (u_device != NULL) {
                                    struct udev_list_entry *u_entry = udev_device_get_properties_list_entry(u_device);
                                    while (u_entry != NULL) {

                                        if (strcmp(udev_list_entry_get_name(u_entry), "ID_SERIAL") == 0){
                                            if (device_list->device[i_dev].serial_number[0] == '\0'){
                                                strcpy(device_list->device[i_dev].serial_number, udev_list_entry_get_value(u_entry));
                                            }
                                        }else if (strcmp(udev_list_entry_get_name(u_entry), "ID_SERIAL_SHORT") == 0){
                                            strcpy(device_list->device[i_dev].serial_number, udev_list_entry_get_value(u_entry));
                            
                                        }else if (strcmp(udev_list_entry_get_name(u_entry), "ID_MODEL") == 0){
                                            strcpy(device_list->device[i_dev].model, udev_list_entry_get_value(u_entry));
                                            replace_all_chars(device_list->device[i_dev].model, '_', ' ');
                            
                                        }else if(strcmp(udev_list_entry_get_name(u_entry), "ID_VENDOR") == 0) {
											strcpy(device_list->device[i_dev].brand, udev_list_entry_get_value(u_entry));
											replace_all_chars(device_list->device[i_dev].brand, '_', ' ');
                            
										} else if(strcmp(udev_list_entry_get_name(u_entry), "ID_PART_TABLE_TYPE") == 0) {
											strcpy(device_list->device[i_dev].partition_table_type, udev_list_entry_get_value(u_entry));
                            
										}else if(strcmp(udev_list_entry_get_name(u_entry), "ID_PART_TABLE_UUID") == 0) {
											strcpy(device_list->device[i_dev].partition_table_uuid, udev_list_entry_get_value(u_entry));
                            
										} else if(strcmp(udev_list_entry_get_name(u_entry), "ID_BUS") == 0) {
											strcpy(device_list->device[i_dev].bus, udev_list_entry_get_value(u_entry));
                            
                                        } else if(strcmp(udev_list_entry_get_name(u_entry), "ID_USB_DRIVER") == 0) {
											strcpy(device_list->device[i_dev].usb_driver, udev_list_entry_get_value(u_entry));
                            
										} else if(strcmp(udev_list_entry_get_name(u_entry), "DEVNAME") == 0) {
											strcpy(device_list->device[i_dev].name, udev_list_entry_get_value(u_entry));
                            
                                            
										}
                                        u_entry = udev_list_entry_get_next(u_entry);
                                    }
                                    
                                    {
                                        struct udev_device *u_device_parent = u_device;
										while(u_device_parent != NULL) {
											const char *type = udev_device_get_sysattr_value(u_device_parent, "type");
											if((type != NULL) && (strlen(type) != 0U)) {
												strcpy(device_list->device[i_dev].type_attribute, type);
												break;
											}
											u_device_parent = udev_device_get_parent(u_device_parent);
										}
                                    }
                                    udev_device_unref(u_device);
									++device_list->count;
                                    printf("\n");
                                }
                                 else {
                                    device_list->device[i_dev].sys_path[0] = '\0';
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
void free_storage_device_list(storage_device_list_t *const device_list)
{

    for (size_t i = 0U; i < device_list->count; ++i)
    {
        free_storage_device(&device_list->device[i]);
    }
    free(device_list->device);
    device_list->device = NULL;

    device_list->count = 0U;
}

int alloc_storage_device_list(storage_device_list_t *const device_list, size_t partition_list_size)
{

    if (partition_list_size < 1U)
        return EXIT_FAILURE;

    device_list->device = (storage_device_t *)malloc(sizeof(storage_device_t) * partition_list_size);

    if (device_list->device == NULL)
        return EXIT_FAILURE;

    for (size_t i = 0U; i < partition_list_size; ++i)
    {
        init_storage_device(&device_list->device[i]);
    }
    device_list->count = partition_list_size;

    return EXIT_SUCCESS;
}

void free_storage_device(storage_device_t *const device)
{

    for (size_t i = 0U; i > device->partition_count; ++i)
    {
        free_partition(&device->partitions[i]);
    }
    free(device->partitions);
    device->partitions = NULL;

    device->partition_count = 0U;
}

void free_partition(partition_t *const partition)
{
    free(partition->mount_points);
    partition->mount_points = NULL;
    partition->mount_point_count = 0U;
}

void replace_all_chars(char *const string, char delete, char add)
{

    char *temp = string;
    while ((temp = strchr(temp, delete)))
        *temp = add;
}


/* Returns >= 0 if successful. If error in Unix returns negated errno. */
int scsi_pt_open_device(const char * device_name, int read_only, int verbose)
{
    /* 04000 octal = 2048 decimal*/
    int oflags = O_NONBLOCK;
    int fd;

    oflags |= (read_only ? O_RDONLY : O_RDWR);

    if (verbose > 1) {
        if (NULL == sg_warnings_strm)
            sg_warnings_strm = stderr;
        fprintf(sg_warnings_strm, "open %s with flags=0x%x\n", device_name, oflags);
    }

    fd = open(device_name, oflags);
    if (fd < 0)
        fd = -errno;
    return fd;
}

int sg_cmds_open_device(const char * device_name, int read_only,
                        int verbose)
{
    return scsi_pt_open_device(device_name, read_only, verbose);
}

static char safe_errbuf[64] = {'u', 'n', 'k', 'n', 'o', 'w', 'n', ' ',
                               'e', 'r', 'r', 'n', 'o', ':', ' ', 0};

char * safe_strerror(int errnum)
{
    size_t len;
    char * errstr;
  
    if (errnum < 0)
        errnum = -errnum;
    errstr = strerror(errnum);
    if (NULL == errstr) {
        len = strlen(safe_errbuf);
        snprintf(safe_errbuf + len, sizeof(safe_errbuf) - len, "%i", errnum);
        safe_errbuf[sizeof(safe_errbuf) - 1] = '\0';  /* bombproof */
        return safe_errbuf;
    }
    return errstr;
}
