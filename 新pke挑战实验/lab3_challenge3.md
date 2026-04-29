

## 5.7 lab3_challenge3 写时复制（Copy On Write）（难度：★★★☆☆）


#### 给定应用
- user/app_cow.c 

```c
/*
 * This app fork a child process to read and write the heap data from parent process.
 * Because implemented copy on write, when child process only read the heap data,
 * the physical address is the same as the parent process.
 * But after writing, child process heap will have different physical address.              
 */

#include "user/user_lib.h"
#include "util/types.h"

int main(void) {
  int *heap_data = naive_malloc();
  printu("the physical address of parent process heap is: ");
  printpa(heap_data);
  int pid = fork();
  if (pid == 0) {
    printu("the physical address of child process heap before copy on write is: ");
    printpa(heap_data);
    heap_data[0] = 0;
    printu("the physical address of child process heap after copy on write is: ");
    printpa(heap_data);
  }
  exit(0);
  return 0;
}
```

该应用执行如下操作：

1. 在父进程的堆上申请一片区域，并输出其物理地址。
2. 进行fork操作，输出子进程在对堆数据写入前后对应的物理地址。


#### 实验内容

本实验为挑战实验，基础代码将继承和使用lab3_3完成后的代码：

- 先提交lab3_3的答案，然后）切换到lab3_challenge3_cow、继承lab3_3（注意，不是继承lab3_challenge1_wait和lab3_challenge2_semaphore！PKE的挑战实验之间无继承关联）中所做修改，并make后的直接运行结果：


```bash
//切换到lab3_challenge3_cow
$ git checkout lab3_challenge3_cow

//继承lab3_3以及之前的答案
$ git merge lab3_3_rrsched -m "continue to work on lab3_challenge3"

$ spike obj/riscv-pke obj/app_cow
In m_start, hartid:0
HTIF is available!
(Emulated) memory size: 2048 MB
Enter supervisor mode...
PKE kernel start 0x0000000080000000, PKE kernel end: 0x000000008000b000, PKE kernel size: 0x000000000000b000 .
free physical memory address: [0x000000008000b000, 0x0000000087ffffff] 
kernel memory manager is initializing ...
KERN_BASE 0x0000000080000000
physical address of _etext is: 0x0000000080005000
kernel page table is on 
Switch to user mode...
in alloc_proc. user frame 0x0000000087fbc000, user stack 0x000000007ffff000, user kstack 0x0000000087fbb000 
User application is loading.
Application: obj/app_two_long_loops
CODE_SEGMENT added at mapped info offset:4
Application program entry point (virtual address): 0x0000000000010078
going to insert process 0 to ready queue.
going to schedule process 0 to run.
the physical address of parent process heap is: 0000000087faf000
User call fork.
will fork a child from parent 0.
in alloc_proc. user frame 0x0000000087fad000, user stack 0x000000007ffff000, user kstack 0x0000000087fac000 
do_fork map code segment at pa:0000000087fb2000 of parent to child at va:0000000000010000.
going to insert process 1 to ready queue.
User exit with code:0.
going to schedule process 1 to run.
the physical address of child process heap before copy on write is: 0000000087fa3000
the physical address of child process heap after copy on write is: 0000000087fa3000
User exit with code:0.
no more ready processes, system shutdown now.
System is shutting down with exit code 0.
```

可以看到，在fork之后，子进程的堆数据无论是否进行写入，其物理地址与父进程均不同。这是因为我们fork时直接为子进程分配了新的物理页面导致的。在实现了写时复制的fork中，是不会直接给子进程申请新的物理页面，而是当子进程执行写入操作时，才通过异常机制来分配新的物理页面，也就是说，子进程的堆数据物理地址在未执行写入前，和父进程应该是一样的。

注意：**不同于基础实验，挑战实验的基础代码具有更大的不完整性，可能无法直接通过构造过程。** 同样，不同于基础实验，我们在代码中也并未专门地哪些地方的代码需要填写，哪些地方的代码无须填写。这样，我们留给读者更大的“想象空间”。

