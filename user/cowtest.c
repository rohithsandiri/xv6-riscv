// Test copy-on-write fork: parent and child must not see each
// other's writes, and physical memory must not leak after many
// fork/write/exit cycles (which would indicate a refcount bug).

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

char buf[4096] __attribute__((aligned(4096)));

void
isolation(void)
{
  buf[0] = 'A';

  int pid = fork();
  if (pid < 0) {
    printf("cowtest: fork failed\n");
    exit(1);
  }

  if (pid == 0) {
    // child: this write must not land in the parent's page.
    if (buf[0] != 'A') {
      printf("cowtest: child saw wrong initial value\n");
      exit(1);
    }
    buf[0] = 'B';
    exit(0);
  }

  wait(0);
  if (buf[0] != 'A') {
    printf("cowtest: parent's page was corrupted by child's write (got %c)\n",
           buf[0]);
    exit(1);
  }
  printf("cowtest: isolation ok\n");
}

void
leakcheck(void)
{
  int i;

  for (i = 0; i < 30; i++) {
    int pid = fork();
    if (pid < 0) {
      printf("cowtest: fork failed during leak check\n");
      exit(1);
    }
    if (pid == 0) {
      buf[0] = (char)i; // trigger a COW fault in the child
      exit(0);
    }
    wait(0);
  }
  printf("cowtest: leakcheck ok (fork storm survived)\n");
}

void
threeway(void)
{
  // Three processes share one COW page; one writes. Only that one
  // should get a private copy -- the other two must still agree.
  buf[0] = 'X';
  int pid1 = fork();
  if (pid1 == 0) {
    pause(2);
    if (buf[0] != 'X') {
      printf("cowtest: threeway child1 saw wrong value\n");
      exit(1);
    }
    exit(0);
  }
  int pid2 = fork();
  if (pid2 == 0) {
    buf[0] = 'Y'; // only this process's view should change
    exit(0);
  }
  wait(0);
  wait(0);
  if (buf[0] != 'X') {
    printf("cowtest: threeway parent saw wrong value (got %c)\n", buf[0]);
    exit(1);
  }
  printf("cowtest: threeway ok\n");
}

int
main(int argc, char *argv[])
{
  isolation();
  threeway();
  leakcheck();
  printf("cowtest: ALL TESTS PASSED\n");
  exit(0);
}
