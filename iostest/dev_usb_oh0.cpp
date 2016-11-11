#include <vector>

#include <ogc/ios.h>
#include <ogc/ipc.h>
#include <ogc/system.h>
#include <unistd.h>

#include "hwtests.h"
#include "CommonTypes.h"
#include "StringUtil.h"

#include "usb.h"

static s32 hId = -1;

void TestGETDEVLIST(const s32 fd)
{
  network_printf("USBV0_IOCTL_GETDEVLIST (listing audio devices)\n");

  u8 cntdevs = 0;
  u8 num_descr = 5;
  const u8 audio_interface_class = 1;
  std::vector<u32> buf(num_descr << 3);

  const int ret = IOS_IoctlvFormat(hId, fd, USBV0_IOCTL_GETDEVLIST, "bb:dd", num_descr,
                                   audio_interface_class, &cntdevs, sizeof(cntdevs), buf.data(),
                                   buf.size());
  network_printf("ret = %d\n", ret);
  if (ret < 0)
    return;

  network_printf("buffer:\n%s\n", ArrayToString(reinterpret_cast<u8*>(buf.data()), buf.size()).c_str());
}

// Once the device is plugged in, the two hooks should be triggered immediately
void TestInsertHook(const s32 fd)
{
  network_printf("USBV0_IOCTL_DEVINSERTHOOK (test insert hook)\n");

  u16 vid = 0x46d;
  u16 pid = 0xa03;

  const s32 ret = IOS_IoctlvFormat(hId, fd, USBV0_IOCTL_DEVINSERTHOOK, "hh", vid, pid);
  network_printf("ret = %d\n", ret);
  if (ret < 0)
    return;

  const s32 ret2 = IOS_IoctlvFormat(hId, fd, USBV0_IOCTL_DEVINSERTHOOK, "hh", vid, pid);
  network_printf("ret2 = %d\n", ret2);
  if (ret < 0)
    return;
}

void TestSetAlternate(const s32 fd)
{
  network_printf("Testing SET ALTERNATE\n");

  /*
  data_addr = cmd_buffer.PayloadBuffer[0].m_Address;
  bmRequestType = Memory::Read_U8(cmd_buffer.InBuffer[0].m_Address);
  bmRequest = Memory::Read_U8(cmd_buffer.InBuffer[1].m_Address);
  wValue = Common::swap16(Memory::Read_U16(cmd_buffer.InBuffer[2].m_Address));
  wIndex = Common::swap16(Memory::Read_U16(cmd_buffer.InBuffer[3].m_Address));
  wLength = Common::swap16(Memory::Read_U16(cmd_buffer.InBuffer[4].m_Address));
  */

  u8 bmRequestType = 0x1;
  u8 bmRequest = 0xb;
  u16 wValue = 1;
  u16 wIndex = 0x1;
  u16 wLength = 0;
  u8 unknown = 0;
  u8 data[] = {};

  const s32 ret = IOS_IoctlvFormat(hId, fd, USBV0_IOCTL_CTRLMSG, "bbhhhb:d", bmRequestType, bmRequest, wValue, wIndex, wLength, unknown, data, 0);
  network_printf("ret = %d\n", ret);
  if (ret < 0)
    return;
}

int main()
{
  if (IOS_GetVersion() != 36)
  {
    IOS_ReloadIOS(36);
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

  const int fd = IOS_Open("/dev/usb/oh0", IPC_OPEN_NONE);
  if (fd < 0)
  {
    network_printf("Failed to open /dev/usb/oh0: ret %d\n", fd);
    return 1;
  }

  TestGETDEVLIST(fd);
  TestInsertHook(fd);

  const int devicefd = IOS_Open("/dev/usb/oh0/46d/a03", IPC_OPEN_NONE);
  if (fd < 0)
  {
    network_printf("Failed to open /dev/usb/oh0/46d/a03: ret %d\n", fd);
    return 1;
  }
  TestSetAlternate(devicefd);

  IOS_Close(fd);
  IOS_Close(devicefd);

  network_printf("Shutting down...\n");
  network_shutdown();
  return 0;
}
