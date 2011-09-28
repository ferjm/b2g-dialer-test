#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <telephony/ril.h>
#include <cutils/properties.h>
#include <cutils/sockets.h>

#include <linux/capability.h>
#include <linux/prctl.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <pwd.h>

#define LOG_TAG "RILD"
#define SOCKET_NAME_RIL "rild"

/*
 * switchUser - Switches UID to radio, preserving CAP_NET_ADMIN capabilities.
 * Our group, cache, was set by init.
 */
void switchUser() 
{
    prctl(PR_SET_KEEPCAPS, 1, 0, 0, 0);
    setuid(1001);

    struct __user_cap_header_struct header;
    struct __user_cap_data_struct cap;
    header.version = _LINUX_CAPABILITY_VERSION;
    header.pid = 0;
    cap.effective = cap.permitted = 1 << CAP_NET_ADMIN;
    cap.inheritable = 0;
    capset(&header, &cap);
}

int main(int argc, char **argv)
{

  struct passwd *pwd = NULL;

  switchUser();

  pwd = getpwuid(getuid());
  if (pwd != NULL) {
    if (strcmp(pwd->pw_name, "radio") == 0) {
      printf("Radio.\n");
    } else {
      printf("No radio.\n");
    }
  } else {
    fprintf(stderr, "getpwuid error.");
  }
  
  int fd = socket(AF_LOCAL, SOCK_STREAM, 0);
  int err = socket_local_client_connect(fd, SOCKET_NAME_RIL,ANDROID_SOCKET_NAMESPACE_RESERVED,SOCK_STREAM);

  if(err < 0) fprintf(stderr, "Error: %i %s\n", fd, strerror(err));
  close(fd);

  return 0;
}
