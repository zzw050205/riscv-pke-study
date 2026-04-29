

## 3.5 lab1_challenge3 pke多核启动及运行（难度：★★★★☆）

之前的实验都在单核环境下进行。在本次实验中，你需要修改操作系统内核使其支持两核并发运行，并且在每个核上加载一个程序运行，等到两个程序都执行完毕后退出并关闭模拟器。

#### 给定应用
- user/app0.c

```c
#include "user_lib.h"
#include "util/types.h"

int main(void) {
  printu(">>> app0 is expected to be executed by hart0\n");
  exit(0);
}
```

- user/app1.c

```c
#include "user_lib.h"
#include "util/types.h"

int main(void) {
  printu(">>> app1 is expected to be executed by hart1\n");
  exit(0);
}
```

在本次实验中，给定两个简单的用户程序，每个程序会输出一句话。你需要让每个核分别加载一个程序，并能够正确运行，输出相应内容然后退出。

#### 实验内容

本实验为挑战实验，基础代码将继承和使用lab1_3完成后的代码：

- 先提交lab1_3的答案，然后）切换到lab1_challenge3_multicore、继承lab1_3（注意，不是继承lab1_challenge1_backtrace和lab1_challenge2_errorline！PKE的挑战实验之间无继承关联）中所做修改：


```bash
//切换到lab1_challenge3_multicore
$ git checkout lab1_challenge3_multicore

//继承lab1_3以及之前的答案
$ git merge lab1_3_irq -m "continue to work on lab1_challenge3"
```

注意：**不同于基础实验，挑战实验的基础代码具有更大的不完整性，可能无法直接通过构造过程。** 同样，不同于基础实验，我们在代码中也并未专门地哪些地方的代码需要填写，哪些地方的代码无须填写。这样，我们留给读者更大的“想象空间”。

在RISC-V处理器中，每一个CPU称作一个hart(hardware thread)，并从0开始编号。此实验要求启动两个CPU，它们的编号分别为0和1。

- 本实验中，你需要修改内核代码，使得riscv-pke能够通过spike启动两个CPU(hart)，并且让CPU0执行app0，让CPU1执行app1。
- `user0.lds`和`user1.lds`中规定了app0从0x81000000处开始加载，app1从0x81500000处开始加载。
- `kernel/config.h`中的`NCPU`规定了操作系统内核支持的核数，在本实验设置为2，即需要能够通过`spike -p2 riscv-pke app0 app1`让pke正确地启动两核并发的操作系统并执行app0和app1。
- 内核中有很多使用`sprint`输出的内容，为了分别每一条输出是哪个核执行的，你需要**在相应的sprint处添加一个hartid的输出项**。你应当思考并阅读和核数有关的内核代码，并且任意添加和修改，使得最终运行的输出对齐以下输出

```
HTIF is available!
(Emulated) memory size: 2048 MB
In m_start, hartid:0
hartid = 0: Enter supervisor mode...
hartid = 0: Application: ./obj/app0
hartid = 0: Application program entry point (virtual address): 0x0000000081000000
In m_start, hartid:1
hartid = 1: Enter supervisor mode...
hartid = 1: Application: ./obj/app1
hartid = 1: Application program entry point (virtual address): 0x0000000085000000
hartid = 1: Switch to user mode...
hartid = 0: Switch to user mode...
hardid = 1: >>> app1 is executed by hart1
hardid = 0: >>> app0 is executed by hart0
hartid = 1: User exit with code:0.
hartid = 0: User exit with code:0.
hartid = 0: shutdown with code:0.
System is shutting down with exit code 0.
```

#### 实验指导


**多核riscv-pke的启动与运行**

spike模拟器支持`-p`选项来模拟多个核，在pke中，通过`spike -p2 riscv-pke ...`会让两个核并发地从`kernel/machine/mentry.S`中的`_mentry`开始执行，并且每个核都会如同之前单核启动一样独立对操作系统进行初始化。在硬件语境下，每一个核都是一个硬件线程hart，可以类比软件线程理解。

更具体地来讲，每个核从`_mentry`开始执行，然后进入`minit.c`中的`m_start`，初始化模拟器设备和接口，设置中断信息等，再进入到S模式执行`s_start`对操作系统进行初始化，使用`load_user_program`加载应用并通过`switch_to`切换至用户程序。

这样启动和执行会带来一些需要处理的问题
1. spike模拟器通过HTIF与host机器的物理设备进行交互，在pke启动时，会在M mode对spike模拟器的一些虚拟设备和HTIF接口进行初始化。这个初始化的过程只能被执行一次，而非每个核都执行一次。并且在spike和HTIF初始化完成之前，需要用**同步机制**保证每个核都不会访问他们相应的资源。
2. 每个核的时钟周期和时钟中断应该是独立的，不能受其他核的干扰。即你需要分开管理每个hart上运行应用的`tick`。
3. 单核实验中，pke会从命令行第一个参数加载应用程序，但是本次实验需要从命令行前两个参数加载两个程序。你需要阅读并修改`elf.c`来让pke能从命令行加载第二个app。
4. 之前的实验在单核的基础上开展，且没有内存管理机制，所以user app使用到的一些内存地址是固定的（见`kernel/config.h`）。即使实验基础代码在elf文件中为两个app指定了不同的起始地址，两个核同时加载并运行的它们时，其部分内存地址也会重叠并出错。你应当想办法分离两个app的内存。
5. 在之前的单核实验中，有一个全局的`current`变量来指示当前正在执行的进程，以便处理中断。然而在多核环境下，每个核的中断是独立的，在处理好中断后，各个核应当恢复到其原本执行的用户app。你需要让每个核能知道他们在执行什么app。
6. 在单核环境下，一个核的用户进程调用`exit`退出时，会立即关闭模拟器；而在多核环境下，这会强使其他核也停止工作。正确的退出方式是，同步等到所有核执行完毕之后再关闭模拟器。本次实验中，你应当让CPU0负责关闭模拟器。

**Tips**

如果你对多核pke的运行原理感到困惑，可以带着以下的tips去复习操作系统以及阅读pke源码：
1. 理解在多核场景下，什么资源是唯一的，什么资源是多份的。唯一的资源有：模拟器相关接口、内存和设备。多份的资源有：每个核的所有寄存器和状态信息。
2. 对于唯一的资源，所有核会共享，所以需要控制并发。对于多份的资源，所有核应当独立占有，所以需要隔离资源。

如果你大致理解了多核pke的运行机制，以下内容可以帮助你实现对多核的支持：
1. 对于riscv处理器，有一个寄存器`tp (thread pointer)`是专门用来保存hart相关信息的，可以利用此寄存器存储每个核关于自身的信息。
2. `kernel/sync_utils.h`提供了一个简单的同步屏障的实现，你可以通过这个同步原语实现并发控制，或者自行设计其它同步原语。

