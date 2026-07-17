#include "wheeltec_link.h"

#include "include.h"

#define UART_LINE_SIZE       64U
#define UART_TX_BUFFER_SIZE  1024U

static uint8_t tx_buffer[UART_TX_BUFFER_SIZE];
static uint16_t tx_head;
static uint16_t tx_tail;
static char rx_line[UART_LINE_SIZE];
static uint8_t rx_line_length;

static uint16_t TxFree(void)
{
    if (tx_head >= tx_tail)
    {
        return (uint16_t)(UART_TX_BUFFER_SIZE - (tx_head - tx_tail) - 1U);
    }
    return (uint16_t)(tx_tail - tx_head - 1U);
}

void WheeltecLink_Init(void)
{
    LQConfig_UART_InitTypeDef_t uart = {
        .Tx = UART0_TX_Pin_A_10,
        .Rx = UART0_RX_Pin_A_11,
        .BaudRate = 9600U,
        .Mode = DL_UART_MODE_NORMAL,
        .Direction = DL_UART_DIRECTION_TX_RX,
        .StopBits = DL_UART_STOP_BITS_ONE,
        .Parity = DL_UART_PARITY_NONE,
        .FlowControl = DL_UART_FLOW_CONTROL_NONE,
        .WordLength = DL_UART_WORD_LENGTH_8_BITS,
    };

    tx_head = 0U;
    tx_tail = 0U;
    rx_line_length = 0U;
    LQ_UART_Init(LQ_UART0, &uart);

    /* 使用 FIFO，但 RX 在主循环中高频轮询，绕开库中不稳定的中断索引路径。 */
    DL_UART_disable(UART0);
    DL_UART_enableFIFOs(UART0);
    DL_UART_setRXFIFOThreshold(UART0, DL_UART_RX_FIFO_LEVEL_ONE_ENTRY);
    DL_UART_enable(UART0);
}

uint8_t WheeltecLink_SendText(const char *text)
{
    size_t length = strlen(text);

    /* 一条消息必须完整入队，空间不足时整体丢弃，禁止发送半帧破坏协议同步。 */
    if (length > TxFree()) return 0U;
    while (length-- > 0U)
    {
        tx_buffer[tx_head] = (uint8_t)*text++;
        tx_head = (uint16_t)((tx_head + 1U) % UART_TX_BUFFER_SIZE);
    }
    return 1U;
}

void WheeltecLink_ServiceTx(void)
{
    /* 每次只填充硬件 FIFO；真正发送由 UART 外设完成，不阻塞 10 ms 控制任务。 */
    while (tx_tail != tx_head && !DL_UART_isTXFIFOFull(UART0))
    {
        DL_UART_transmitData(UART0, tx_buffer[tx_tail]);
        tx_tail = (uint16_t)((tx_tail + 1U) % UART_TX_BUFFER_SIZE);
    }
}

void WheeltecLink_Poll(WheeltecLineHandler handler)
{
    while (!DL_UART_isRXFIFOEmpty(UART0))
    {
        char byte = (char)DL_UART_receiveData(UART0);
        if (byte == '\r' || byte == '\n')
        {
            if (rx_line_length > 0U)
            {
                rx_line[rx_line_length] = '\0';
                if (handler != NULL) handler(rx_line);
                rx_line_length = 0U;
            }
        }
        else if (rx_line_length < UART_LINE_SIZE - 1U)
        {
            rx_line[rx_line_length++] = byte;
        }
        else
        {
            /* 超长行直接丢弃，等待下一个换行重新同步。 */
            rx_line_length = 0U;
        }
    }
}
