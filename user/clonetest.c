// Layer 1 checkpoint: clone() shares the parent's address space.
// Raw syscall only -- no thread library yet (that's Layer 4) -- so
// clone() is used exactly like fork(): it returns twice, 0 in the
// new thread (running on the stack we handed it) and the new pid in
// the caller.

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define STACKSIZE 4096

int shared_global = 0;
char thread_stack[STACKSIZE] __attribute__((aligned(16)));

int
main(int argc, char *argv[])
{
  int pid = clone(thread_stack + STACKSIZE);
  if (pid < 0) {
    printf("clonetest: clone failed\n");
    exit(1);
  }

  if (pid == 0) {
    // New thread: same address space as the parent, so this write
    // lands in the one shared copy of shared_global, not a private
    // COW copy the way a fork()ed child's write would.
    shared_global = 42;
    exit(0);
  }

  if (join(pid) != pid) {
    printf("clonetest: join failed\n");
    exit(1);
  }

  if (shared_global != 42) {
    printf("clonetest: parent saw %d, expected 42 -- address space was not "
           "shared\n",
           shared_global);
    exit(1);
  }

  printf("clonetest: ALL TESTS PASSED\n");
  exit(0);
}
