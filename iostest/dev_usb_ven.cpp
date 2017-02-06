#include <array>
#include <algorithm>
#include <cstring>
#include <vector>

#include <ogc/ios.h>
#include <ogc/ipc.h>
#include <ogc/usb.h>
#include <unistd.h>

#include "hwtests.h"
#include "CommonTypes.h"
#include "StringUtil.h"

#include "usb.h"

static s32 hId = -1;

void TestGETDEVPARAMS(const s32 fd, const s32 device_id)
{
  network_printf("USBV5_IOCTL_GETDEVPARAMS (device_id %x)\n", device_id);

  u32* in_buffer = (u32*)iosAlloc(hId, 0x20);
  u8* buffer = (u8*)iosAlloc(hId, 0xC0);
  if (!in_buffer || !buffer)
  {
    network_printf("failed to alloc buffer\n");
    return;
  }
  memset(buffer, 0, 0xC0);
  memset(in_buffer, 0, 0x20);
  in_buffer[0] = device_id;
  // in_buffer[2] = 0x02000001;

  network_printf("in buffer:\n%s", ArrayToString(reinterpret_cast<u8*>(in_buffer), 0x20, 4).c_str());

  const s32 ret = IOS_Ioctl(fd, 3, in_buffer, 0x20, buffer, 0xC0);
  if (ret < 0)
  {
    network_printf("ret = %d\n", ret);
    return;
  }

  network_printf("buffer:\n%s", ArrayToString(reinterpret_cast<u8*>(buffer), 0xc0, 4).c_str());
}

void TestDEVICECHANGE(const s32 fd)
{
  network_printf("ioctl USBV5_IOCTL_GETDEVICECHANGE\n");
  network_printf("note: this will block until a device is plugged in or removed,\n");
  network_printf("unless this is the first GETDEVICECHANGE ioctl\n");

  u8* buffer = (u8*)iosAlloc(hId, 0x180);
  if (!buffer)
  {
    network_printf("failed to alloc buffer\n");
    return;
  }
  memset(buffer, 0, 0x180);

  const s32 ret = IOS_Ioctl(fd, USBV5_IOCTL_GETDEVICECHANGE, nullptr, 0, buffer, 0x180);
  network_printf("ioctl(fd, USBV5_IOCTL_GETDEVICECHANGE) ret = %d\n", ret);
  if (ret < 0)
  {
    return;
  }

  network_printf("devices:\n%s", ArrayToString(reinterpret_cast<u8*>(buffer), 0xc0, sizeof(usb_device_entry)).c_str());
  iosFree(hId, buffer);

  network_printf("ioctl USBV5_IOCTL_ATTACHFINISH\n");
  const s32 ret2 = IOS_Ioctl(fd, USBV5_IOCTL_ATTACHFINISH, nullptr, 0, nullptr, 0);
  network_printf("ioctl(fd, USBV5_IOCTL_ATTACHFINISH) ret = %d\n", ret2);
}

void TestSETALTSETTING(const s32 fd, const s32 device_id)
{
  network_printf("Testing set_alt_setting\n");
  u32 data[8] = {(u32)device_id, 0x00000002, 0x01000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000};
  u32* in_buffer = (u32*)iosAlloc(hId, sizeof(data));
  std::memcpy(in_buffer, data, sizeof(data));
  const s32 ret = IOS_Ioctl(fd, USBV5_IOCTL_SETALTERNATE, in_buffer, sizeof(data), nullptr, 0);
  network_printf("IOS_Ioctl(fd, USBV5_IOCTL_SETALTERNATE, ...) = = %d\n", ret);
}

int main()
{
  if (IOS_GetVersion() != 58)
  {
    IOS_ReloadIOS(58);
    usleep(50000);
  }

  network_init();
  network_printf("Current IOS version: %d\n\n", IOS_GetVersion());

  hId = iosCreateHeap(16384);
  if (hId < 0)
  {
    network_printf("Failed to create heap\n");
    return IPC_ENOMEM;
  }

  auto* descr_buffer = static_cast<usb_device_entry*>(iosAlloc(hId, 8 * sizeof(usb_device_entry)));
  if (!descr_buffer) {
    network_printf("Failed to allocate buffer");
    return 1;
  }

  memset(descr_buffer, 0, 8 * sizeof(usb_device_entry));

  u8 cnt_descr;
  USB_Initialize();
  const s32 ret = USB_GetDeviceList(descr_buffer, 5 /* number of descriptors */, 8, &cnt_descr);
  if (ret < 0)
  {
    network_printf("failed to get list: %d\n", ret);
    return 1;
  }
  USB_Deinitialize();

  const int fd = IOS_Open("/dev/usb/ven", IPC_OPEN_NONE);
  if (fd < 0)
  {
    network_printf("Failed to open /dev/usb/ven\n");
    return 1;
  }
  u32* ven_ver = (u32*)iosAlloc(hId, 0x20);
  if (!ven_ver)
  {
    network_printf("failed to alloc ven_ver");
    return 1;
  }
  memset(ven_ver, 0, 0x20);
  if (IOS_Ioctl(fd, USBV5_IOCTL_GETVERSION, nullptr, 0, ven_ver, 0x20) != 0 || ven_ver[0] != 0x50001)
  {
    network_printf("IOCtl failed or wrong version\n");
    return 1;
  }
  iosFree(hId, ven_ver);

  for (int i = 0; i < cnt_descr; ++i)
  {
    const auto& entry = descr_buffer[i];
    network_printf("entry: device_id %x vid %x pid %x token %x\n", entry.device_id, entry.vid, entry.pid, entry.token);

    // Resume device
    s32* buf = (s32*)iosAlloc(hId, 32);
    if (!buf)
    {
      network_printf("failed to alloc buf to resume device\n");
      return 1;
    }
    buf[0] = entry.device_id;
    buf[2] = 1;
    const s32 resume_ret = IOS_Ioctl(fd, USBV5_IOCTL_SUSPEND_RESUME, buf, 32, nullptr, 0);
    network_printf("resume_ret = %d\n", resume_ret);
    iosFree(hId, buf);

    // TestGETDEVPARAMS(fd, entry.device_id);
    // TestSETALTSETTING(fd, entry.device_id);
    network_printf("\n");
  }
  //while (true);
  /*
    for (int i = 0; i < 5; ++i)
    {
      network_printf("TestDEVICECHANGE #%d\n", i);
      TestDEVICECHANGE(fd);
    }
  */
  IOS_Close(fd);

  network_printf("Shutting down...\n");
  network_shutdown();
  return 0;
}
