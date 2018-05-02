# epos
>这个项目源自于重庆大学大数据与软件学院操作系统课程实验</br>
项目原地址：https://github.com/hongmingjian/epos

>我的原博客地址: https://blog.csdn.net/August_1997

本项目fork自课程教学用操作系统，并包含了我自己的在做实验时的相关代码。
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

# 实验二 线程的创建
&emsp;这个实验要求是要创建多线程使用不通的排序算法对数组进行排序，并以绘图排序的方式表现出整个排序过程。我在实现时，完成了按画线长度排序和按色彩排序两种，效果大致如图：
![排序示意图](https://img-blog.csdn.net/20180419104132515?watermark/2/text/aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L0F1Z3VzdF8xOTk3/font/5a6L5L2T/fontsize/400/fill/I0JBQkFCMA==/dissolve/70)

&emsp;首先通过具体添加一个排序线程的方式讲一下我实现的整体结构：

（1） 调用attrGenerator函数，传递相应排序函数地址等信息，该函数将构造并返回线程函数需要参数结构体</br>
（2）调用sortThreadRun函数，传递上一步构造的参数结构体，该函数是一个内联函数，它将申请用户栈和运行	task_create创建具体的排序线程，并返回线程号码</br>
（3） 调用task_wait函数，传递上一步得来的线程号码，等待线程结束</br>
（4） 调用sortFree函数，该函数将释放用户线程参数结构体中的用户线程栈以及排序数组的内存空间</br>

&emsp;这样的设计的目的是尽可能进行封装并向外提供接口，以此简化添加一个特定排序线程所需要的代码流程。</br>
&emsp;在上述设计中，所有排序函数需要具有相同的函数类型（即函数的返回值与参数类型必须相同），这样所有的排序线程才能通过一个通用的函数指针调用和共用一种函数参数结构体和一个线程创建函数。接下来我将一步一步的说明具体步骤。

## 1. 构造排序线程所需的参数结构体
先说一下我用来描述一个排序函数所有特征的结构体：
```
typedef struct sortFunctionAttributes
{
    sortFunction sortFun;   // 排序函数指针
    int *randNumber;        // 数组指针
    int size;               // 数组大小
    int X_Location;         // 水平位置
    int Y_Location;         // 垂直位置
    unsigned char *stack_sort;  //线程栈
    int colorSort;         // 1：按颜色排序； 0：按长度排序
} sortAttributes;
```
为了完成结构体实例的创建，我实现了一个attrGenerator函数去完成这个工作：
```
sortAttributes *attrGenerator(sortFunction sortFun, int colorSort, int X_Location, int Y_Location)
{
    sortAttributes *sortAttr = (sortAttributes*)malloc(sizeof(sortAttributes));

    // 设置各属性
    sortAttr->size  = BLOCK_HIGHT;
    sortAttr->sortFun = sortFun;
    sortAttr->X_Location = X_Location;
    sortAttr->Y_Location = Y_Location;
    sortAttr->colorSort = colorSort;

    // 播种随机数
    srand(time(NULL));
    if(colorSort == 0)  // 长度排序
    {
        sortAttr->randNumber = malloc(sizeof(int) * 1024);
        for (int i = 0; i < BLOCK_HIGHT; ++i)
            sortAttr->randNumber[i] = rand() % BLOCK_WIDTH;
    }
    else  // 色彩排序
    {
        sortAttr->randNumber = malloc(sizeof(int) * 1024);
        for (int i = 0; i < BLOCK_HIGHT; ++i)
            sortAttr->randNumber[i] = rand() % 360;
    }

    return sortAttr;
}
```
实现中值得一提的是在按色彩排序的方式中我对数组进行初始化时，我让每个随机数对360°取模，这里涉及到了RGB和色相转换的关系，相关知识可参考这篇文章：https://www.cnblogs.com/wang726zq/p/3660581.htm

结构体生成器函数参数中的sortFunction就是排序函数的函数指针，声明如下：
```
typedef void (*sortFunction)(int ar[], int sz, int crSort, int X_Loc, int Y_Loc);
```
使用了这样的函数指针就要求我的每个排序函数使用相同的函数类型，如冒泡排序：
```
// 冒泡
void bubbleSort(int ar[], int size, int colorSort, int X_Location, int Y_Location);
```

最后两个参数是指的这个排序线程过程画在水平和垂直方向的哪一块。
我对屏幕区域进行了划分的计算，为了方便使用定义成了宏：
```
#define SCREEN_WIDTH g_graphic_dev.XResolution
#define SCREEN_HIGHT g_graphic_dev.YResolution

#define X_DIVISION  3
#define Y_DIVISION  1

#define BLOCK_WIDTH SCREEN_WIDTH / X_DIVISION
#define BLOCK_HIGHT SCREEN_HIGHT / Y_DIVISION
```
## 2.创建具体的排序线程
有了上述的参数结构体后，我们在创建一个具体的排序线程时就很方便了，只需调用一个sortThreadRun函数并向其传递前面结构体生成器返回的结构体即可：
```
// 创建并运行指定排序线程的函数
inline
int sortThreadRun(sortAttributes *sortAttr)
{
	// 申请用户栈
    unsigned int stack_size = 1024 * 1024;
    sortAttr->stack_sort = (unsigned char*)malloc(stack_size);

    // 创建并运行线程
    int tid_sort = task_create(sortAttr->stack_sort + stack_size, &sort_runner, (void *)sortAttr);

    return tid_sort;
}
```
注意到我在实现这个函数的时候为了避免函数的调用过程将他定义为了内联函数。同时该函数会返回创建线程的线程号，方便后续对该线程的调度等过程。

sort_runner函数实现如下：
```
// 统一的线程函数
void sort_runner(void *pv)
{
    // 取出需执行的函数及其参数
    sortAttributes *attr = (sortAttributes *)pv;
    // 执行该函数
    attr->sortFun(attr->randNumber, attr->size, attr->colorSort, attr->X_Location, attr->Y_Location);
    // 线程退出
    task_exit(0);
}
```

## 3.等待排序线程的结束
等待排序线程的结束只需利用了一个系统调用（task_wait函数）和sortThreadRun函数返回的线程号即可
```
task_wait(tid, NULL);
```

## 4.资源的释放
在释放资源之前，我们先回想一下在上述过程的那些地方我们申请了内存空间：
（1）在生成结构体时对参数结构体进行了内存请求
（2）在生成结构体时对排序数组进行了内存请求
（3）在创建线程时申请了用户栈空间
所以在释放资源时上述三处资源都需要释放，好在它们都与参数结构体相关，所以我实现了一个sortFree函数完成整个释放过程：
```
void sortFree(sortAttributes *sortAttr)
{
    free(sortAttr->randNumber);
    free(sortAttr->stack_sort);
    free(sortAttr);
    sortAttr = NULL;
}
```

##5.具体绘图过程
具体的绘图过程使用了一个drawlines函数，在排序函数中合适的位置调用它即可：
```
void drawLines(int ar[], int size, int X_Location, int Y_Location, int colorSort)
{
	// 计算起始位置
    int startPoint_X = X_Location * BLOCK_WIDTH;
    int startPoint_Y = Y_Location * BLOCK_HIGHT;
	// 计算结束位置
    int endPoint_X = (X_Location + 1) * BLOCK_WIDTH;
    int endPoint_Y = startPoint_Y;

    if(colorSort == 0) // 不按颜色排序
    {
        for (int k = 0; k < size; ++k)
        {
            refreshArea(startPoint_X, startPoint_Y + k, endPoint_X, endPoint_Y + k, 0x000000);
            line(startPoint_X, startPoint_Y + k, startPoint_X + ar[k], endPoint_Y + k, 0xffffff);
        }
    }
    else  // 按颜色排序
    {
        for (int k = 0; k < size; ++k)
        {
            // 转换rgb值
            rgb_struct rgb = Hue2rgb(ar[k]);

            refreshArea(startPoint_X, startPoint_Y + k, endPoint_X, endPoint_Y + k, 0x000000);
            line(startPoint_X, startPoint_Y + k, startPoint_X + BLOCK_WIDTH, endPoint_Y + k, RGB(rgb.ri, rgb.gi, rgb.bi));
        }
    }
}
```
注意在转换rgb时调用了Hue2rgb函数，这个函数是我根据色相和RGB转换关系
（https://www.cnblogs.com/wang726zq/p/3660581.html）实现的函数
```
rgb_struct Hue2rgb(int hue)
{
    // 色环一圈360度
    hue = hue % 360;
    // 色相和RGB折线图的斜率
    float slope = 255.0 / 60;
    // 三个色值
    int ri, gi, bi;

    // 根据色相和rgb折线图
    if(hue <= 60)
    {
        ri = 255;
        gi = slope * hue;
        bi = 0;
    }
    else if(hue <= 120)
    {
        ri = 255 - slope * (hue - 60);
        gi = 255;
        bi = 0;
    }
    else if(hue <= 180)
    {
        ri = 0;
        gi = 255;
        bi = slope * (hue - 120);
    }
    else if(hue <= 240)
    {
        ri = 0;
        gi = 255 - (hue - 180) * slope;
        bi = 255;
    }
    else if(hue <= 300)
    {
        ri = (hue - 240) * slope;
        gi = 0;
        bi = 255;
    }
    else if(hue <= 360)
    {
        ri = 255;
        gi = 0;
        bi = 255 - (hue - 300) * slope;
    }

    rgb_struct rgb;
    rgb.ri=ri;
    rgb.gi=gi;
    rgb.bi=bi;

    return rgb;
}
```
同时我也把rgb转色相的函数贴出来吧：
```
int rgb2Hue(rgb_struct rgb)
{
    float hsb[3];
    int maxIndex = 0, minIndex = 0;

    //将rgb的值从小到大排列，存在ranged数组里
    int rangedRGB[3];
    rangedRGB[0] = (int)minRGB((float)rgb.ri, (float)rgb.gi, (float)rgb.bi);
    rangedRGB[2] = (int)maxRGB((float)rgb.ri, (float)rgb.gi, (float)rgb.bi);
    rangedRGB[1] = rgb.ri + rgb.gi + rgb.bi - rangedRGB[0] - rangedRGB[2];

    //rgb的索引分别为0、1、2，maxIndex和minIndex用于存储rgb中最大最小值的下标
    if(rangedRGB[0] == rgb.ri)
        minIndex = 0;
    else if(rangedRGB[0] == rgb.gi)
        minIndex = 1;
    else
        minIndex = 2;

    if(rangedRGB[2] == rgb.ri)
        maxIndex = 0;
    else if(rangedRGB[2] == rgb.gi)
        maxIndex = 1;
    else
        maxIndex = 2;

    //算出亮度
    hsb[2] = rangedRGB[2] / 255.0f;
    //算出饱和度
    hsb[1]=1-rangedRGB[0]/rangedRGB[2];
    //算出色相
    hsb[0]=maxIndex*120+60* (rangedRGB[1]/hsb[1]/rangedRGB[2]+(1-1/hsb[1])) *((maxIndex-minIndex+3)%3==1?1:-1);
    //防止色相为负值
    hsb[0] = ((int)hsb[0] + 360) % 360;
    return (int)hsb[0];
}
```
最后贴一张排序结果的图吧
![排序结果](https://img-blog.csdn.net/20180419114658328?watermark/2/text/aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L0F1Z3VzdF8xOTk3/font/5a6L5L2T/fontsize/400/fill/I0JBQkFCMA==/dissolve/70)

</br>

# 实验三 线程的调度
在这个实验中，我们需要先将系统原有的轮转调度算法改为优先调度算法，然后创建两个能用键盘控制优先级调度的排序线程。</br>

## 1. 优先级调度器：
&emsp; 首先给出优先级调度器的具体实现。在这个调度器中，我们将完成选出当前优先级最高的就绪线程并调度运行的工作。需要注意的是，task0只能在系统没有其他任何可调度线程的时候进行调度。</br>
```
// 优先调度算法版
void schedule()
{
    struct tcb *select = NULL;
    int maxPriority = -1;

    // 遍历整个线程队列
    for(struct tcb *tsk = g_task_head; tsk != NULL; tsk = tsk->next)
    {
        // 计算每个线程的动态优先级
        tsk->priority = PRI_USER_MAX - fixedpt_toint(fixedpt_div(tsk->estcpu,fixedpt_fromint(4))) - tsk->nice * 2;

        // 截断
        // if(tsk->priority > PRI_USER_MAX)
        //     tsk->priority = PRI_USER_MAX;
        // if(tsk->priority < PRI_USER_MIN)
        //     tsk->priority = PRI_USER_MIN;

        // 避免运行task0
        if(tsk->tid == 0)
            tsk->priority = -1;

        // 及时响应系统主线程
        if(tsk->tid == 1 && tsk->state == TASK_STATE_READY)
        { 
            switch_to(tsk);
            return;
        }

        // 判断优先级
        if(tsk->priority > maxPriority && tsk->state == TASK_STATE_READY)
        {
            select = tsk;
            maxPriority = tsk->priority;
        }
    }

    // 选择为空 无其他线程可运行
    if(select == NULL)
        select = task0;

    if(select->signature != TASK_SIGNATURE)
        printk("warning: kernel stack of task #%d overflow!!!", select->tid);

    g_resched = 0;
    switch_to(select);
}
```

## 2. 优先级相关的系统调用
&emsp; 要完成在用户态下对线程优先级的修改，需要添加两个系统调用，这两个调度将用来获取和修改指定线程的静态优先级(即nice值)。用户态中系统调用函数的声明如下：
```
int get_priority(int tid);
int set_priority(int tid, int prio);
```
&emsp; 函数的实现在kernel/machdep.c，需要注意静态优先级(nice值)取值范围为-20~19并且是值越小优先级越高, 而动态优先级(priority值)取值范围是0~39并且是值越大优先级越高。而且get_priority应该的返回的是动态优先级值，传递给set_priority的值也应该是动态优先级值。
```
// #define NZERO 20
// 获取线程静态优先级
int sys_get_priority(int tid)
{
    uint32_t flags;
    struct tcb * tsk;

    if(tid == 0)    // tid为0，获取当前线程的nice值
        tid = g_task_running->tid;
    
    save_flags_cli(flags);      // 关中断
    tsk = get_task(tid);        // 取tid
    restore_flags(flags);       // 开中断

    return tsk->nice + NZERO;
}

// 设置线程静态优先级
// 成功返回0，失败返回-1
int sys_set_priority(int tid, int prio)
{
    int ret = -1;
    uint32_t flags;
    struct tcb * tsk;
    
    if(tid == 0)    // tid为0，设置当前线程的nice值
        tid = g_task_running->tid;

    if(prio >= 0 && prio < 2 * NZERO - 1)
    {
        save_flags_cli(flags);       // 关中断
        tsk = get_task(tid);         // 取tid
        restore_flags(flags);        // 开中断

        tsk->nice = prio - NZERO; // 修改优先级
        ret = 0;
    }

    return ret;
}
```

# 3. 排序线程的调度
&emsp; 先给出调度排序线程的效果图吧
![排序示意图](https://img-blog.csdn.net/20180502224200343?watermark/2/text/aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L0F1Z3VzdF8xOTk3/font/5a6L5L2T/fontsize/400/fill/I0JBQkFCMA==/dissolve/70)
&emsp; 图中间的两个柱子用来表示两边排序线程的优先级高低，使用方向键进行调整。

&emsp; 两边排序线程的创建步骤同实验二一样，具体如下：
```
// 两个线程都是用冒泡排序 右边按色彩排序
sortAttributes *bubbleAttr_left = attrGenerator(bubbleSort, 0, 0, 0);
sortAttributes *bubbleAttr_right = attrGenerator(bubbleSort, 1, 2, 0);
// 获取线程id号
int tid_bubble_left  = sortThreadRun(bubbleAttr_left);
int tid_bubble_right = sortThreadRun(bubbleAttr_right);
```

&emsp; 接下来对左右两个线程以及中间两个进度条进行初始化
```
    // 初始化优先级
    int key = 0;
    int curr_left_nice  = -10;
    int curr_right_nice = -10;

    set_priority(tid_bubble_left, curr_left_nice + NZERO);
    set_priority(tid_bubble_right, curr_right_nice + NZERO);

    // 将屏幕竖直划分为40块，代表40级静态优先级
    double levelHight = SCREEN_HIGHT / (2 * NZERO);

    // 左边进度条X轴开始位置 和 右边进度条X轴结束位置
    int left_start = SCREEN_WIDTH / 2 - BLOCK_WIDTH / 2;
    int right_end  = SCREEN_WIDTH / 2 + BLOCK_WIDTH / 2;

    uint32_t left_color = 0xffff6f;
    uint32_t right_color = 0x2cff68;

    // 画出进度条
    refreshArea(left_start, levelHight * (curr_left_nice + NZERO),
                    SCREEN_WIDTH / 2, BLOCK_HIGHT, left_color);
    refreshArea(SCREEN_WIDTH / 2, levelHight * (curr_right_nice + NZERO),
                    right_end, BLOCK_HIGHT, right_color);
```



