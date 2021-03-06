#include "Interface/HLE/Syscalls.h"
#include "Interface/HLE/x64/Syscalls.h"
#include "Interface/HLE/x32/Syscalls.h"

#include <stddef.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/timex.h>
#include <unistd.h>
#include <utime.h>

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore::HLE {
  void RegisterTime() {
    REGISTER_SYSCALL_IMPL(pause, [](FEXCore::Core::InternalThreadState *Thread) -> uint64_t {
      uint64_t Result = ::pause();
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(time, [](FEXCore::Core::InternalThreadState *Thread, time_t *tloc) -> uint64_t {
      uint64_t Result = ::time(tloc);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(times, [](FEXCore::Core::InternalThreadState *Thread, struct tms *buf) -> uint64_t {
      uint64_t Result = ::times(buf);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(utime, [](FEXCore::Core::InternalThreadState *Thread, char* filename, const struct utimbuf* times) -> uint64_t {
      uint64_t Result = ::utime(filename, times);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(adjtimex, [](FEXCore::Core::InternalThreadState *Thread, struct timex *buf) -> uint64_t {
      uint64_t Result = ::adjtimex(buf);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(clock_adjtime, [](FEXCore::Core::InternalThreadState *Thread, clockid_t clk_id, struct timex *buf) -> uint64_t {
      uint64_t Result = ::clock_adjtime(clk_id, buf);
      SYSCALL_ERRNO();
    });
  }
}
