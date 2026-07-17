#include "wheeltec_link.h"

#include "include.h"
#include "LQ_dma.h"

#define UART_LINE_SIZE       64U
#define UART_TX_BUFFER_SIZE  1024U
#define UART_TX_DMA_CHANNEL  DMA_Channel_0

static uint8_t tx_buffer[UART_TX_BUFFER_SIZE];
static uint16_t tx_head;
static uint16_t tx_tail;
static uint16_t tx_dma_length;
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
    LQConfig_DMA_InitTypeDef_t dma = {
        .trigger = (uint8_t)DMA_Trigger_UART0_TX,
        .triggerType = DL_DMA_TRIGGER_TYPE_EXTERNAL,
        .transferMode = DL_DMA_SINGLE_TRANSFER_MODE,
        .srcWidth = DL_DMA_WIDTH_BYTE,
        .destWidth = DL_DMA_WIDTH_BYTE,
        .srcIncrement = DL_DMA_ADDR_INCREMENT,
        .destIncrement = DL_DMA_ADDR_UNCHANGED,
    };

    tx_head = 0U;
    tx_tail = 0U;
    tx_dma_length = 0U;
    rx_line_length = 0U;
    LQ_UART_Init(LQ_UART0, &uart);

    /* 使用 FIFO，但 RX 在主循环中高频轮询，绕开库中不稳定的中断索引路径。 */
    DL_UART_disable(UART0);
    DL_UART_enableFIFOs(UART0);
    DL_UART_setRXFIFOThreshold(UART0, DL_UART_RX_FIFO_LEVEL_ONE_ENTRY);
    DL_UART_setTXFIFOThreshold(UART0, DL_UART_TX_FIFO_LEVEL_3_4_EMPTY);
    DL_UART_enable(UART0);

    /* UART0 TX 请求逐字节触发 DMA0；DMA 完成状态由主循环轮询，无需中断。 */
    LQ_DMA_Init(UART_TX_DMA_CHANNEL, &dma);
    LQ_DMA_SetDstAddr(UART_TX_DMA_CHANNEL,
                      LQ_UART_GetTXRegister(LQ_UART0));
    LQ_UART_EnableDMATransmit(LQ_UART0);
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
    /* DMA 完成一段连续内存后再推进环形队列，回绕部分留到下一次提交。 */
    uint16_t length;

    if (tx_dma_length > 0U)
    {
        if (DL_DMA_getTransferSize(DMA, UART_TX_DMA_CHANNEL) > 0U) return;
        DL_DMA_disableChannel(DMA, UART_TX_DMA_CHANNEL);
        tx_tail = (uint16_t)((tx_tail + tx_dma_length) % UART_TX_BUFFER_SIZE);
        tx_dma_length = 0U;
    }

    if (tx_tail == tx_head) return;
    length = (tx_head > tx_tail) ?
             (uint16_t)(tx_head - tx_tail) :
             (uint16_t)(UART_TX_BUFFER_SIZE - tx_tail);

    LQ_DMA_SetSrcAddr(UART_TX_DMA_CHANNEL,
                      (uint32_t)&tx_buffer[tx_tail]);
    LQ_DMA_SetTransferSize(UART_TX_DMA_CHANNEL, length);
    tx_dma_length = length;
    DL_DMA_enableChannel(DMA, UART_TX_DMA_CHANNEL);
    DL_DMA_startTransfer(DMA, UART_TX_DMA_CHANNEL);
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