- 本实验要求你通过修改内核代码，使得在fork时不直接为新的堆空间分配内存，而是等写入之后才分配，即实现fork的写时复制(COW)机制。
- 你的程序输出应该如下：

```
$ spike obj/riscv-pke obj/app_cow
In m_start, hartid:0
HTIF is available!
(Emulated) memory size: 2048 MB
Enter supervisor mode...
PKE kernel start 0x0000000080000000, PKE kernel end: 0x000000008004b000, PKE kernel size: 0x000000000004b000 .
free physical memory address: [0x000000008004b000, 0x0000000087ffffff] 
kernel memory manager is initializing ...
KERN_BASE 0x0000000080000000
physical address of _etext is: 0x0000000080005000
kernel page table is on 
Switch to user mode...
in alloc_proc. user frame 0x0000000087fbc000, user stack 0x000000007ffff000, user kstack 0x0000000087fbb000 
User application is loading.
Application: obj/app_cow
CODE_SEGMENT added at mapped info offset:4
Application program entry point (virtual address): 0x0000000000010078
going to insert process 0 to ready queue.
going to schedule process 0 to run.
the physical address of parent process heap is: 0000000087faf000
User call fork.
will fork a child from parent 0.
in alloc_proc. user frame 0x0000000087fad000, user stack 0x000000007ffff000, user kstack 0x0000000087fac000 
do_fork map code segment at pa:0000000087fb2000 of parent to child at va:0000000000010000.
going to insert process 1 to ready queue.
User exit with code:0.
going to schedule process 1 to run.
the physical address of child process heap before copy on write is: 0000000087faf000
handle_page_fault: 0000000000400000
the physical address of child process heap after copy on write is: 0000000087fa0000
User exit with code:0.
no more ready processes, system shutdown now.
System is shutting down with exit code 0.
```

可以看到，输出里多出了处理异常和cow验证输出的语句。

#### 实验指导

写时复制是一种非常常见的优化手段。在Shell中，我们经常fork完一个进程之后会立即调用exec来将子进程替换为新的进程。如果我们在fork时将父进程的所有地址空间全部复制给子进程，而子进程在exec后做的第一件事情是丢弃这个地址空间，也就是我们做了一些无用的复制操作，产生了额外的开销。所以我们可以用“先映射但不实际复制”的方式来优化上述提到的开销，只有子进程真正需要写入映射到父进程的地址空间时，我们才真正的执行“申请空间-复制”这一操作，这就是写时复制（Copy on Write，COW）技术。

- 为完成该挑战，你需要对pke中进程空间有更详细的了解，判断哪些地址范围可能会被访问。
- 你可以参考Linux或其他主流操作系统中写时复制机制的实现。
- 你对内核代码的修改可能包含以下内容：
  - 在堆段的复制发生时，实现”先映射但不实际复制“。
  - 对页表项的标志位进行修改。当子进程拥有映射到父进程的地址的页表项时，页表项应该具有哪些权限？当COW发生后，页表项又该具有哪些权限？
  - 内核如何分辨现在是一个copy-on-write fork的场景？**提示：PTE的标志位中有两位RSW是没有使用的。**
  - 如果有父进程fork了多个子进程，意味着该父进程的物理页会被多个子进程”共享“。举个例子，当父进程退出时我们需要更加的小心，因为我们要判断是否能立即释放相应的物理页。如果有子进程还在使用这些物理页，而内核又释放了这些物理页，将会出现问题。该“共享”页在什么条件下才能释放？

**注意：本挑战的难点在于对页表项的控制以及对页面释放的判断，读者应思考两个问题如何解决，完成设计并验证后，读者可以继续尝试在数据段也加入写时复制机制。**

**另外，后续的基础实验代码并不依赖挑战实验，所以读者可自行决定是否将自己的工作提交到本地代码仓库中（当然，提交到本地仓库是个好习惯，至少能保存自己的“作品”）。**

