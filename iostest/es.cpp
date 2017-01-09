#include <vector>

#include <ogc/es.h>
#include <ogc/ios.h>
#include <ogc/ipc.h>
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

  __ES_Close();
  std::vector<s32> es_fds;
  for (int i = 0; i < 5; ++i)
  {
    network_printf("Opening /dev/es [%d]: ", i);
    const s32 es_fd = IOS_Open("/dev/es", 0);
    if (es_fd < 0)
    {
      network_printf("failed! ret = %d\n", es_fd);
      break;
    }
    network_printf("success! fd = %d\n", es_fd);
    es_fds.push_back(es_fd);
  }
  for (const s32 fd_to_close : es_fds)
  {
    network_printf("Closing fd %d\n", fd_to_close);
    IOS_Close(fd_to_close);
  }
  es_fds.clear();
  __ES_Init();
  network_printf("Shutting down...\n");
  network_shutdown();
  return 0;
}
