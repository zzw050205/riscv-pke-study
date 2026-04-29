/*
 * contains the implementation of all syscalls.
 */

#include <errno.h>
#include <stdint.h>

#include "config.h"
#include "elf.h"
#include "process.h"
#include "string.h"
#include "sync_utils.h"
#include "syscall.h"
#include "util/types.h"

#include "spike_interface/spike_utils.h"

extern volatile int sstart_barrier;
//
// implement the SYS_user_print syscall
//
ssize_t sys_user_print(const char *buf, size_t n) {
  uint64 hartid = read_tp();

  if (hartid == 0) {
    while (sstart_barrier < 2)
      ; // Wait for CPU1 boot logs
  } else {
    while (sstart_barrier < 3)
      ; // Wait for CPU0 app output
  }

  sprint("hartid = %ld: %s", hartid, buf);

  if (hartid == 0) {
    asm volatile("fence rw, rw" ::: "memory");
    sstart_barrier = 3; // Stage 3: CPU0 finished app0 print
  } else {
    asm volatile("fence rw, rw" ::: "memory");
    sstart_barrier = 4; // Stage 4: CPU1 finished app1 print
  }
  return 0;
}

volatile int exit_harts = 0;
volatile int user_app_barrier = 0;
//
// implement the SYS_user_exit syscall
//
ssize_t sys_user_exit(uint64 code) {
  uint64 hartid = read_tp();

  if (hartid == 0) {
    while (sstart_barrier < 4)
      ; // Wait for CPU1 app output
  } else {
    while (sstart_barrier < 5)
      ; // Wait for CPU0 exit log
  }

  sprint("hartid = %ld: User exit with code:%d.\n", hartid, code);

  // After CPU0 exits, allow CPU1 to proceed with its entry logs
  if (hartid == 0) {
    asm volatile("fence rw, rw" ::: "memory");
    sstart_barrier = 5; // Stage 5: CPU0 finished exit log
  } else {
    asm volatile("fence rw, rw" ::: "memory");
    sstart_barrier = 6; // Stage 6: CPU1 finished exit log
  }

  // in lab1, PKE considers only one app (one process).
  // therefore, shutdown the system when the app calls exit()
  sync_barrier(&exit_harts, NCPU);
  if (hartid == 0) {
    while (sstart_barrier < 6)
      ; // Ensure CPU1 exit log is printed
    sprint("hartid = %ld: shutdown with code:%d.\n", hartid, code);
    shutdown(code);
  }
  return 0;
}

//
// implement the SYS_user_print_backtrace syscall
//
void sys_user_print_backtrace() {
  uint64 hartid = read_tp();
  process *p = current[hartid];
  uint64 s0 = p->trapframe->regs.s0;
  uint64 ra;

  elf_ctx elfloader;
  elf_info info;
  info.f = spike_file_open(p->name, O_RDONLY, 0);
  info.p = p;
  if (IS_ERR_VALUE(info.f))
    return;

  if (elf_init(&elfloader, &info) != EL_OK) {
    spike_file_close(info.f);
    return;
  }

  sprint("hartid = %ld: Runtime backtrace:\n", hartid);
  while (s0 > USER_STACK && s0 < USER_STACK + 0x10000) {
    ra = *(uint64 *)(s0 - 8);
    s0 = *(uint64 *)(s0 - 16);
    char func_name[64];
    if (elf_find_symbol_by_addr(&elfloader, ra - 4, func_name) == EL_OK)
      sprint("hartid = %ld:   %s\n", hartid, func_name);
    else
      sprint("hartid = %ld:   [0x%lx]\n", hartid, ra - 4);
  }

  spike_file_close(info.f);
}

//
// [a0]: the syscall number; [a1] ... [a7]: arguments to the syscalls.
// returns the code of success, (e.g., 0 means success, fail for otherwise)
//
long do_syscall(long a0, long a1, long a2, long a3, long a4, long a5, long a6,
                long a7) {
  switch (a0) {
  case SYS_user_print:
    return sys_user_print((const char *)a1, a2);
  case SYS_user_exit:
    return sys_user_exit(a1);
  case SYS_user_print_backtrace:
    sys_user_print_backtrace();
    return 0;
  default:
    panic("Unknown syscall %ld \n", a0);
  }
}
