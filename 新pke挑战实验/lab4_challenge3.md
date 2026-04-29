

## 6.7 lab4_challenge3 简易Shell（难度：★★★★★）


#### 给定应用
- user/app_shell.c 

```c
/*
 * This app starts a very simple shell and executes some simple commands.
 * The commands are stored in the hostfs_root/shellrc
 * The shell loads the file and executes the command line by line.                 
 */
#include "user_lib.h"
#include "string.h"
#include "util/types.h"

int main(int argc, char *argv[]) {
  printu("\n======== Shell Start ========\n\n");
  int fd;
  int MAXBUF = 1024;
  char buf[MAXBUF];
  char *token;
  char delim[3] = " \n";
  fd = open("/shellrc", O_RDONLY);

  read_u(fd, buf, MAXBUF);
  close(fd);
  char *command = naive_malloc();
  char *para = naive_malloc();
  int start = 0;
  while (1)
  {
    if(!start) {
      token = strtok(buf, delim);
      start = 1;
    }
    else 
      token = strtok(NULL, delim);
    strcpy(command, token);
    token = strtok(NULL, delim);
    strcpy(para, token);
    if(strcmp(command, "END") == 0 && strcmp(para, "END") == 0)
      break;
    printu("Next command: %s %s\n\n", command, para);
    printu("==========Command Start============\n\n");
    int pid = fork();
    if(pid == 0) {
      int ret = exec(command, para);
      if (ret == -1)
      printu("exec failed!\n");
    }
    else
    {
      wait(pid);
      printu("==========Command End============\n\n");
    }
  }
  exit(0);
  return 0;
}

```

该应用程序从位于hostfs_root下的shellrc文件中读取若干条命令，对于每条命令，我们都会fork一个新的进程，并exec对应的程序来完成命令。exec将用第一个参数对应的程序来替换fork出来的新进程，并将第二个参数的内容传入新的进程。

给出的shellrc文件如下：

```
/bin/app_mkdir /RAMDISK0/sub_dir
/bin/app_touch /RAMDISK0/sub_dir/ramfile1
/bin/app_touch /RAMDISK0/sub_dir/ramfile2
/bin/app_echo /RAMDISK0/sub_dir/ramfile1
/bin/app_cat /RAMDISK0/sub_dir/ramfile1
/bin/app_ls /RAMDISK0/sub_dir
/bin/app_ls /RAMDISK0
END END

```

可以看到，我们提供了五种简化版的命令，分别是ls，mkdir，touch，echo，cat。命令的对应功能参考Linux下的命令。（echo命令的内容有所不同，为了简单起见，提供的echo命令功能为：向指定的文件写入hello world。比如shellrc中的echo命令实际上是向/RAMDISK0/sub_dir/ramfile1文件中写入了hello world）

同样是为了简单起见，我们规定在遇到两个END时结束程序。

shellrc中的其余命令实现均位于user目录下，这里不一一列出，请感兴趣的读者自行查看。

shell程序将会依次执行上述命令，如果你完成了本实验，你应当看到的输出如下：

