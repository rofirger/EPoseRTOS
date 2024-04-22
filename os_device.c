/***********************
 * @file: os_device.c
 * @author: Feijie Luo
 * @Change Logs:
 * Date           Author       Notes
 * 2023-10-21     Feijie Luo   First version
 * @note:
 ***********************/
#include "board/libcpu_headfile.h"
#include "os_device.h"
#include "string.h"
#include "components/lib/os_string.h"

static LIST_HEAD(__os_device_list);

void os_kobject_init(struct os_kobject *ko, const char *name)
{
    if (NULL == ko)
        return;
    unsigned int len = OS_NAME_MAX_LEN > strlen(name) ? strlen(name) : OS_NAME_MAX_LEN;
    os_memcpy(ko->_name, name, len);
    list_head_init(&ko->_list);
}

void os_kobject_deinit(struct os_kobject *ko)
{
    list_del_init(&ko->_list);
}

struct os_device *os_device_find(const char *name)
{
    struct list_head *curren_pos;
    struct os_kobject *kobj;
    struct os_device *dev;

    OS_ENTER_CRITICAL

    list_for_each(curren_pos, &__os_device_list)
    {
        kobj = os_list_entry(curren_pos, struct os_kobject, _list);
        if (strlen(name) == strlen(kobj->_name) &&
            strcmp(name, kobj->_name) == 0) {
            dev = os_container_of(kobj, struct os_device, _kobject);
            OS_EXIT_CRITICAL
            return dev;
        }
    }
    OS_EXIT_CRITICAL
    return NULL;
}

os_handle_state_t os_device_register(struct os_device *dev,
                                     const char *name,
                                     enum os_device_flag flag)
{
    if (NULL == dev ||
        NULL == name)
        return OS_HANDLE_FAIL;
    //
    if (NULL != os_device_find(name))
        return OS_HANDLE_FAIL;

    os_kobject_init(&dev->_kobject, name);
    dev->_flag = flag;
    dev->_ref_count = 0;
    dev->_open_flag = 0;

    OS_ENTER_CRITICAL

    list_add_tail(&__os_device_list, &(dev->_kobject._list));
    dev->_flag |= OS_DEVICE_REGISTER;

    OS_EXIT_CRITICAL

    return OS_HANDLE_SUCCESS;
}

inline bool os_device_is_register(struct os_device *dev)
{
    return ((dev->_flag & OS_DEVICE_REGISTER) != 0);
}

os_handle_state_t os_device_unregister(struct os_device *dev)
{
    if (NULL == dev)
        return OS_HANDLE_FAIL;

    OS_ENTER_CRITICAL

    dev->_flag &= (~OS_DEVICE_REGISTER);
    os_kobject_deinit(&dev->_kobject);

    OS_EXIT_CRITICAL

    return OS_HANDLE_SUCCESS;
}

os_handle_state_t os_device_init(struct os_device *dev)
{
    if (NULL == dev)
        return OS_HANDLE_FAIL;

    if (false == os_device_is_register(dev))
        return OS_HANDLE_FAIL;

    // if inited, return
    if (dev->_flag & OS_DEVICE_INIT)
        return OS_HANDLE_FAIL;

    if (NULL != dev->_file_ops.init) {
        if (OS_HANDLE_SUCCESS == dev->_file_ops.init(dev)) {
            dev->_flag |= OS_DEVICE_INIT;
            return OS_HANDLE_SUCCESS;
        }
    }
    return OS_HANDLE_FAIL;
}

os_handle_state_t os_device_open(struct os_device *dev, enum os_device_flag open_flag)
{
    if (NULL == dev)
        return OS_HANDLE_FAIL;

    if (!(dev->_flag & OS_DEVICE_INIT))
        if (OS_HANDLE_FAIL == os_device_init(dev))
            return OS_HANDLE_FAIL;

    if (NULL != dev->_file_ops.open)
        if (OS_HANDLE_SUCCESS == dev->_file_ops.open(dev, open_flag)) {
            dev->_open_flag |= open_flag;
            dev->_ref_count++;
            dev->_flag |= OS_DEVICE_OPEN;
            return OS_HANDLE_SUCCESS;
        }
    return OS_HANDLE_FAIL;
}

os_handle_state_t os_device_close(struct os_device *dev)
{
    if (NULL == dev)
        return OS_HANDLE_FAIL;

    if (dev->_ref_count == 0)
        return OS_HANDLE_FAIL;

    dev->_ref_count--;

    if (dev->_ref_count != 0)
        return OS_HANDLE_SUCCESS;

    if (NULL != dev->_file_ops.close)
        if (OS_HANDLE_SUCCESS == dev->_file_ops.close(dev)) {
            dev->_open_flag = OS_DEVICE_CLOSE;
            return OS_HANDLE_SUCCESS;
        }

    return OS_HANDLE_FAIL;
}

size_t os_device_read(struct os_device *dev,
                      os_off_t pos,
                      void *buffer,
                      os_size_t size)
{
    if (NULL == dev)
        return 0;

    if (0 == dev->_ref_count)
        return 0;

    if (NULL != dev->_file_ops.read &&
        (OS_DEVICE_READ & dev->_open_flag) != 0) {
        return dev->_file_ops.read(dev, pos, buffer, size);
    }

    return 0;
}

size_t os_device_write(struct os_device *dev,
                       os_off_t pos,
                       const void *buffer,
                       os_size_t size)
{
    if (NULL == dev)
        return 0;

    if (0 == dev->_ref_count)
        return 0;

    if (NULL != dev->_file_ops.write &&
        (OS_DEVICE_WRITE & dev->_open_flag) != 0) {
        return dev->_file_ops.write(dev, pos, buffer, size);
    }

    return 0;
}

os_handle_state_t os_device_ioctl(struct os_device *dev, int cmd, void *buffer)
{
    if (NULL == dev)
        return 0;

    if (0 == dev->_ref_count)
        return 0;

    if (NULL != dev->_file_ops.ioctl)
        return dev->_file_ops.ioctl(dev, cmd, buffer);
    return OS_HANDLE_FAIL;
}

os_handle_state_t os_device_rx_indicate(struct os_device *dev, os_size_t size)
{
    if (NULL == dev)
        return OS_HANDLE_FAIL;

    if (0 == dev->_ref_count)
        return OS_HANDLE_FAIL;

    if (NULL != dev->rx_indicate)
        return dev->rx_indicate(dev, size);

    return OS_HANDLE_SUCCESS;
}

os_handle_state_t os_device_tx_complete(struct os_device *dev, void *buffer)
{
    if (NULL == dev)
        return OS_HANDLE_FAIL;

    if (0 == dev->_ref_count)
        return OS_HANDLE_FAIL;

    if (NULL != dev->tx_complete)
        return dev->tx_complete(dev, buffer);

    return OS_HANDLE_SUCCESS;
}

os_handle_state_t os_device_set_rx_indicate(struct os_device *dev,
                                            os_handle_state_t (*rx_indicate)(struct os_device *dev, os_size_t size))
{
    if (NULL == dev)
        return OS_HANDLE_FAIL;

    dev->rx_indicate = rx_indicate;

    return OS_HANDLE_SUCCESS;
}

os_handle_state_t os_device_set_tx_complete(struct os_device *dev,
                                            os_handle_state_t (*tx_complete)(struct os_device *dev, void *buffer))
{
    if (NULL == dev)
        return OS_HANDLE_FAIL;

    dev->tx_complete = tx_complete;

    return OS_HANDLE_SUCCESS;
}
