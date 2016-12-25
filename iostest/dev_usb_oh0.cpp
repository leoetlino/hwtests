#include <atomic>
#include <cstring>
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
  network_printf("USBV0_IOCTL_GETDEVLIST\n");

  u8 cntdevs = 0;
  u8 num_descr = 5;
  const u8 audio_interface_class = 0;
  std::vector<u32> buf(num_descr * 8);

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

  u16 vid = 0x57e;
  u16 pid = 0x305;

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

void TestUnknown15(const s32 fd)
{
  network_printf("Testing USBV0_IOCTL_UNKNOWN_15 (ioctl)\n");

  u8 idata[4] = {0xde,0xad,0xbe,0xef};
  auto data = (u8*)iosAlloc(hId, sizeof(idata));
  std::memcpy(idata, &data, sizeof(data));

  const s32 ret = IOS_Ioctl(fd, 15, nullptr, 0, data, sizeof(idata));
  network_printf("ret = %d\n", ret);
  if (ret < 0)
    return;

  network_printf("buffer:\n%s\n", ArrayToString(data, sizeof(data)).c_str());
}

void TestGetRhPortStatus20(const s32 fd)
{
  network_printf("Testing USBV0_IOCTLV_GETRHPORTSTATUS\n");

  u8 input = 0x1;
  u8 data[4] = {};

  const s32 ret = IOS_IoctlvFormat(hId, fd, 20, "b:d", input, data, 4);
  network_printf("ret = %d\n", ret);
  if (ret < 0)
    return;

  network_printf("buffer:\n%s\n", ArrayToString(data, sizeof(data)).c_str());
}

static std::atomic<s32> s_callback_result{-99};

s32 DevInsertHookIdCallback(const s32 result, void* userdata)
{
  s_callback_result.store(result);
  return IPC_OK;
}

s32 TestUnknown30(const s32 fd)
{
  network_printf("Testing USBV0_IOCTLV_DEVINSERTHOOKID\n");

  u16 vid = 0x057e;
  u16 pid = 0x0308;
  u8 want_new_device = 1;
  u32 id = 0x12345678;

  auto vec = (ioctlv*)iosAlloc(hId, sizeof(ioctlv) * 4);
  auto pVid = (u16*)iosAlloc(hId, sizeof(vid));
  *pVid = vid;
  auto pPid = (u16*)iosAlloc(hId, sizeof(pid));
  *pPid = pid;
  auto pWantNew = (u8*)iosAlloc(hId, sizeof(want_new_device));
  *pWantNew = want_new_device;
  auto pId = (u32*)iosAlloc(hId, sizeof(id));
  *pId = id;

  vec[0].data = pVid;
  vec[0].len = sizeof(vid);
  vec[1].data = pPid;
  vec[1].len = sizeof(pid);
  vec[2].data = pWantNew;
  vec[2].len = sizeof(want_new_device);
  vec[3].data = pId;
  vec[3].len = sizeof(id);

  const s32 ret = IOS_IoctlvAsync(fd, 30, 3, 1, vec, DevInsertHookIdCallback, nullptr);
  network_printf("ret = %d\n", ret);
  return *pId;
}

void TestUnknown31(const s32 fd, s32 hook_id)
{
  network_printf("Testing USBV0_IOCTL_CANCEL_INSERT_HOOK\n");

  const s32 ret = IOS_Ioctl(fd, 31, &hook_id, sizeof(hook_id), nullptr, 0);
  network_printf("IOS_Ioctl(fd, 31, ...) = = %d\n", ret);
  while (s_callback_result.load() == -99)
  {
    usleep(50000);
  }
  network_printf("callback_result = %d\n", s_callback_result.load());
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

  const s32 fd = IOS_Open("/dev/usb/oh0", IPC_OPEN_NONE);
  network_printf("IOS_Open(path=/dev/usb/oh0) = %d\n", fd);
  if (fd < 0)
    return 1;

  TestGETDEVLIST(fd);
  // TestInsertHook(fd);
  TestUnknown15(fd);
  TestGetRhPortStatus20(fd);
  const s32 hook_id = TestUnknown30(fd);
  TestUnknown31(fd, 0);

  // const s32 devicefd = IOS_Open("/dev/usb/oh0/57e/308", IPC_OPEN_NONE);
  // network_printf("IOS_Open() = %d\n", devicefd);
  // if (devicefd < 0)
    // return 1;
  // TestSetAlternate(devicefd);

  IOS_Close(fd);
  // IOS_Close(devicefd);

  network_printf("Shutting down...\n");
  network_shutdown();
  return 0;
}
