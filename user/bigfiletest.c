// Test that file addressing correctly crosses from direct blocks,
// through the singly-indirect block, into the doubly-indirect block.
// Writes a few thousand blocks (well past NDIRECT+NINDIRECT=267)
// rather than the full MAXFILE, so the test finishes quickly.

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

#define BSIZE 1024
#define NBLOCKS 2000 // > NDIRECT + NINDIRECT (267): exercises the doubly-indirect path

int
main(int argc, char *argv[])
{
  int fd, i, n;
  char buf[BSIZE];

  unlink("bigfile.dat");
  fd = open("bigfile.dat", O_CREATE | O_RDWR);
  if (fd < 0) {
    printf("bigfiletest: create failed\n");
    exit(1);
  }

  for (i = 0; i < NBLOCKS; i++) {
    memset(buf, i % 256, BSIZE);
    if (write(fd, buf, BSIZE) != BSIZE) {
      printf("bigfiletest: write failed at block %d\n", i);
      exit(1);
    }
  }
  close(fd);

  fd = open("bigfile.dat", O_RDONLY);
  if (fd < 0) {
    printf("bigfiletest: reopen failed\n");
    exit(1);
  }
  for (i = 0; i < NBLOCKS; i++) {
    n = read(fd, buf, BSIZE);
    if (n != BSIZE) {
      printf("bigfiletest: short read at block %d (%d)\n", i, n);
      exit(1);
    }
    if (buf[0] != (char)(i % 256) || buf[BSIZE - 1] != (char)(i % 256)) {
      printf("bigfiletest: wrong data at block %d\n", i);
      exit(1);
    }
  }
  if (read(fd, buf, BSIZE) != 0) {
    printf("bigfiletest: file longer than expected\n");
    exit(1);
  }
  close(fd);
  unlink("bigfile.dat");

  printf("bigfiletest: ALL TESTS PASSED\n");
  exit(0);
}
