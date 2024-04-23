/***********************
 * @file: os_board.c
 * @author: Feijie Luo
 * @Change Logs:
 * Date           Author       Notes
 * 2023-10-12     Feijie Luo   Support CH32V307
 * @note: 32bit risc-v mcu
 ***********************/

#include "../os_board.h"
#include "string.h"
#include "../../os_headfile.h"
#include "ch32v30x.h"
#include "ch32v30x_it.h"

SysTick_Type old_systick;

// 采用RISC-V内核滴答时钟进行计时. 每一次调用重启一次新的计时. 此函数不可随意调用，否则会造成程序出现时序错误！
static void os_hw_systick_init(const uint64_t _reload_tick)
{
    os_soft_timer_reset_systick_times();

    // 保存SysTick上文
    old_systick.CMP = SysTick->CMP;
    old_systick.CTLR = SysTick->CTLR;
    old_systick.SR = SysTick->SR;

    // 清除标志位
    SysTick->SR &= ~(1 << 0);
    // 此条件下必须设置为向下计数
    SysTick->CMP = _reload_tick;
    // 向下计数
    SysTick->CTLR |= (1 << 4);
    // 选择HCLK作为时基
    SysTick->CTLR |= (1 << 2);
    // 向上计数时更新为 0，向下计数时更新为比较值 | 计数器中断使能控制位 | 启动系统计数器 STK | 自动装载
    SysTick->CTLR |= (1 << 5) | (1 << 1) | (1 << 0) | (1 << 3);
    NVIC_SetPriority(SysTicK_IRQn, 0xf0);     //设置SysTick中断优先级
    NVIC_DisableIRQ(SysTicK_IRQn);
}

inline unsigned long os_hw_systick_get_reload(void)
{
    return SysTick->CMP;
}

inline unsigned long os_hw_systick_get_val(void)
{
    return SysTick->CNT;
}

void os_hw_systick_restore(void)
{
    SysTick->CMP = old_systick.CMP;
    SysTick->SR = old_systick.SR;
    SysTick->CTLR = old_systick.CTLR;
}

/********************* system uart *********************/
os_handle_state_t sys_uart_hw_init(struct os_device* dev)
{
    GPIO_InitTypeDef  GPIO_InitStructure  = {0};
    USART_InitTypeDef USART_InitStructure = {0};
    NVIC_InitTypeDef  NVIC_InitStructure  = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    // 接收和发送
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(USART1, &USART_InitStructure);

    /* 使能串口1中断 */
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

    USART_Cmd(USART1, ENABLE);

    return OS_HANDLE_SUCCESS;
}

os_handle_state_t sys_uart_open(struct os_device* dev, enum os_device_flag open_flag)
{
    return OS_HANDLE_SUCCESS;
}

os_handle_state_t sys_uart_close(struct os_device* dev)
{
    return OS_HANDLE_SUCCESS;
}

os_size_t sys_uart_write(struct os_device* dev, os_off_t pos, const void* buffer, os_size_t size)
{
    char* buf = (char*)buffer;
    int i;
    for(i = 0; i < size; i++)
    {
        while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET) {};
        USART_SendData(USART1, *buf++);

    }
    return 1;
}

os_size_t sys_uart_read(struct os_device* dev, os_off_t pos, void* buffer, os_size_t size)
{
    return 1;
}

os_handle_state_t sys_uart_rx_indicate(struct os_device* dev, os_size_t size)
{
#ifdef CONFIG_FISH
    static char sys_uart_tmp_rec_char;
    sys_uart_tmp_rec_char = USART1->DATAR & 0xFF;
    return os_fish_irq_handle_callback((unsigned int)sys_uart_tmp_rec_char);
#else
    return OS_HANDLE_FAIL;
#endif
}

static const struct os_file_operation sys_uart_file_ops = {
        .init  = sys_uart_hw_init,
        .write = sys_uart_write,
        .open  = sys_uart_open,
        .close = sys_uart_close,
        .read  = sys_uart_read
};

static struct os_device sys_uart_device = {
        ._type       = OS_DEVICE_TYPE_CHAR,
        ._id         = 0,
        ._flag       = OS_DEVICE_RW,
        .rx_indicate = sys_uart_rx_indicate,
        .tx_complete = NULL,
        ._file_ops = sys_uart_file_ops,
        ._user_data  = NULL,
};

inline void sys_uart_write_flush(void)
{

}

inline struct os_device* os_get_sys_uart_device(void)
{
    return &sys_uart_device;
}

/********************* system uart *********************/


void os_set_sys_heap_head(void* ptr);
void os_board_init(void)
{
    os_hw_systick_init(MS_TO_CLOCK_COUNT(1, CONFIG_SYSTICK_CLOCK_FREQUENCY));
    os_set_sys_heap_head(malloc(CONFIG_HEAP_SIZE));
    /* 与 SysTick 的优先级相同  */
    NVIC_SetPriority(Software_IRQn, 0xf0);
    NVIC_DisableIRQ(Software_IRQn);
}

/**
 * @brief Start interrupts for the kernel.
 *
 * This function is called in the `os_sys_start()` function. It's purpose is to enable
 * interrupts for the kernel. It ensures that the interrupts used by the kernel are enabled
 * during the `os_sys_start` stage.
 *
 * @return None
 *
 * @note This function is essential for proper functioning of the kernel, as it enables the
 *       interrupt mechanism required for handling various events and scheduling tasks. It
 *       should be called ONLY during the `os_sys_start` stage to ensure that interrupts are
 *       enabled at the appropriate time.
 *       The SysTick interrupt MUST be enabled in the os_board_start_interrupt() function
 *       and DISABLED during the initialization stage.
 */
void os_board_start_interrupt(void)
{
    NVIC_EnableIRQ(Software_IRQn);
    NVIC_EnableIRQ(SysTicK_IRQn);
}

void SysTick_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void SysTick_Handler(void)
{
    GET_INT_MSP();

    // 清除标志位
    os_clear_systick_flag();
    os_soft_timer_systick_handle();
    os_systick_handler();

    FREE_INT_MSP();
}

void USART1_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void USART1_IRQHandler(void)
{
    GET_INT_MSP();
    os_sys_enter_irq();

    if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
        os_device_rx_indicate(os_get_sys_uart_device_handle(), 1);
    }

    os_sys_exit_irq();
    FREE_INT_MSP();
}

