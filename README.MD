# epos
>这个项目源自于重庆大学大数据与软件学院操作系统课程实验，作者洪明坚教授</br>
项目原地址：https://github.com/hongmingjian/epos

本项目fork自洪教授的教学用操作系统，并包含了我自己的在做实验时的相关代码。
</br>

# 实验一 系统调用
第一个实验是向系统中添加一个系统调用，要求如下：
![实验要求](https://img-blog.csdn.net/2018032522145554?watermark/2/text/aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L0F1Z3VzdF8xOTk3/font/5a6L5L2T/fontsize/400/fill/I0JBQkFCMA==/dissolve/70)
实验的流程如下：
		1.首先是在内核头文件中 kernel/kernel.h 中添加该调用函数的声明；
		2.然后便是在内核态的 kernel/machdep.c 文件中实现该系统调用函数 sys_time()，该文件包含了内核态的中断处理、系统调用分发等函数；
```
time_t sys_time()
{
	// 变量g_startup_time,它记录了EPOS启动时，距离格林尼治时间1970年1月1日午夜的秒数
	// g_timer_ticks记录了EPOS启动以来定时器中断的总次数
	// HZ是定时器每秒钟中断的次数,即定时器中断的频率
    return ((long)(g_timer_ticks) & 0xffff) / HZ + g_startup_time;
}
``` 
3.实现了具体的函数功能后，便是将该函数添加到系统调用中了。首先要在 include/syscall-nr.h 中添加该函数的系统调用号码（自定义）；
```
#define SYSCALL_time          2016
```
4.定义完系统调用的号码后，就是将在系统调用的分发函数 syscall ( kernel/machdep.c) 中添加该调用分支。 

```
    case SYSCALL_time:
    {
        // 取出已入栈的函数参数
        time_t *loc = *(time_t **)(ctx->esp + 4);
        // 调用系统函数
        ctx->eax = sys_time();
        // 判断是否为空
        if(loc != NULL)
            // 不为空 写入数据
            *loc = ctx->eax;
        break;
    }
```
注意到在取出入栈的函数参数时，我们是将 esp+4 强制转换为了 time_t 的二级指针后再解引用才得到参数的。首先解释这个二级指针的含义，我们要求用户传递给函数的参数是用来保存结果值（time_t类型）的内存地址，即参数本身是一个指针（time_t *）；而我们又是通过 esp 这一个函数栈指针取出参数来的。因此综合起来，栈中的参数地址就是一个 time_t 类型的二级指针了，最后再对这个二级指针解引用就得到了参数的值。以下是示意图：
![](https://img-blog.csdn.net/20180331123814954?watermark/2/text/aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L0F1Z3VzdF8xOTk3/font/5a6L5L2T/fontsize/400/fill/I0JBQkFCMA==/dissolve/70)


&emsp;上面四部完成了在内核态中的编码，接下来就是在用户态下进行编码了：
1. 我们要在用户态的 main.c 中调用内核态中的刚添加的函数，显然需要一个入口让程序从用户态陷入内核态中，这便是 wrapper函数的作用。
```
# wrapper函数
#define WRAPPER(name) \
  .globl _ ## name; \
_ ## name: \
    movl $SYSCALL_ ## name, %eax; \
    int $0x82; \
    ret
```
wrapper函数将系统调用号码存入 eax 中，然后调用82号中断陷入内核态中执行系统调用。因此我们要做的就是添加一个time系统调用的汇编接口
```
// 添加汇编语言接口
WRAPPER(time)
```
2.添加完到内核态的接口后便是在用户态系统调用头文件（userapp/include/syscall.h）中声明 time 函数供主函数调用
```
// 时间调用函数的C语言声明
time_t time(time_t *loc);
```
3.最后就是在 userapp/main.c 中调用该系统调用了，因注意分为参数为NULL和非NULL的情况
```
// 	系统时间调用函数
    // 分配内存测试
    time_t *loc = (time_t *)malloc(sizeof(time_t));
    long NonNULL_time = time(loc);
    printf("\nNonNULL case : the seconds since Greenwich time is %ld Loc:%ld\n", 
	    NonNULL_time, *loc);
    free(loc);
    loc = NULL;

    // 不分配内存测试
    long NULL_time = time(NULL);      
    printf("NULL case : the seconds since Greenwich time is %ld\n", NULL_time);
```
结果如下：
![这里写图片描述](https://img-blog.csdn.net/20180331154423209?watermark/2/text/aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L0F1Z3VzdF8xOTk3/font/5a6L5L2T/fontsize/400/fill/I0JBQkFCMA==/dissolve/70)

以上就是 epos 系统的系统调用部分实验
</br>
