#pragma once
#include <functional>
#include <stdint.h>

namespace FEXCore {
  class CodeLoader;
}

namespace FEXCore::Core {
  struct CPUState;
  struct ThreadState;
}

namespace FEXCore::CPU {
  class CPUBackend;
}

namespace FEXCore::HLE {
  struct SyscallArguments;
  class SyscallVisitor;
}
namespace FEXCore::SHM {
  struct SHMObject;
}

namespace FEXCore::Context {
  struct Context;
  enum ExitReason {
    EXIT_NONE,
    EXIT_WAITING,
    EXIT_ASYNC_RUN,
    EXIT_SHUTDOWN,
    EXIT_DEBUG,
    EXIT_UNKNOWNERROR,
  };
  using CustomCPUFactoryType = std::function<FEXCore::CPU::CPUBackend* (FEXCore::Context::Context*, FEXCore::Core::ThreadState *Thread)>;

  /**
   * @brief This initializes internal FEXCore state that is shared between contexts and requires overhead to setup
   */
  void InitializeStaticTables();

  /**
   * @brief [[threadsafe]] Create a new FEXCore context object
   *
   * This is necessary to do when running threaded contexts
   *
   * @return a new context object
   */
  FEXCore::Context::Context *CreateNewContext();

  /**
   * @brief Post creation context initialization
   * Once configurations have been set, do the post-creation initialization with that configuration
   *
   * @param CTX The context that we created
   *
   * @return true if we managed to initialize correctly
   */
  bool InitializeContext(FEXCore::Context::Context *CTX);

  /**
   * @brief Destroy the context object
   *
   * @param CTX
   */
  void DestroyContext(FEXCore::Context::Context *CTX);

  /**
   * @brief Adds a base pointer that the VM can use for "physical" memory backing
   *
   * Will be the guests physical memory location of zero
   *
   * @return true on added. false when we had already added a guest memory region
   */
  bool AddGuestMemoryRegion(FEXCore::Context::Context *CTX, FEXCore::SHM::SHMObject *SHM);

  /**
   * @brief Allows setting up in memory code and other things prior to launchign code execution
   *
   * @param CTX The context that we created
   * @param Loader The loader that will be doing all the code loading
   *
   * @return true if we loaded code
   */
  bool InitCore(FEXCore::Context::Context *CTX, FEXCore::CodeLoader *Loader);

  void SetApplicationFile(FEXCore::Context::Context *CTX, std::string const &File);

  /**
   * @brief Starts running the CPU core
   *
   * If WaitForIdle is enabled then this call will block until the thread exits or if single stepping is enabled, after the core steps one instruction
   *
   * @param CTX The context that we created
   * @param WaitForIdle Should we wait for the core to be idle or not
   *
   * @return The ExitReason for the parentthread. ASYNC_RUN if WaitForIdle was false
   */
  ExitReason RunLoop(FEXCore::Context::Context *CTX, bool WaitForIdle);

  /**
   * @brief [[threadsafe]] Returns the ExitReason of the parent thread. Typically used for async result status
   *
   * @param CTX The context that we created
   *
   * @return The ExitReason for the parentthread
   */
  ExitReason GetExitReason(FEXCore::Context::Context *CTX);

  /**
   * @brief [[theadsafe]] Checks if the Context is either done working or paused(in the case of single stepping)
   *
   * Use this when the context is async running to determine if it is done
   *
   * @param CTX the context that we created
   *
   * @return true if the core is done or paused
   */
  bool IsDone(FEXCore::Context::Context *CTX);

  /**
   * @brief Gets a copy the CPUState of the parent thread
   *
   * @param CTX The context that we created
   * @param State The state object to populate
   */
  void GetCPUState(FEXCore::Context::Context *CTX, FEXCore::Core::CPUState *State);

  /**
   * @brief Copies the CPUState provided to the parent thread
   *
   * @param CTX The context that we created
   * @param State The satate object to copy from
   */
  void SetCPUState(FEXCore::Context::Context *CTX, FEXCore::Core::CPUState *State);

  void Pause(FEXCore::Context::Context *CTX);

  /**
   * @brief Allows the frontend to pass in a custom CPUBackend creation factory
   *
   * This allows the frontend to have its own frontend. Typically for debugging
   *
   * @param CTX The context that we created
   * @param Factory The factory that the context will call if the DefaultCore config ise set to CUSTOM
   */
  void SetCustomCPUBackendFactory(FEXCore::Context::Context *CTX, CustomCPUFactoryType Factory);

  /**
   * @brief Allows a custom CPUBackend creation factory for fallback routines when the main CPUBackend core can't handle an instruction
   *
   * This is only useful for debugging new instruction decodings that FEXCore doesn't understand
   * The CPUBackend that is created from this factory must have its NeedsOpDispatch function to return false
   *
   * @param CTX The context that we created
   * @param Factory The factory that the context will call on core creation
   */
  void SetFallbackCPUBackendFactory(FEXCore::Context::Context *CTX, CustomCPUFactoryType Factory);

  /**
   * @brief This allows a frontend core to call Syscall routines directly. Useful for debugging
   *
   * @param CTX The context that we created
   * @param Thread The thread to run the syscall on
   * @param Args The arguments to the syscall
   *
   * @return The value that a syscall returns
   */
  uint64_t HandleSyscall(FEXCore::Context::Context *CTX, FEXCore::Core::ThreadState *Thread, FEXCore::HLE::SyscallArguments *Args);

  /**
   * @brief Sets up memory regions on the guest for mirroring within the guest's VM space
   *
   * @param VirtualAddress The address we want to set to mirror a physical memory region
   * @param PhysicalAddress The physical memory region we are mapping
   * @param Size Size of the region to mirror
   *
   * @return true when successfully mapped. false if there was an error adding
   */
  bool AddVirtualMemoryMapping(FEXCore::Context::Context *CTX, uint64_t VirtualAddress, uint64_t PhysicalAddress, uint64_t Size);

  /**
   * @brief Allows the frontend to set a custom syscall handler
   *
   * Useful for debugging purposes. May not work if the syscall ID exceeds the maximum number of syscalls in the lookup table
   *
   * @param Syscall Which syscall ID to install a visitor to
   * @param Visitor The Visitor to install
   */
  void RegisterExternalSyscallVisitor(FEXCore::Context::Context *CTX, uint64_t Syscall, FEXCore::HLE::SyscallVisitor *Visitor);

}