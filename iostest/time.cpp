#include <cstdio>
#include <ctime>

#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
#include <wiiuse/wpad.h>

int main()
{
  // Init video hardware
  VIDEO_Init();
  auto rmode = VIDEO_GetPreferredMode(nullptr);
  auto xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
  console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);
  VIDEO_Configure(rmode);
  VIDEO_SetNextFramebuffer(xfb);
  VIDEO_SetBlack(false);
  VIDEO_Flush();
  VIDEO_WaitVSync();
  if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();
  printf("\x1b[2;0H");

  printf("Time: %ld\n", time(nullptr));
  while (true);
  return 0;
}
