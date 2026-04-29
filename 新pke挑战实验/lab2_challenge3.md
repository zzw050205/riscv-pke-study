## 4.7 lab2_challenge3 pke多核内存管理（难度：★★☆☆☆）

在进行此实验之前，你应当完成lab1_challenge3。

#### 给定应用

- user/app_alloc0.c 

```c

#include "user_lib.h"
#include "util/types.h"

#define N 5
#define BASE 0

int main(void) {
  void *p[N];
  
  for (int i = 0; i < N; i++) {
    p[i] = naive_malloc();
    int *pi = p[i];
    *pi = BASE + i;
    printu("=== user alloc 0 @ vaddr 0x%x\n", p[i]);
  }
  
  for (int i = 0; i < N; i++) {
    int *pi = p[i];
    printu("=== user0: %d\n", *pi);
    naive_free(p[i]);
  }

  exit(0);
}


```

- user/app_alloc1.c

```c

#include "user_lib.h"
#include "util/types.h"

#define N 5
#define BASE 5

int main(void) {
  void *p[N];

  for (int i = 0; i < N; i++) {
    p[i] = naive_malloc();
    int *pi = p[i];
    *pi = BASE + i;
    printu(">>> user alloc 1 @ vaddr 0x%x\n", p[i]);
  }

  for (int i = 0; i < N; i++) {
    int *pi = p[i];
    printu(">>> user 1: %d\n", *pi);
    naive_free(p[i]);
  }

  exit(0);
}

```

在本次实验中，给定两个程序，每个程序会通过lab2_2实现的`naive_malloc`申请一些内存页，在内存页开始处写入一个int并打印内存页的虚拟地址。最后每个进程会打印自己写入内存页的数，并通过`naive_free`释放申请的内存页。`app_alloc0.c`会依次写入并输出`0,1,2,3,4`，`app_alloc1.c`会依次写入并输出`5,6,7,8,9`。

#### 实验内容

本实验为挑战实验，基础代码将继承和使用lab2_3完成后的代码：

- 先提交lab2_3的答案，然后切换到lab2_challenge3_multicoremem，继承lab2_3（注意，不是继承lab2_challenge1_pagefaults和lab2_challenge2_singlepageheap！PKE的挑战实验之间无继承关联）中所做修改：

```shell
//切换到lab2_challenge3_multicoremem
$ git checkout lab2_challenge3_multicoremem

//继承lab2_3以及之前的答案
$ git merge lab2_3_pagefault -m "continue to work on lab2_challenge3"
```

注意：**不同于基础实验，挑战实验的基础代码具有更大的不完整性，可能无法直接通过构造过程。**同样，不同于基础实验，我们在代码中也并未专门地哪些地方的代码需要填写，哪些地方的代码无须填写。这样，我们留给读者更大的“想象空间”。

- 在lab1_challenge3中，你已经实现了一个不支持虚拟内存的简单的多核操作系统。现在在lab2中，因为虚拟内存概念的引入，需要你为这个简单操作系统添加额外的多核内存管理。
- 如同lab1_challenge3，`kernel/config.h`中的`NCPU`规定了操作系统内核支持的核数，在本实验设置为2，即要求你能够正确执行`spike -p2 riscv-pke app_alloc0 app_alloc1`，每个核分别执行一个进程，进行内存的分配与释放。
- 注意，每个进程分配得到的**虚拟地址应当是连续**的，并且不同核上的进程**不应该分配到同一个物理页**。
- 最终你的输出应当如下所示：

