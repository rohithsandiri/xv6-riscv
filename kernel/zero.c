// /dev/zero: reads return an endless stream of zero bytes, writes
// are silently discarded. Registered as devsw[ZERO], the same
// major-number dispatch mechanism the console uses.

#include "types.h"
#include "fcntl.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "riscv.h"
#include "defs.h"

#define ZEROBUFSZ 512
static char zerobuf[ZEROBUFSZ]; // always all-zero; never written to

int
zeroread(int user_dst, uint64 dst, int n)
{
  int tot, m;

  for (tot = 0; tot < n; tot += m) {
    m = n - tot;
    if (m > ZEROBUFSZ)
      m = ZEROBUFSZ;
    // either_copyout: dst is a user address when user_dst==1, a
    // kernel address otherwise -- same convention readi() uses.
    if (either_copyout(user_dst, dst + tot, zerobuf, m) == -1)
      break;
  }
  return tot;
}

int
zerowrite(int user_src, uint64 src, int n)
{
  // Discard: the bytes are never looked at, but n is still the
  // correct return value -- write() callers expect the number of
  // bytes "written" back, not 0 (which would look like EOF/retry).
  return n;
}

void
zeroinit(void)
{
  devsw[ZERO].read = zeroread;
  devsw[ZERO].write = zerowrite;
}
