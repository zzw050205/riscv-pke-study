#ifndef _SYNC_UTILS_H_
#define _SYNC_UTILS_H_

typedef volatile int spinlock_t __attribute__((aligned(4)));

static inline void spin_lock(spinlock_t *lock) {
  int tmp;
  asm volatile(
      "1: li %0, 1\n"
      "   amoswap.w.aq %0, %0, %1\n"
      "   bnez %0, 1b\n"
      : "=r"(tmp), "+A"(*lock)
      :
      : "memory");
}

static inline void spin_unlock(spinlock_t *lock) {
  asm volatile("li t0, 0\n"
               "amoswap.w.rl zero, t0, (%0)\n"
               :
               : "r"(lock)
               : "memory", "t0");
}

static inline void sync_barrier(volatile int *counter, int all) {
  int local;
  asm volatile("amoadd.w %0, %2, (%1)\n" : "=r"(local) : "r"(counter), "r"(1) : "memory");

  if (local + 1 < all) {
    do {
      asm volatile("lw %0, (%1)\n" : "=r"(local) : "r"(counter) : "memory");
    } while (local < all);
  }
}

#endif
