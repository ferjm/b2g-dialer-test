#define HAVE_SYS_SOCKET_H 1
#define HAVE_SYS_UIO_H
#include <arpa/inet.h>
#include <cutils/properties.h>
#include <cutils/sockets.h>
#include <binder/Parcel.h>
#include <telephony/ril.h>
#include <linux/prctl.h>
#include <pwd.h>

static int
blockingWrite(int fd, const void *buffer, size_t len) {
    size_t writeOffset = 0;
    const uint8_t *toWrite;

    toWrite = (const uint8_t *)buffer;

    while (writeOffset < len) {
        ssize_t written;
        do {
            written = write (fd, toWrite + writeOffset,
                                len - writeOffset);
        } while (written < 0 && errno == EINTR);

        if (written >= 0) {
            writeOffset += written;
        } else {   // written < 0
            LOGE ("RIL Response: unexpected error on write errno:%d", errno);
            close(fd);
            return -1;
        }
    }

    return 0;
}

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


int main(int argc, char **argv) {

#define  QEMUD_SOCKET_NAME    "rild"

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


	int done;
	int tries = 10;
	int  fd;	
	int ret;
	printf("Connecting to socket %s\n", QEMUD_SOCKET_NAME);
	fd = socket_local_client(
		QEMUD_SOCKET_NAME,
		ANDROID_SOCKET_NAMESPACE_RESERVED,
		SOCK_STREAM );

	if (fd < 0) {
		LOGD("could not connect to %s socket: %s\n",
			 QEMUD_SOCKET_NAME, strerror(errno));
		return 1;
	}
	printf("connected to socket %s\n", QEMUD_SOCKET_NAME);

	char data[1024];
	
	ret = read(fd, data, 1024);
	printf("Read %d\n", ret);
	ret = read(fd, data, 1024);
	printf("Read %d\n", ret);
	
	android::Parcel p;
	p.writeInt32 (61);
	p.writeInt32 (32);
	p.writeInt32 (1);
	p.writeInt32 (1);

	uint32_t header = htonl(p.dataSize());
	printf("Writing header\n");
    ret = blockingWrite(fd, (void *)&header, sizeof(header));
	printf("Wrote header %d\n", ret);
    ret = blockingWrite(fd, (void *)p.data(), p.dataSize());
	printf("Wrote data %d %d\n", p.dataSize(), ret);

	// char buf[1000];
	// ret = read(fd, buf, 4);
	// printf("Read %d\n", buf[0]);
	
	return 0;
}
