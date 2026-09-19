#include <Display/LCD/lcd_st7305.h>
#include <Display/Graphics.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t expected[15000],actual[15000];
static uint8_t mode, command, capture, *target;
static size_t used;
static unsigned callbacks, fail_row;
static void Mode(const SPI_DisplayBus *b,uint8_t m){(void)b;mode=m;}
static int Transfer(const SPI_DisplayBus *b,const uint8_t *d,size_t n,uint8_t dma)
{
 (void)b;(void)dma;
 if(!mode){assert(n==1);command=*d;}
 else if(capture&&command==0x2c){
   if(fail_row&&used/n+1==fail_row)return -1;
   assert(used+n<=15000);memcpy(target+used,d,n);used+=n;
 }
 return 0;
}
static void Delay(void *c,uint32_t n){(void)c;(void)n;}
static ST7305_Status Scene(void *context)
{
 const ROTATION rotation=*(ROTATION *)context;
 const unsigned width=(rotation==ROTATION_90||rotation==ROTATION_270)?400:300;
 const unsigned height=(width==400)?300:400;
 ++callbacks;
 /* Pixel/rect paths include clipping across byte, page and rotated edges. */
 DrawFilledRect(0,0,width,height,(COLOR){255,255,255});
 for(unsigned i=0;i<67;i++){
   DrawFilledRect((i*47)%width,(i*31)%height,17,23,(COLOR){0,0,0});
   DrawHLine(0,(i*13)%height,width-1,(COLOR){0,0,0});
   DrawVLine((i*7)%width,0,height-1,(COLOR){255,255,255});
 }
 Pixel p={width-1,height-1,{0,0,0}};DrawPixel(&p);
 return ST7305_OK;
}
static ST7305_Status Nested(void *context)
{ assert(LCD_ST7305_RenderPaged(Scene,context)==ST7305_ERR_STATE);return Scene(context); }
int main(void)
{
 SPI_DisplayBus bus={.transfer=Transfer,.data_mode=Mode};
 uint8_t full[15200],page[1522],line[75];
 for(ROTATION rotation=NO_ROTATION;rotation<=ROTATION_270;rotation++){
   ST7305_Binding b={.bus=&bus,.panel=&ST7305_PANEL_FD042MN_ZF21_H06_B,
     .delay_ms=Delay,.framebuffer=full,.framebuffer_size=sizeof(full),
     .line_buffer=line,.line_buffer_size=sizeof(line),.rotation=rotation};
   capture=0;assert(LCD_ST7305_Bind(&b)==ST7305_OK);assert(LCD_ST7305_Initialize()==ST7305_OK);
   Scene(&rotation);used=0;target=expected;capture=1;
   assert(LCD_ST7305_Refresh()==ST7305_OK&&used==15000);
   b.framebuffer=page+1;b.framebuffer_size=1520;b.page_rows=40;
   page[0]=0xa5;page[1521]=0x5a;capture=0;
   assert(LCD_ST7305_Bind(&b)==ST7305_OK);assert(LCD_ST7305_Initialize()==ST7305_OK);
   callbacks=used=0;target=actual;capture=1;
   assert(LCD_ST7305_Refresh()==ST7305_ERR_STATE); /* no stale previous-page refresh */
   assert(LCD_ST7305_RenderPaged(Nested,&rotation)==ST7305_OK);
   assert(callbacks==10&&used==15000&&memcmp(expected,actual,used)==0);
   assert(page[0]==0xa5&&page[1521]==0x5a);
   fail_row=35;used=0;assert(LCD_ST7305_RenderPaged(Scene,&rotation)==ST7305_ERR_IO);
   assert(!LCD_ST7305_IsReady());fail_row=capture=0;
   assert(LCD_ST7305_Initialize()==ST7305_OK);
   b.page_rows=39;assert(LCD_ST7305_Bind(&b)==ST7305_ERR_PARAM);
   b.page_rows=40;b.framebuffer_size=1519;assert(LCD_ST7305_Bind(&b)==ST7305_ERR_PARAM);
 }
 LCD_ST7305_Unbind();puts("paged ST7305: four rotations byte-identical to full frame, bounds and I/O recovery passed");return 0;
}
