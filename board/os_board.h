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

void os_board_init(void);
struct os_device* os_get_sys_uart_device_handle(void);
unsigned long os_hw_systick_get_reload(void);
unsigned long os_hw_systick_get_val(void);
void sys_uart_write_flush(void);

#endif