```
$ spike obj/riscv-pke /bin/app_shell
In m_start, hartid:0
HTIF is available!
(Emulated) memory size: 2048 MB
Enter supervisor mode...
PKE kernel start 0x0000000080000000, PKE kernel end: 0x0000000080011000, PKE kernel size: 0x0000000000011000 .
free physical memory address: [0x0000000080011000, 0x0000000087ffffff] 
kernel memory manager is initializing ...
KERN_BASE 0x0000000080000000
physical address of _etext is: 0x0000000080009000
kernel page table is on 
RAMDISK0: base address of RAMDISK0 is: 0x0000000087f35000
RFS: format RAMDISK0 done!
Switch to user mode...
in alloc_proc. user frame 0x0000000087f29000, user stack 0x000000007ffff000, user kstack 0x0000000087f28000 
FS: created a file management struct for a process.
in alloc_proc. build proc_file_management successfully.
User application is loading.
Application: /bin/app_shell
CODE_SEGMENT added at mapped info offset:4
DATA_SEGMENT added at mapped info offset:5
Application program entry point (virtual address): 0x00000000000100b0
going to insert process 0 to ready queue.
going to schedule process 0 to run.

======== Shell Start ========

Next command: /bin/app_mkdir /RAMDISK0/sub_dir

==========Command Start============

User call fork.
will fork a child from parent 0.
in alloc_proc. user frame 0x0000000087f0e000, user stack 0x000000007ffff000, user kstack 0x0000000087f0c000 
FS: created a file management struct for a process.
in alloc_proc. build proc_file_management successfully.
do_fork map code segment at pa:0000000087f13000 of parent to child at va:0000000000010000.
going to insert process 1 to ready queue.
going to schedule process 1 to run.
Application: /bin/app_mkdir
CODE_SEGMENT added at mapped info offset:4
Application program entry point (virtual address): 0x0000000000010078

======== mkdir command ========
mkdir: /RAMDISK0/sub_dir
User exit with code:0.
going to insert process 0 to ready queue.
going to schedule process 0 to run.
==========Command End============

Next command: /bin/app_touch /RAMDISK0/sub_dir/ramfile1

==========Command Start============

User call fork.
will fork a child from parent 0.
in alloc_proc. user frame 0x0000000087ef3000, user stack 0x000000007ffff000, user kstack 0x0000000087ef2000 
FS: created a file management struct for a process.
in alloc_proc. build proc_file_management successfully.
do_fork map code segment at pa:0000000087f13000 of parent to child at va:0000000000010000.
going to insert process 2 to ready queue.
going to schedule process 2 to run.
Application: /bin/app_touch
CODE_SEGMENT added at mapped info offset:4
Application program entry point (virtual address): 0x0000000000010078

======== touch command ========
touch: /RAMDISK0/sub_dir/ramfile1
file descriptor fd: 0
User exit with code:0.
going to insert process 0 to ready queue.
going to schedule process 0 to run.
==========Command End============

Next command: /bin/app_touch /RAMDISK0/sub_dir/ramfile2

==========Command Start============

User call fork.
will fork a child from parent 0.
in alloc_proc. user frame 0x0000000087ee8000, user stack 0x000000007ffff000, user kstack 0x0000000087ee6000 
FS: created a file management struct for a process.
in alloc_proc. build proc_file_management successfully.
do_fork map code segment at pa:0000000087f13000 of parent to child at va:0000000000010000.
going to insert process 3 to ready queue.
going to schedule process 3 to run.
Application: /bin/app_touch
CODE_SEGMENT added at mapped info offset:4
Application program entry point (virtual address): 0x0000000000010078

======== touch command ========
touch: /RAMDISK0/sub_dir/ramfile2
file descriptor fd: 0
User exit with code:0.
going to insert process 0 to ready queue.
going to schedule process 0 to run.
==========Command End============

Next command: /bin/app_echo /RAMDISK0/sub_dir/ramfile1

==========Command Start============

User call fork.
will fork a child from parent 0.
in alloc_proc. user frame 0x0000000087ed1000, user stack 0x000000007ffff000, user kstack 0x0000000087ecf000 
FS: created a file management struct for a process.
in alloc_proc. build proc_file_management successfully.
do_fork map code segment at pa:0000000087f13000 of parent to child at va:0000000000010000.
going to insert process 4 to ready queue.
going to schedule process 4 to run.
Application: /bin/app_echo
CODE_SEGMENT added at mapped info offset:4
DATA_SEGMENT added at mapped info offset:5
Application program entry point (virtual address): 0x00000000000100b0

======== echo command ========
echo: /RAMDISK0/sub_dir/ramfile1
file descriptor fd: 0
write content: 
hello world
User exit with code:0.
going to insert process 0 to ready queue.
going to schedule process 0 to run.
==========Command End============

Next command: /bin/app_cat /RAMDISK0/sub_dir/ramfile1

==========Command Start============

User call fork.
will fork a child from parent 0.
in alloc_proc. user frame 0x0000000087eba000, user stack 0x000000007ffff000, user kstack 0x0000000087eb8000 
FS: created a file management struct for a process.
in alloc_proc. build proc_file_management successfully.
do_fork map code segment at pa:0000000087f13000 of parent to child at va:0000000000010000.
going to insert process 5 to ready queue.
going to schedule process 5 to run.
Application: /bin/app_cat
CODE_SEGMENT added at mapped info offset:4
Application program entry point (virtual address): 0x0000000000010078

======== cat command ========
cat: /RAMDISK0/sub_dir/ramfile1
file descriptor fd: 0
read content: 
hello world
User exit with code:0.
going to insert process 0 to ready queue.
going to schedule process 0 to run.
==========Command End============

Next command: /bin/app_ls /RAMDISK0/sub_dir

==========Command Start============

User call fork.
will fork a child from parent 0.
in alloc_proc. user frame 0x0000000087ea2000, user stack 0x000000007ffff000, user kstack 0x0000000087ea0000 
FS: created a file management struct for a process.
in alloc_proc. build proc_file_management successfully.
do_fork map code segment at pa:0000000087f13000 of parent to child at va:0000000000010000.
going to insert process 6 to ready queue.
going to schedule process 6 to run.
Application: /bin/app_ls
CODE_SEGMENT added at mapped info offset:4
DATA_SEGMENT added at mapped info offset:5
Application program entry point (virtual address): 0x00000000000100b0
---------- ls command -----------
ls "/RAMDISK0/sub_dir":
[name]               [inode_num]
ramfile1             2
ramfile2             3
------------------------------
User exit with code:0.
going to insert process 0 to ready queue.
going to schedule process 0 to run.
==========Command End============

Next command: /bin/app_ls /RAMDISK0

==========Command Start============

User call fork.
will fork a child from parent 0.
in alloc_proc. user frame 0x0000000087e8b000, user stack 0x000000007ffff000, user kstack 0x0000000087f14000 
FS: created a file management struct for a process.
in alloc_proc. build proc_file_management successfully.
do_fork map code segment at pa:0000000087f13000 of parent to child at va:0000000000010000.
going to insert process 7 to ready queue.
going to schedule process 7 to run.
Application: /bin/app_ls
CODE_SEGMENT added at mapped info offset:4
DATA_SEGMENT added at mapped info offset:5
Application program entry point (virtual address): 0x00000000000100b0
---------- ls command -----------
ls "/RAMDISK0":
[name]               [inode_num]
sub_dir              1
------------------------------
User exit with code:0.
going to insert process 0 to ready queue.
going to schedule process 0 to run.
==========Command End============

User exit with code:0.
no more ready processes, system shutdown now.
System is shutting down with exit code 0.
```


