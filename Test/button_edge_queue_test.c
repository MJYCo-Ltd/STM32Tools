#include <Button.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
static GPIO_TypeDef port;
static GPIO_PinState levels[16];
static uint32_t tick, primask;
uint32_t HAL_GetTick(void) { return tick; }
uint32_t __get_PRIMASK(void) { return primask; }
void __disable_irq(void) { primask=1U; }
void __set_PRIMASK(uint32_t value) { primask=value; }
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *p,uint16_t pin)
{ (void)p; for(unsigned i=0;i<16U;++i) if(pin==(1U<<i))return levels[i]; return GPIO_PIN_SET; }
static void Edge(Button *b, uint32_t at, uint8_t pressed)
{
  tick=at;
  for(unsigned i=0;i<16U;++i) if(b->pin==(1U<<i))levels[i]=pressed?GPIO_PIN_RESET:GPIO_PIN_SET;
  Button_NotifyExti(b);
}
static void Init(Button *b)
{ for(unsigned i=0;i<16U;++i)levels[i]=GPIO_PIN_SET; tick=0U; Button_Init(b,&port,1U,GPIO_PIN_RESET,30U); assert(Button_IsRegistered(b)); }
int main(void)
{
  Button b;
  Init(&b);
  Edge(&b,100U,1U); Edge(&b,300U,0U); tick=400U;
  assert(Button_Process(&b)==BUTTON_EVENT_SHORT); /* Both edges precede task. */
  assert(Button_Process(&b)==BUTTON_EVENT_NONE);
  Edge(&b,500U,1U); Edge(&b,2000U,0U); tick=2100U;
  assert(Button_Process(&b)==BUTTON_EVENT_LONG);
  Edge(&b,2200U,1U); Edge(&b,2220U,0U); tick=2300U;
  assert(Button_Process(&b)==BUTTON_EVENT_NONE);
  Edge(&b,2400U,1U); Edge(&b,2600U,0U); Edge(&b,2700U,1U); Edge(&b,2900U,0U); tick=3000U;
  assert(Button_Process(&b)==BUTTON_EVENT_SHORT);
  assert(Button_NextWakeDelay(&b,tick)==0U);
  assert(Button_Process(&b)==BUTTON_EVENT_SHORT);
  Edge(&b,4000U,1U); tick=9000U;
  assert(Button_Process(&b)==BUTTON_EVENT_EXTRA_LONG);
  Edge(&b,9010U,0U); tick=9050U;
  assert(Button_Process(&b)==BUTTON_EVENT_NONE);
  Edge(&b,10000U,1U); Edge(&b,14999U,0U); tick=16000U;
  assert(Button_Process(&b)==BUTTON_EVENT_LONG); /* Release before threshold. */
  for(unsigned i=0;i<BUTTON_EDGE_CAPACITY+4U;++i)Edge(&b,17000U+i,(uint8_t)(!(i&1U)));
  tick=18000U; assert(Button_Process(&b)==BUTTON_EVENT_NONE); assert(Button_OverflowCount(&b)>0U);
  Edge(&b,18100U,1U); Edge(&b,18300U,0U); tick=18400U;
  assert(Button_Process(&b)==BUTTON_EVENT_SHORT);
  Button_Deinit(&b); Init(&b);
  Edge(&b,UINT32_MAX-100U,1U); Edge(&b,100U,0U); tick=150U;
  assert(Button_Process(&b)==BUTTON_EVENT_SHORT);
  Button_Deinit(&b);
  levels[0]=GPIO_PIN_RESET; Button_Init(&b,&port,1U,GPIO_PIN_RESET,30U);
  Edge(&b,6000U,0U); tick=6040U; assert(Button_Process(&b)==BUTTON_EVENT_NONE);
  Button_Deinit(&b);
  puts("Button queued-edge/debounce/overflow/wrap regressions passed"); return 0;
}
