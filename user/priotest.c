// Test the priority scheduler: fork children with different
// priorities, let them spin for a fixed wall-clock duration, and
// compare how many loop iterations each one managed. Higher
// priority should win proportionally more CPU time.

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// QEMU runs with -smp 3 (3 real cores). To see priority actually
// matter, we need more RUNNABLE processes than cores, so they're
// forced to compete for a CPU instead of each getting its own.
#define NCHILD 6
#define DURATION_TICKS 40

// Mirrors MIN_PRIORITY/DEFAULT_PRIORITY/MAX_PRIORITY from kernel/proc.h.
int priorities[NCHILD] = {1, 1, 10, 10, 20, 20};

int
main(int argc, char *argv[])
{
  int pipes[NCHILD][2];
  int i;

  for (i = 0; i < NCHILD; i++) {
    if (pipe(pipes[i]) < 0) {
      printf("priotest: pipe failed\n");
      exit(1);
    }
  }

  int start = uptime();

  for (i = 0; i < NCHILD; i++) {
    int pid = fork();
    if (pid < 0) {
      printf("priotest: fork failed\n");
      exit(1);
    }
    if (pid == 0) {
      close(pipes[i][0]);
      nice(getpid(), priorities[i]);

      long count = 0;
      while (uptime() - start < DURATION_TICKS) {
        count++;
      }

      write(pipes[i][1], &count, sizeof(count));
      close(pipes[i][1]);
      exit(0);
    }
    close(pipes[i][1]);
  }

  for (i = 0; i < NCHILD; i++) {
    long count = 0;
    read(pipes[i][0], &count, sizeof(count));
    close(pipes[i][0]);
    printf("priority %d: %ld iterations\n", priorities[i], count);
    wait(0);
  }

  exit(0);
}
