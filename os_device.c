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
#include "os_mutex.h"
#include "board/os_board.h"

static LIST_HEAD(__os_device_list);
static struct os_mutex __os_device_mutex;

/********************* system uart *********************/
static struct os_device* sys_uart_handle;
struct os_device* os_get_sys_uart_device_handle(void)
{
    return sys_uart_handle;
}

static void sys_uart_init(void)
{
    os_device_register(os_get_sys_uart_device(), "sys_uart", OS_DEVICE_RW);
    sys_uart_handle =  os_device_find("sys_uart");
    os_device_init(sys_uart_handle);
    os_device_open(sys_uart_handle, OS_DEVICE_RW);
}

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

    os_mutex_lock(&__os_device_mutex, OS_MUTEX_NEVER_TIMEOUT);
    list_for_each(curren_pos, &__os_device_list)
    {
        kobj = os_list_entry(curren_pos, struct os_kobject, _list);
        if (strlen(name) == strlen(kobj->_name) &&
            strcmp(name, kobj->_name) == 0) {
            dev = os_container_of(kobj, struct os_device, _kobject);
            os_mutex_unlock(&__os_device_mutex);
            return dev;
        }
    }
    os_mutex_unlock(&__os_device_mutex);
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

    os_mutex_lock(&__os_device_mutex, OS_MUTEX_NEVER_TIMEOUT);

    list_add_tail(&__os_device_list, &(dev->_kobject._list));
    dev->_flag |= OS_DEVICE_REGISTER;

    os_mutex_unlock(&__os_device_mutex);

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

    os_mutex_lock(&__os_device_mutex, OS_MUTEX_NEVER_TIMEOUT);

    dev->_flag &= (~OS_DEVICE_REGISTER);
    os_kobject_deinit(&dev->_kobject);

    os_mutex_unlock(&__os_device_mutex);

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

void os_sys_device_init(void)
{
    os_mutex_init(&__os_device_mutex, OS_MUTEX_NO_RECURSIVE);
    sys_uart_init();
}
