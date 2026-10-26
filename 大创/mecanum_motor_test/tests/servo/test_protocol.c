#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "usart.h"
#include "st3215.h"
static USART_TypeDef uart;
UART_HandleTypeDef huart3 = {&uart};
static uint8_t regs[256], reply[16], len, offset, corrupt, enables;
static unsigned tx_count;
static uint32_t tick;
uint32_t HAL_GetTick(void) { return tick++ / 16U; }
int HAL_UART_Transmit(UART_HandleTypeDef *h, uint8_t *tx, uint16_t size, uint32_t timeout)
{
  uint8_t sum = 0, n = 0;
  unsigned i;
  (void)timeout;
  ++tx_count;
  assert(!(h->Instance->CR1 & USART_CR1_RE));
  assert(tx[0]==255 && tx[1]==255 && tx[2]==1);
  assert(size == tx[3]+4U);
  for (i=2; i<size; ++i) sum += tx[i];
  assert(sum == 255);
  if (tx[4]==3)
  {
    if (tx[5]==0x37) assert(tx[6]==1); /* Never unlock EEPROM. */
    memcpy(regs+tx[5],tx+6,size-7);
    if (tx[5]==0x28 && tx[6]==1) ++enables;
  }
  else { assert(tx[4]==2); n=tx[6]; }
  reply[0]=reply[1]=255;reply[2]=1;reply[3]=n+2;reply[4]=0;
  if (n) memcpy(reply+5,regs+tx[5],n);
  sum=0; for (i=2;i<n+5U;++i) sum+=reply[i];
  reply[n+5]=(uint8_t)~sum;
  if (corrupt) reply[n+5]^=1;
  len=n+6;offset=0;
  return HAL_OK;
}
int HAL_UART_Receive(UART_HandleTypeDef *h, uint8_t *data, uint16_t size, uint32_t timeout)
{
  (void)timeout;(void)size;
  assert(h->Instance->CR1 & USART_CR1_RE);
  if (offset==len) return 1;
  *data=reply[offset++]; return HAL_OK;
}
int main(void)
{
  regs[0x12]=0x18; /* Preserve unrelated phase flags. */
  assert(St3215_Init()); assert(regs[0x21]==3 && regs[0x28]==0);
  assert(regs[0x12]==0x1C && regs[0x37]==1);
  assert(St3215_Start()); assert(regs[0x2A]==0 && regs[0x2B]==0 && regs[0x28]==1);
  unsigned before = tx_count;
  assert(St3215_Step(1)); assert(regs[0x2A]==0x55 && regs[0x2B]==1);
  assert(tx_count == before+1 && regs[0x28]==1 && regs[0x21]==3);
  assert(St3215_Step(-1)); assert(regs[0x2A]==0x55 && regs[0x2B]==0x81);
  assert(St3215_Run()); assert(regs[0x21]==1);
  assert(regs[0x2E]==0xF4 && regs[0x2F]==1);
  assert(St3215_Stop()); assert(regs[0x28]==0);
  assert(St3215_Check()); regs[0x41]=4; assert(!St3215_Check());
  regs[0x41]=0; corrupt=1; enables=0;
  assert(!St3215_Run()); assert(enables==0);
  corrupt=0;
  uint8_t moving;
  regs[0x42]=1; assert(St3215_GetMoving(&moving) && moving==1);
  regs[0x42]=0; assert(St3215_GetMoving(&moving) && moving==0);
  regs[0x41]=4; assert(!St3215_GetMoving(&moving));
  regs[0x41]=0;
  assert(St3215_PrepareStep());assert(regs[0x21]==3&&regs[0x28]==0);
  assert(St3215_Start());
  assert(St3215_MoveAngle(-450,600));assert(regs[0x2A]==0&&regs[0x2B]==0x82);
  assert(!St3215_MoveAngle(0,600)&&!St3215_MoveAngle(7201,600));
  assert(St3215_RunSpeed(-1,800));assert(regs[0x21]==1&&regs[0x2E]==0x20&&regs[0x2F]==0x83);
  assert(!St3215_RunSpeed(0,500));
  puts("PASS: checksum/USART3/single-frame legs/relative direction/clear old goal/motion feedback/fault rejection");
  return 0;
}
