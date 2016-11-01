#include <vector>

#include <ogc/ios.h>
#include <ogc/ipc.h>
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

  IOS_Close(fd);

  network_printf("Shutting down...\n");
  network_shutdown();
  return 0;
}
