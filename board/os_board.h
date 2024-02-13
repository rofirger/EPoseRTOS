/***********************
 * @file: os_board.h
 * @author: Feijie Luo
 * @Change Logs:
 * Date           Author       Notes
 * 2023-10-12     Feijie Luo   First version
 * @note:
 ***********************/
#ifndef _OS_BOARD_H_
#define _OS_BOARD_H_

#include "hpm_common.h"
#include "hpm_soc.h"

/* for heap */
extern int _ebss, _heap_end;
#define RTOS_HEAP_BEGIN ((void *)&_ebss)
#define RTOS_HEAP_END   ((void *)&_heap_end)
struct os_device *os_get_sys_uart_device_handle(void);
void sys_uart_write_flush(void);
void os_board_init(void);
uint64_t os_hw_systick_get_reload(void);
uint64_t os_hw_systick_get_val(void);

#endif