#### 实验内容

本实验为挑战实验，基础代码将继承和使用lab4_3_hardlink完成后的代码：

- 先提交lab4_3_hardlink的答案，然后）切换到lab4_challenge3_shell、继承lab4_3_hardlink中所做修改：


```bash
//切换到lab4_challenge3_shell
$ git checkout lab4_challenge3_shell

//继承lab4_3_hardlink以及之前的答案
$ git merge lab4_3_hardlink -m "continue to work on lab4_challenge3"
```

注意：**不同于基础实验，挑战实验的基础代码具有更大的不完整性，可能无法直接通过构造过程。** 同样，不同于基础实验，我们在代码中也并未专门地哪些地方的代码需要填写，哪些地方的代码无须填写。这样，我们留给读者更大的“想象空间”。

- 本实验的具体要求为：

  - 通过修改PKE内核和系统调用，为用户程序提供exec函数的功能。exec接受一个可执行程序的路径名，以及**一个字符串**作为参数。（实际上，exec系统调用的第二个参数本应是一个参数数组，即char** 类型，这里为简单起见仅需传入一个参数即可。如果读者学有余力，可以自行实现多个参数传入版本的exec）
  - 此外，为了保证shell的正规性，你需要额外实现wait功能，来保证程序的执行顺序。如果你已经完成了lab3_challenge1，那么可以直接使用对应成果。
  

#### 实验指导

shell是Linux中的一个重要概念，主要用于和用户的交互。其基本流程为：用户输入命令->shell创建一个新进程，并使用exec函数加载命令对应的程序->将结果通过shell返回用户。其中，fork函数的功能已在前面的实验中实现，本实验主要实现exec函数的功能。（如果你已完成lab4_challenge2的话，对本实验会非常有帮助）

不考虑参数传递问题的话，exec要做的工作基本为替换elf内容和重置堆栈等地址空间。但是因为存在新进程需要得知命令参数的问题，exec除了提到的工作之外，还需要考虑以下问题：

- main函数中的argc，argv数组的含义，以及main函数是从哪里得到它们的。（**提示：main函数也是函数，函数的参数一般保存在哪里呢**）
- 在lab1中我们初步了解了系统调用，可以发现，系统调用使用了a0~a7寄存器。请读者思考：**处理系统调用的函数的返回值保存在哪里，有什么作用呢？**
- 在exec替换完elf后，我们将堆栈重置。此时的sp指向什么位置，是否可以修改以达到传递参数的效果。
- 注意地址对齐。读者可自行参阅RISC-V的相关资料，了解该指令集下的地址对齐规则。

**注意：完成实验内容后，若读者同时完成了之前的“lab3_challenge3 写时复制”挑战实验，则可以将fork添加写时复制功能来减少不必要的开销。此外，本挑战实验具有很强的扩展性，读者可通过自行比对与真实shell的差别，为pke-shell添加更完善的功能。**

**另外，后续的基础实验代码并不依赖挑战实验，所以读者可自行决定是否将自己的工作提交到本地代码仓库中（当然，提交到本地仓库是个好习惯，至少能保存自己的“作品”）。**

