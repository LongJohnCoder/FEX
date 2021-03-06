#include "Interface/HLE/Syscalls.h"
#include "Interface/HLE/Syscalls/Thread.h"
#include "Interface/HLE/x64/Syscalls.h"
#include "Interface/HLE/x64/Thread.h"

#include "Interface/Context/Context.h"
#include "Interface/Core/InternalThreadState.h"
#include "Interface/HLE/Syscalls.h"

#include <stdint.h>
#include <linux/futex.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>

namespace FEXCore::HLE::x64 {
  uint64_t SetThreadArea(FEXCore::Core::InternalThreadState *Thread, void *tls) {
    Thread->State.State.fs = reinterpret_cast<uint64_t>(tls);
    return 0;
  }

  void AdjustRipForNewThread(FEXCore::Core::CPUState *Thread) {
    Thread->rip += 2;
  }

  static bool AnyFlagsSet(uint64_t Flags, uint64_t Mask) {
    return (Flags & Mask) != 0;
  }

  static bool AllFlagsSet(uint64_t Flags, uint64_t Mask) {
    return (Flags & Mask) == Mask;
  }

  void RegisterThread() {
    REGISTER_SYSCALL_IMPL_X64(futex, [](FEXCore::Core::InternalThreadState *Thread, int *uaddr, int futex_op, int val, const struct timespec *timeout, int *uaddr2, uint32_t val3) -> uint64_t {
      uint64_t Result = syscall(SYS_futex,
        uaddr,
        futex_op,
        val,
        timeout,
        uaddr2,
        val3);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(clone, [](FEXCore::Core::InternalThreadState *Thread, uint32_t flags, void *stack, pid_t *parent_tid, pid_t *child_tid, void *tls) -> uint64_t {
    #define FLAGPRINT(x, y) if (flags & (y)) LogMan::Msg::I("\tFlag: " #x)
      FLAGPRINT(CSIGNAL,              0x000000FF);
      FLAGPRINT(CLONE_VM,             0x00000100);
      FLAGPRINT(CLONE_FS,             0x00000200);
      FLAGPRINT(CLONE_FILES,          0x00000400);
      FLAGPRINT(CLONE_SIGHAND,        0x00000800);
      FLAGPRINT(CLONE_PTRACE,         0x00002000);
      FLAGPRINT(CLONE_VFORK,          0x00004000);
      FLAGPRINT(CLONE_PARENT,         0x00008000);
      FLAGPRINT(CLONE_THREAD,         0x00010000);
      FLAGPRINT(CLONE_NEWNS,          0x00020000);
      FLAGPRINT(CLONE_SYSVSEM,        0x00040000);
      FLAGPRINT(CLONE_SETTLS,         0x00080000);
      FLAGPRINT(CLONE_PARENT_SETTID,  0x00100000);
      FLAGPRINT(CLONE_CHILD_CLEARTID, 0x00200000);
      FLAGPRINT(CLONE_DETACHED,       0x00400000);
      FLAGPRINT(CLONE_UNTRACED,       0x00800000);
      FLAGPRINT(CLONE_CHILD_SETTID,   0x01000000);
      FLAGPRINT(CLONE_NEWCGROUP,      0x02000000);
      FLAGPRINT(CLONE_NEWUTS,         0x04000000);
      FLAGPRINT(CLONE_NEWIPC,         0x08000000);
      FLAGPRINT(CLONE_NEWUSER,        0x10000000);
      FLAGPRINT(CLONE_NEWPID,         0x20000000);
      FLAGPRINT(CLONE_NEWNET,         0x40000000);
      FLAGPRINT(CLONE_IO,             0x80000000);

      if (AnyFlagsSet(flags, CLONE_UNTRACED | CLONE_PTRACE)) {
        LogMan::Msg::D("clone: Ptrace* not supported");
      }

      if (AnyFlagsSet(flags, CLONE_NEWNS | CLONE_NEWCGROUP | CLONE_NEWUTS | CLONE_NEWIPC | CLONE_NEWUSER | CLONE_NEWPID | CLONE_NEWNET)) {
        ERROR_AND_DIE("clone: Namespaces are not supported");
      }

      if (!(flags & CLONE_THREAD)) {

        if (flags & CLONE_VFORK) {
          flags &= ~CLONE_VFORK;
          flags &= ~CLONE_VM;
          LogMan::Msg::D("clone: WARNING: CLONE_VFORK w/o CLONE_THREAD");
        }

        if (AnyFlagsSet(flags, CLONE_SYSVSEM | CLONE_FS |  CLONE_FILES | CLONE_SIGHAND | CLONE_VM)) {
          ERROR_AND_DIE("clone: Unsuported flags w/o CLONE_THREAD (Shared Resources), %X", flags);
        }

        // CLONE_PARENT is ignored (Implied by CLONE_THREAD)

        if (Thread->CTX->GetThreadCount() != 1) {
          LogMan::Msg::E("clone: Fork only supported on single threaded applications. Allowing");
        } else {
          LogMan::Msg::D("clone: Forking process");
        }

        return ForkGuest(Thread, flags, stack, parent_tid, child_tid, tls);
      } else {

        if (!AllFlagsSet(flags, CLONE_SYSVSEM | CLONE_FS |  CLONE_FILES | CLONE_SIGHAND)) {
          ERROR_AND_DIE("clone: CLONE_THREAD: Unsuported flags w/ CLONE_THREAD (Shared Resources), %X", flags);
        }

        auto NewThread = CreateNewThread(Thread, flags, stack, parent_tid, child_tid, tls);

        // Return the new threads TID
        uint64_t Result = NewThread->State.ThreadManager.GetTID();
        LogMan::Msg::D("Child [%d] starting at: 0x%lx. Parent was at 0x%lx", Result, NewThread->State.State.rip, Thread->State.State.rip);

        // Actually start the thread
        Thread->CTX->RunThread(NewThread);

        if (flags & CLONE_VFORK) {
          // If VFORK is set then the calling process is suspended until the thread exits with execve or exit
          NewThread->ExecutionThread.join();
        }
        SYSCALL_ERRNO();
      }
    });

    REGISTER_SYSCALL_IMPL_X64(set_robust_list, [](FEXCore::Core::InternalThreadState *Thread, struct robust_list_head *head, size_t len) -> uint64_t {
      Thread->State.ThreadManager.robust_list_head = reinterpret_cast<uint64_t>(head);
      uint64_t Result = ::syscall(SYS_set_robust_list, head, len);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(get_robust_list, [](FEXCore::Core::InternalThreadState *Thread, int pid, struct robust_list_head **head, size_t *len_ptr) -> uint64_t {
      uint64_t Result = ::syscall(SYS_get_robust_list, pid, head, len_ptr);
      SYSCALL_ERRNO();
    });
  }
}
