// Test symlink(2): create, follow, O_NOFOLLOW, and cycle detection.

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

void
basic(void)
{
  int fd;
  char buf[16];

  unlink("symtarget");
  unlink("symlink1");

  fd = open("symtarget", O_CREATE | O_RDWR);
  if (fd < 0) {
    printf("symlinktest: create symtarget failed\n");
    exit(1);
  }
  if (write(fd, "hello", 5) != 5) {
    printf("symlinktest: write symtarget failed\n");
    exit(1);
  }
  close(fd);

  if (symlink("symtarget", "symlink1") != 0) {
    printf("symlinktest: symlink failed\n");
    exit(1);
  }

  fd = open("symlink1", O_RDONLY);
  if (fd < 0) {
    printf("symlinktest: open through symlink failed\n");
    exit(1);
  }
  memset(buf, 0, sizeof(buf));
  if (read(fd, buf, 5) != 5 || strcmp(buf, "hello") != 0) {
    printf("symlinktest: read through symlink got wrong data\n");
    exit(1);
  }
  close(fd);

  // O_NOFOLLOW should open the symlink itself, not the target: its
  // content is the target path string, not "hello".
  fd = open("symlink1", O_RDONLY | O_NOFOLLOW);
  if (fd < 0) {
    printf("symlinktest: O_NOFOLLOW open failed\n");
    exit(1);
  }
  memset(buf, 0, sizeof(buf));
  read(fd, buf, sizeof(buf) - 1);
  if (strcmp(buf, "symtarget") != 0) {
    printf("symlinktest: O_NOFOLLOW returned wrong content: %s\n", buf);
    exit(1);
  }
  close(fd);

  unlink("symlink1");
  unlink("symtarget");
  printf("symlinktest: basic ok\n");
}

void
cycle(void)
{
  unlink("cyc1");
  unlink("cyc2");

  if (symlink("cyc2", "cyc1") != 0 || symlink("cyc1", "cyc2") != 0) {
    printf("symlinktest: creating cycle failed\n");
    exit(1);
  }

  int fd = open("cyc1", O_RDONLY);
  if (fd >= 0) {
    printf("symlinktest: open of symlink cycle should have failed\n");
    exit(1);
  }

  unlink("cyc1");
  unlink("cyc2");
  printf("symlinktest: cycle ok\n");
}

int
main(int argc, char *argv[])
{
  basic();
  cycle();
  printf("symlinktest: ALL TESTS PASSED\n");
  exit(0);
}