```text
HTIF is available!
(Emulated) memory size: 2048 MB
In m_start, hartid:0
hartid = 0: Enter supervisor mode...
PKE kernel start 0x0000000080000000, PKE kernel end: 0x000000008000a000, PKE kernel size: 0x000000000000a000 .
free physical memory address: [0x000000008000a000, 0x0000000087ffffff] 
kernel memory manager is initializing ...
In m_start, hartid:1
hartid = 1: Enter supervisor mode...
KERN_BASE 0x0000000080000000
physical address of _etext is: 0x0000000080006000
hartid = 0: User application is loading.
hartid = 1: User application is loading.
hartid = 0: user frame 0x0000000087fbc000, user stack 0x000000007ffff000, user kstack 0x0000000087fbb000 
hartid = 0: Application: obj/app_alloc0
hartid = 1: user frame 0x0000000087fb8000, user stack 0x000000007ffff000, user kstack 0x0000000087fb7000 
hartid = 1: Application: obj/app_alloc1
hartid = 0: Application program entry point (virtual address): 0x00000000000100b0
hartid = 1: Application program entry point (virtual address): 0x00000000000100b0
hartid = 0: Switch to user mode...
hartid = 0: alloc page 0x87fa4000
hartid = 0: alloc page 0x87fa3000
hartid = 1: Switch to user mode...
hartid = 0: vaddr 0x00400000 is mapped to paddr 0x87fa4000
hartid = 1: alloc page 0x87fa2000
hartid = 1: alloc page 0x87fa1000
=== user alloc 0 @ vaddr 0x00400000
hartid = 1: vaddr 0x00400000 is mapped to paddr 0x87fa2000
hartid = 0: alloc page 0x87fa0000
hartid = 0: vaddr 0x00401000 is mapped to paddr 0x87fa0000
>>> user alloc 1 @ vaddr 0x00400000
=== user alloc 0 @ vaddr 0x00401000
hartid = 1: alloc page 0x87f9f000
hartid = 1: vaddr 0x00401000 is mapped to paddr 0x87f9f000
hartid = 0: alloc page 0x87f9e000
hartid = 0: vaddr 0x00402000 is mapped to paddr 0x87f9e000
>>> user alloc 1 @ vaddr 0x00401000
=== user alloc 0 @ vaddr 0x00402000
hartid = 1: alloc page 0x87f9d000
hartid = 1: vaddr 0x00402000 is mapped to paddr 0x87f9d000
hartid = 0: alloc page 0x87f9c000
hartid = 0: vaddr 0x00403000 is mapped to paddr 0x87f9c000
>>> user alloc 1 @ vaddr 0x00402000
=== user alloc 0 @ vaddr 0x00403000
hartid = 1: alloc page 0x87f9b000
hartid = 1: vaddr 0x00403000 is mapped to paddr 0x87f9b000
hartid = 0: alloc page 0x87f9a000
hartid = 0: vaddr 0x00404000 is mapped to paddr 0x87f9a000
>>> user alloc 1 @ vaddr 0x00403000
=== user alloc 0 @ vaddr 0x00404000
hartid = 1: alloc page 0x87f99000
hartid = 1: vaddr 0x00404000 is mapped to paddr 0x87f99000
=== user0: 0
>>> user alloc 1 @ vaddr 0x00404000
=== user0: 1
>>> user 1: 5
=== user0: 2
>>> user 1: 6
=== user0: 3
>>> user 1: 7
=== user0: 4
>>> user 1: 8
hartid = 0: User exit with code: 0.
>>> user 1: 9
hartid = 1: User exit with code: 0.
hartid = 0: shutdown with code: 0.
System is shutting down with exit code 0.
```

#### 实验指导

参照lab1_challenge3，在引入进程与内存的概念之后，需要对其内存管理进行并发控制和资源隔离。

在通过`s_start`进入S mode的时候，pke会对物理内存和内核页表进行初始化。如同lab1_challenge3中spike设备的初始化，这个过程也只应执行一次，并且初始化完毕后所有核才能够开启页表，继续执行之后的指令。

物理地址是所有核共享的，各个核可能会并发的操作物理地址，带来未知的错误。你需要实现一个互斥锁，使得同一时间只有一个核才能分配和释放物理地址。关于互斥锁的实现，你可以选择使用RISC-V原子指令`amoswap`制作一个简单的自旋锁。

基础代码中在分配物理页处有一行代码`sprint`，打印用户分配得到的物理页地址。`vm_alloc_stage`是用来帮助内核判断是否由用户进程在申请内存，其被初始化为0，表示一开始是内核在申请物理内存；并在`switch_to`被设置为`1`，表示是用户进程在申请物理内存。但是这个代码目前不支持多核，你需要进行相应修改使其适配多核，并能够正确打印出每个核上用户进程申请的物理页地址。
```c
void *alloc_page(void) {
  list_node *n = g_free_mem_list.next;
  uint64 hartid = 0;
  if (vm_alloc_stage[hartid]) {
    sprint("hartid = %ld: alloc page 0x%x\n", hartid, n);
  }
  if (n) g_free_mem_list.next = n->next;
  return (void *)n;
}
```

此实验中，你需要利用`kernel/sync_utils.h`中的同步原语来管理内存，防止同一个物理内存页被两个进程同时占有。如果你没能正确控制物理内存管理的并发，可能会出现：
1. `app_alloc0`的输出不依次为`0, 1, 2, 3, 4`，或者`app_alloc1`的输出不依次为`5, 6, 7, 8, 9`
2. 由于两个进程同时写入同一个物理页，虚拟机触发异常并导致操作系统内核崩溃

此外，在lab2之前的单进程实验中，虚拟地址是内核的全局变量在管理。然而在正确的实现中，每个进程的虚拟地址空间应该是隔离的。否则，在多进程情况下，每个进程的虚拟地址可能会发生一些重叠或者缺失的情况。你需要实更改单进程情况下的虚拟地址管理，使其能够支持多个进程。如果你的实现不正确，你会看见每个进程通过`naive_malloc`得到的虚拟内存页不连续。

在实验的基础代码中，有一些打印分配内存地址的代码，你需要将这部分代码改为支持多核执行的版本，然后便可以阅读执行后的输出来判断是否为每个进程分配得到的物理内存页和对应的虚拟内存。


