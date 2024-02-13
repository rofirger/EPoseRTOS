/***********************
 * @file: os_board.c
 * @author: Feijie Luo
 * @Change Logs:
 * Date           Author       Notes
 * 2024-02-08     Feijie Luo   Support HPM6750
 * @note: 32bit risc-v mcu
 ***********************/

#include "../os_headfile.h"
#include "board.h"
#include "hpm_clock_drv.h"
#include "hpm_mchtmr_drv.h"
#include "hpm_pmp_drv.h"
#include "hpm_sysctl_drv.h"
#include "hpm_uart_drv.h"
#include "os_board.h"
#include "string.h"

/********************* systick *********************/
static void os_hw_systick_init(const uint64_t _reload_tick)
{
    // 设置CPU0的mchtmr
    sysctl_config_clock(HPM_SYSCTL, clock_node_mchtmr0, clock_source_osc0_clk0, 1);
    sysctl_add_resource_to_cpu0(HPM_SYSCTL, sysctl_resource_mchtmr0);
    board_ungate_mchtmr_at_lp_mode();

    // 开启中断
    enable_mchtmr_irq();
    mchtmr_init_counter(HPM_MCHTMR, 0);
    mchtmr_set_compare_value(HPM_MCHTMR, _reload_tick);
}

inline uint64_t os_hw_systick_get_reload(void)
{
    return CLOCK_COUNT_TO_MS(CONFIG_SYSTICK_CLOCK_FREQUENCY, 1);
}

inline uint64_t os_hw_systick_get_val(void)
{
    return mchtmr_get_count(HPM_MCHTMR);
}

/********************* software interrupt *********************/
static void os_hw_sw_init()
{
    intc_m_enable_swi();
    intc_m_init_swi();
}

/********************* system uart *********************/
#define SYS_UART (BOARD_APP_UART_BASE)
static struct os_device *sys_uart_handle;

struct os_device *os_get_sys_uart_device_handle(void)
{
    return sys_uart_handle;
}

os_handle_state_t sys_uart_hw_init(struct os_device *dev)
{
    board_init_console();
    uart_enable_irq(SYS_UART, uart_intr_rx_data_avail_or_timeout);
    intc_m_enable_irq_with_priority(BOARD_APP_UART_IRQ, 1);
    return OS_HANDLE_SUCCESS;
}

os_handle_state_t sys_uart_open(struct os_device *dev, enum os_device_flag open_flag)
{
    return OS_HANDLE_SUCCESS;
}

os_handle_state_t sys_uart_close(struct os_device *dev)
{
    return OS_HANDLE_SUCCESS;
}

os_size_t sys_uart_write(struct os_device *dev, os_off_t pos, const void *buffer, os_size_t size)
{
    hpm_stat_t stat = uart_send_data(SYS_UART, (uint8_t *)buffer, size);
    return 1;
}

os_size_t sys_uart_read(struct os_device *dev, os_off_t pos, void *buffer, os_size_t size)
{
    hpm_stat_t stat = uart_receive_data(SYS_UART, (uint8_t *)buffer, size);
    if (stat == status_fail)
        return 0;
    return 1;
}

os_handle_state_t sys_uart_rx_indicate(struct os_device *dev, os_size_t size)
{
#ifdef CONFIG_FISH
    static char sys_uart_tmp_rec_char;
    if (status_success == uart_receive_byte(SYS_UART, &sys_uart_tmp_rec_char))
        return os_fish_irq_handle_callback((unsigned int)sys_uart_tmp_rec_char);
    else 
        return OS_HANDLE_FAIL;
#else
    return OS_HANDLE_FAIL;
#endif
}

__os_inline void sys_uart_write_flush(void)
{
    uart_flush(SYS_UART);
}

static struct os_file_operation sys_uart_file_ops = {
    .init = sys_uart_hw_init,
    .write = sys_uart_write,
    .open = sys_uart_open,
    .close = sys_uart_close,
    .read = sys_uart_read};

static struct os_device sys_uart_device = {
    ._type = OS_DEVICE_TYPE_CHAR,
    ._id = 0,
    ._flag = OS_DEVICE_RW,
    .rx_indicate = sys_uart_rx_indicate,
    .tx_complete = NULL,
    ._user_data = NULL,
};

static void sys_uart_register(void)
{
    sys_uart_device._file_ops = sys_uart_file_ops;
    os_device_register(&sys_uart_device, "sys_uart", OS_DEVICE_RW);
    sys_uart_handle = os_device_find("sys_uart");
    os_device_init(sys_uart_handle);
    os_device_open(sys_uart_handle, OS_DEVICE_RW);
}

/********************* system uart *********************/
const uint32_t SYSTICK_RELOAD_VAL = (MS_TO_CLOCK_COUNT(1, CONFIG_SYSTICK_CLOCK_FREQUENCY));
void os_board_init(void)
{
    board_init_clock();
    board_init_pmp();
    // board_init_ahb();

    /* systick */
    os_hw_systick_init(SYSTICK_RELOAD_VAL);
    /* SW */
    os_hw_sw_init();

    sys_uart_register();
}

void systick_handler(void)
{
    // GET_INT_MSP();

    mchtmr_delay(HPM_MCHTMR, SYSTICK_RELOAD_VAL);
    os_clear_systick_flag();
    os_soft_timer_systick_handle();
    os_systick_handler();

    // FREE_INT_MSP();
}
SDK_DECLARE_MCHTMR_ISR(systick_handler)

void sysuart_handler(void)
{
    GET_INT_MSP();
    os_sys_enter_irq();

    if (uart_get_irq_id(SYS_UART) & uart_intr_id_rx_data_avail) {
        os_device_rx_indicate(sys_uart_handle, 1);
    }

    os_sys_exit_irq();
    FREE_INT_MSP();
}
SDK_DECLARE_EXT_ISR_M(BOARD_APP_UART_IRQ, sysuart_handler)
