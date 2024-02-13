
/***********************
 * @file: os_device.h
 * @author: Feijie Luo
 * @Change Logs:
 * Date           Author       Notes
 * 2023-10-21     Feijie Luo   First version
 * @note:
 ***********************/

#ifndef _OS_DEVICE_H_
#define _OS_DEVICE_H_

#include "os_block.h"
#include "os_config.h"
#include "os_list.h"

#define OS_NAME_MAX_LEN (16)

enum os_device_type {
    OS_DEVICE_TYPE_CHAR,
    OS_DEVICE_TYPE_PIN,
};

enum os_device_flag {
    OS_DEVICE_CLOSE = 0,
    OS_DEVICE_READ = (1 << 0),
    OS_DEVICE_WRITE = (1 << 1),
    OS_DEVICE_RW = ((1 << 0) | 1 << 1),

    OS_DEVICE_REGISTER = (1 << 31),
    OS_DEVICE_INIT = (1 << 30),
    OS_DEVICE_OPEN = (1 << 29),
};

struct os_kobject {
    char _name[OS_NAME_MAX_LEN];
    struct list_head _list;
};

struct os_device;

struct os_file_operation {
    os_handle_state_t (*init)(struct os_device *dev);
    os_handle_state_t (*open)(struct os_device *dev, enum os_device_flag open_flag);
    os_handle_state_t (*close)(struct os_device *dev);
    os_size_t (*read)(struct os_device *dev, os_off_t pos, void *buffer, os_size_t size);
    os_size_t (*write)(struct os_device *dev, os_off_t pos, const void *buffer, os_size_t size);
    os_handle_state_t (*ioctl)(struct os_device *dev, int cmd, void *buffer);
};

#define OS_DEVICE_FLAG_MASK (0xFFFF)

struct os_device {
    enum os_device_type _type;
    // OS_DEVICE_RW
    enum os_device_flag _flag;
    //
    enum os_device_flag _open_flag;
    unsigned char _ref_count;
    unsigned char _id;

    os_handle_state_t (*rx_indicate)(struct os_device *dev, os_size_t size);
    os_handle_state_t (*tx_complete)(struct os_device *dev, void *buffer);

    struct os_file_operation _file_ops;
    void *_user_data;

    struct os_kobject _kobject;
};

struct os_device *os_device_find(const char *name);
os_handle_state_t os_device_register(struct os_device *dev,
                                     const char *name,
                                     enum os_device_flag flag);
bool os_device_is_register(struct os_device *dev);
os_handle_state_t os_device_unregister(struct os_device *dev);
os_handle_state_t os_device_init(struct os_device *dev);
os_handle_state_t os_device_open(struct os_device *dev, enum os_device_flag open_flag);
os_handle_state_t os_device_close(struct os_device *dev);
size_t os_device_read(struct os_device *dev,
                      os_off_t pos,
                      void *buffer,
                      os_size_t size) __OS_OPTIMIZE_O0__;
size_t os_device_write(struct os_device *dev,
                       os_off_t pos,
                       const void *buffer,
                       os_size_t size) __OS_OPTIMIZE_O0__;
os_handle_state_t os_device_ioctl(struct os_device *dev, int cmd, void *buffer);
os_handle_state_t os_device_rx_indicate(struct os_device *dev, os_size_t size);
os_handle_state_t os_device_tx_complete(struct os_device *dev, void *buffer);
os_handle_state_t os_device_set_rx_indicate(struct os_device *dev,
                                            os_handle_state_t (*rx_indicate)(struct os_device *dev, os_size_t size));
os_handle_state_t os_device_set_tx_complete(struct os_device *dev,
                                            os_handle_state_t (*tx_complete)(struct os_device *dev, void *buffer));

#endif
