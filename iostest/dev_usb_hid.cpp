#include <cstring>
#include <vector>

#include <ogc/ios.h>
#include <ogc/ipc.h>
#include <ogc/system.h>
#include <unistd.h>

#include "hwtests.h"
#include "CommonTypes.h"
#include "StringUtil.h"

static s32 hId = -1;

enum USBV4IOCtl
{
  USBV4_IOCTL_GETDEVICECHANGE = 0,
  USBV4_IOCTL_SET_SUSPEND = 1,
  USBV4_IOCTL_CTRLMSG = 2,
  USBV4_IOCTL_INTRMSG_IN = 3,
  USBV4_IOCTL_INTRMSG_OUT = 4,
  USBV4_IOCTL_GET_US_STRING = 5,
  USBV4_IOCTL_GETVERSION = 6,
  USBV4_IOCTL_SHUTDOWN = 7,
  USBV4_IOCTL_CANCELINTERRUPT = 8,
};

void TestGETDEVCHANGE(const s32 fd)
{
  network_printf("USBV4_IOCTL_GETDEVICECHANGE\n");

  u8* buf = (u8*)iosAlloc(hId, 0x600);
  network_printf("iosAlloc\n");
  const int ret = IOS_Ioctl(fd, USBV4_IOCTL_GETDEVICECHANGE, nullptr, 0, buf, 0x600);
  network_printf("ret = %d\n", ret);
  if (ret < 0)
    return;

  network_printf("buffer:\n%s\n", ArrayToString(buf, 0x600, 4).c_str());
  iosFree(hId, buf);
}

int main()
{
  if (IOS_GetVersion() != 55)
  {
    IOS_ReloadIOS(55);
    usleep(50000);
  }

  network_init();
  printf("Current IOS version: %d\n", IOS_GetVersion());

  hId = iosCreateHeap(16384);
  if (hId < 0)
  {
    network_printf("Failed to create heap\n");
    return IPC_ENOMEM;
  }

  const int fd = IOS_Open("/dev/usb/hid", IPC_OPEN_NONE);
  if (fd < 0)
  {
    network_printf("Failed to open /dev/usb/hid: ret %d\n", fd);
    return 1;
  }

  u8* buf = (u8*)iosAlloc(hId, 0x20);
  const int ret = IOS_Ioctl(fd, USBV4_IOCTL_GETVERSION, nullptr, 0, buf, 0x20);
  network_printf("get version ret = %x\n", ret);
  if (ret != 0x40001)
  {
    network_printf("wrong version, aborting\n");
    return 1;
  }

  TestGETDEVCHANGE(fd);
  TestGETDEVCHANGE(fd);

  IOS_Close(fd);

  usleep(10000);
  network_printf("Shutting down...\n");
  network_shutdown();
  return 0;
}
