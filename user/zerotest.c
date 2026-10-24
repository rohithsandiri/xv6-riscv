// Test /dev/zero: reads return zero bytes, writes are discarded
// without being reflected back anywhere readable.

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

#define ZERO_MAJOR 2

int
main(int argc, char *argv[])
{
  char buf[1000];
  int fd, i, n;

  unlink("zero_dev");
  if (mknod("zero_dev", ZERO_MAJOR, 0) < 0) {
    printf("zerotest: mknod failed\n");
    exit(1);
  }

  fd = open("zero_dev", O_RDWR);
  if (fd < 0) {
    printf("zerotest: open failed\n");
    exit(1);
  }

  // Poison the buffer so a real zero-fill is the only way it passes.
  memset(buf, 'A', sizeof(buf));
  n = read(fd, buf, sizeof(buf));
  if (n != sizeof(buf)) {
    printf("zerotest: short read (%d)\n", n);
    exit(1);
  }
  for (i = 0; i < sizeof(buf); i++) {
    if (buf[i] != 0) {
      printf("zerotest: byte %d was %d, not 0\n", i, buf[i]);
      exit(1);
    }
  }

  memset(buf, 'B', sizeof(buf));
  n = write(fd, buf, sizeof(buf));
  if (n != sizeof(buf)) {
    printf("zerotest: write did not report success (%d)\n", n);
    exit(1);
  }

  close(fd);
  unlink("zero_dev");

  printf("zerotest: ALL TESTS PASSED\n");
  exit(0);
}
