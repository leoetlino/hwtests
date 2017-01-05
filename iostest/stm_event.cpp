#include <ogc/ios.h>
#include <ogc/ipc.h>
#include <ogc/stm.h>
#include <unistd.h>

#include "hwtests.h"
#include "CommonTypes.h"
#include "StringUtil.h"

static s32 hId = -1;

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

  __STM_Close();
  const s32 stm_eh = IOS_Open("/dev/stm/eventhook", 0);
  if (stm_eh < 0)
  {
    network_printf("Failed to open eventhook device!\n");
    return 1;
  }
  for (int i = 0; i < 5; ++i)
  {
    u8 in_buffer[32] = {};
    u8 out_buffer[32] = {};
    network_printf("Setting event hook\n");
    const s32 ret = IOS_Ioctl(stm_eh, 0x1000, in_buffer, sizeof(in_buffer), out_buffer,
                              sizeof(out_buffer));
    network_printf("IOS_Ioctl returned %d\n", ret);
    if (ret >= 0)
      network_printf("Buffer: %s", ArrayToString(out_buffer, sizeof(out_buffer), 32).c_str());
  }
  IOS_Close(stm_eh);

  network_printf("Shutting down...\n");
  network_shutdown();
  return 0;
}
