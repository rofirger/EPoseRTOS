/***********************
 * @file: os_semaphore.c
 * @author: Feijie Luo
 * @Change Logs:
 * Date           Author       Notes
 * 2023-10-19     Feijie Luo   First version
 * @note:
 ***********************/

#include "os_semaphore.h"
#include "os_config.h"
#include "os_sched.h"
#include "os_tick.h"
#include "board/libcpu_headfile.h"
#include "stddef.h"

os_handle_state_t os_sem_init(struct os_sem* sem, unsigned short value)
{
    if (NULL == sem)
        return OS_HANDLE_FAIL;
    sem->_value = value;
    os_block_init(&sem->_block_obj, OS_BLOCK_SEM);
    return OS_HANDLE_SUCCESS;
}

__os_inline os_handle_state_t os_sem_try_take(struct os_sem* sem)
{
    if (NULL == sem)
        return OS_HANDLE_FAIL;
    return (sem->_value > 0) ? OS_HANDLE_SUCCESS : OS_HANDLE_FAIL;
}

os_handle_state_t os_sem_take(struct os_sem* sem, unsigned int time_out)
{
    if (NULL == sem)
        return OS_HANDLE_FAIL;
    unsigned int _critical_state = os_port_enter_critical();
    if (os_sem_try_take(sem)){
        sem->_value--;
        os_port_exit_critical(_critical_state);
        return OS_HANDLE_SUCCESS;
    } else {
        struct task_control_block* _current_task_tcb = os_get_current_task_tcb();
        // ����ǰ�������
        os_add_tick_task(_current_task_tcb, time_out, &sem->_block_obj);
        os_port_exit_critical(_critical_state);
        __os_sched();
        // �ô���Ϊ�˸����ж��쳣������ǰ����ʱ��
        while(os_task_is_block(_current_task_tcb)){};

        _critical_state = os_port_enter_critical();

        // ��ʱ
        if (_current_task_tcb->_task_block_state == OS_TASK_BLOCK_TIMEOUT) {
            _current_task_tcb->_task_block_state = OS_TASK_BLOCK_NONE;
            os_port_exit_critical(_critical_state);
            return OS_HANDLE_FAIL;
        }else {
            _current_task_tcb->_task_block_state = OS_TASK_BLOCK_NONE;
            os_port_exit_critical(_critical_state);
            return OS_HANDLE_SUCCESS;
        }
    }
}

static bool is_take = false;
__os_static __os_inline void __os_sem_release_cb(struct task_control_block* task)
{
    is_take = true;
    // ��������ӹ��ص�tick��ժ��
    list_del_init(&(task->_bt_nd));
}

os_handle_state_t os_sem_release(struct os_sem* sem)
{
    if (NULL == sem)
        return OS_HANDLE_FAIL;
    unsigned int _critical_state = os_port_enter_critical();
    sem->_value++;
    is_take = false;
    os_block_wakeup_first_task(&sem->_block_obj, __os_sem_release_cb);
    if (is_take)
        sem->_value--;
    os_port_exit_critical(_critical_state);
    return OS_HANDLE_SUCCESS;
}
