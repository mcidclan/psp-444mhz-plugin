#include <pspsdk.h>
#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <psputilsforkernel.h>
#include <psppower.h>
#include <psprtc.h>

#include "overclock.h"
#include "hook.h"

PSP_MODULE_INFO("expover-plugin", 0x1006, 1, 1);
PSP_NO_CREATE_MAIN_THREAD();
PSP_HEAP_SIZE_KB(512);

int thid, alive = 0;
int delay = 0, lastFreq = DEFAULT_FREQUENCY;

static const u32 __attribute__((aligned(16))) COLORS_16[3] = {0xFFFF, 0x001F, 0x07E0};
static const u32 __attribute__((aligned(16))) COLORS_32[3] = {0xFFFFFFFF, 0xFF0000FF, 0xFF00FF00};

static inline int exitGameWithStatus() {
  cancelOverclock();
  return _exitGameWithStatus();
}
static inline void exitGame() {
  cancelOverclock();
  _exitGame();
}

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
        
        int drawInner = (x >= 8 && x < 24 && y >= 8 && y < 24);
        int activated = (lastFreq == THEORETICAL_FREQUENCY);
        
        int colorIndex = (!activated && !drawInner) ? 2 : (activated && drawInner) ? 1 : 0;
        u32 color = (bytesPerPixel == 4) ? COLORS_32[colorIndex] : COLORS_16[colorIndex];

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
    !switched && (
      (ctl.Buttons & PSP_CTRL_NOTE) ||
      (ctl.Buttons & PSP_CTRL_CIRCLE)
    ) &&
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
  
  sceKernelDelayThread(6000000);
  
  alive = 1;
  int init = 0;
  u64 lastTime = 0;
  u64 prev, now;
  int width, format;
  void *frame = NULL;
  
  while (alive) {
    
    sceDisplayGetFrameBuf(&frame, &width, &format, 0);
    
    if (frame) {
      
      if (!init) {
        initOverclock(&delay);
        delay = 10;
        init = 1;
      }

      sceRtcGetCurrentTick(&prev);
      const int switching = switchOverclock();
      
      if (delay == 0 && switching) {
        const int freq = lastFreq == THEORETICAL_FREQUENCY ? DEFAULT_FREQUENCY : THEORETICAL_FREQUENCY;
        if (freq == THEORETICAL_FREQUENCY) {
          delay = 1;
          lastFreq = THEORETICAL_FREQUENCY;
          setOverclock();
        } else {
          delay = 1;
          lastFreq = DEFAULT_FREQUENCY;
          cancelOverclock();
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
  _exitGame = hook("sceLoadExec", "LoadExecForUser", 0x05572A5F, (void*)exitGame);
  _exitGameWithStatus = hook("sceLoadExec", "LoadExecForUser", 0x2AC9954B, (void*)exitGameWithStatus);

  thid = sceKernelCreateThread("expover-thread", thread, 0x18, 0x8000, PSP_THREAD_ATTR_VFPU, NULL);
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
