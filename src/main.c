#include <pspsdk.h>
#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <psputilsforkernel.h>
#include <psppower.h>
#include <psprtc.h>

#include "overclock.h"
#include "hook.h"

PSP_MODULE_INFO("expover-plugin", 0x1000, 1, 1);
PSP_NO_CREATE_MAIN_THREAD();
PSP_HEAP_SIZE_KB(512);

int thid, alive = 0;
int delay = 0, lastFreq = DEFAULT_FREQUENCY;

static inline int displaySetFrameBuf(void *fbuf, int width, int format, int sync) {
  
  void *frame = (void*)(0x40000000 | (u32)fbuf);
  
  if (delay > 0) {
    
    int bytesPerPixel = 4;
    if (format != PSP_DISPLAY_PIXEL_FORMAT_8888) {
      bytesPerPixel = 2;
    }
    
    u32 *ptr;
    
    int y = 0;
    while (y < 32) {
      ptr = (u32 *)((u8 *)frame + (y * width * bytesPerPixel));
      int x = 0;
      while (x < 32) {
        u32 color;
        
        int isInner = (x >= 8 && x < 24 && y >= 8 && y < 24);
        int isWhite = (lastFreq == THEORETICAL_FREQUENCY) ? !isInner : isInner;
        color = isWhite ? ((bytesPerPixel == 4) ? 0xFFFFFFFF : 0xFFFF) : 0;
      
        if (bytesPerPixel == 4) {
          *(u32 *)((u8 *)ptr + (x * 4)) = color;
        } else {
          *(u16 *)((u8 *)ptr + (x * 2)) = (u16)color;
        }
        x++;
      }
      y++;
    }
  }
  
  return _displaySetFrameBuf(fbuf, width, format, sync);
}

int switchOverclock() {
  SceCtrlData ctl;
  static int switched = 0;
  
  int ret = 0;
  sceCtrlPeekBufferPositive(&ctl, 1);
  if (
    !switched &&
    (ctl.Buttons & PSP_CTRL_NOTE) &&
    (ctl.Buttons & PSP_CTRL_LTRIGGER) &&
    (ctl.Buttons & PSP_CTRL_RTRIGGER)
  ) {
    ret = 1;
    switched = 1;
  } else {
    switched = 0;
  }
  
  return ret;
}

int thread(SceSize args, void *argp) {
  
  alive = 1;
  int init = 0;
  u64 lastTime = 0;
  u64 prev, now;
  
  while (alive) {
    
    int width, format;
    void *frame = NULL;
    sceDisplayGetFrameBuf(&frame, &width, &format, 0);
    
    if (frame) {
      
      if(!init) {
        cancelOverclock();
        init = 1;
      }

      sceRtcGetCurrentTick(&prev);
      const int switching = switchOverclock();
      
      if (delay == 0 && switching) {
        
        const int freq = lastFreq == THEORETICAL_FREQUENCY ? DEFAULT_FREQUENCY : THEORETICAL_FREQUENCY;
        if (freq == THEORETICAL_FREQUENCY) {
          setOverclock();
          lastFreq = THEORETICAL_FREQUENCY;
        } else {
          cancelOverclock();
          lastFreq = DEFAULT_FREQUENCY;
        }
        
        lastTime = sceKernelGetSystemTimeWide();
        delay = 10;
      }
      
      if (delay > 0) {
        u64 currentTime = sceKernelGetSystemTimeWide();
        if (currentTime - lastTime >= 200000) {
          delay -= 1;
          lastTime = currentTime;
        }
      }
      
      sceRtcGetCurrentTick(&now);
    }
    sceKernelDelayThread(1000);
  }
  return sceKernelExitDeleteThread(0);
}

int module_start(SceSize args, void *argp) {
  
  _displaySetFrameBuf = hook("sceDisplay_Service", "sceDisplay", 0x289D82FE, (void*)displaySetFrameBuf);

  thid = sceKernelCreateThread("expover-thread", thread, 0x18, 0x8000, 0, NULL);
  if (thid >= 0) {
    sceKernelStartThread(thid, 0, NULL);
  }
  return 0;
}

int module_stop(SceSize args, void *argp) {
  
  if (alive) {
    alive = 0;
    SceUInt timeout = 500000;
    int ret = sceKernelWaitThreadEnd(thid, &timeout);
    if (ret < 0) {
      sceKernelTerminateDeleteThread(thid);
    }
  }
  
  return 0;
}
