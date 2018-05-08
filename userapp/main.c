/*
 * vim: filetype=c:fenc=utf-8:ts=4:et:sw=4:sts=4
 */
#include <inttypes.h>
#include <stddef.h>
#include <math.h>
#include <stdio.h>
#include <sys/mman.h>
#include <syscall.h>
#include <netinet/in.h>
#include <stdlib.h>
#include "graphics.h"
#include "sort.h"

#define NZERO 20

#define true    1
#define false   0

extern void *tlsf_create_with_pool(void* mem, size_t bytes);
extern void *g_heap;

// 实验运行
void Lab1_Go();
void Lab2_Go();
void Lab3_Go();
void Lab4_Go();

/**
 * GCC insists on __main
 *    http://gcc.gnu.org/onlinedocs/gccint/Collect2.html
 */
void __main()
{
    size_t heap_size = 32*1024*1024;
    void  *heap_base = mmap(NULL, heap_size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);
	g_heap = tlsf_create_with_pool(heap_base, heap_size);
}

/**
 * 第一个运行在用户模式的线程所执行的函数
 */
void main(void *pv)
{
    printf("task #%d: I'm the first user task(pv=0x%08x)!\r\n",
            task_getid(), pv);

    //TODO: Your code goes here
    // list_graphic_modes();

    Lab4_Go();

    while (1)
        ;
    task_exit(0);
}

// ----------------------------------------------------------------------------------------------------
// 实验一 系统时间调用函数
void Lab1_Go()
{
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
}
// ----------------------------------------------------------------------------------------------------


// ----------------------------------------------------------------------------------------------------
// 实验二 线程的创建
void Lab2_Go()
{
    // 屏幕尺寸可更换
    // list_graphic_modes();
    init_graphic(0x0192);

    // 设置各排序函数参数
    // (排序函数地址， 是否色彩排序， X轴分块位置， Y轴分块位置)
    sortAttributes *bubbleAttr = attrGenerator(&bubbleSort, 0, 0, 0);
    sortAttributes *selectAttr = attrGenerator(&selectSort, 0, 1, 0);
    sortAttributes *insertAttr = attrGenerator(&insertSort, 0, 2, 0);
    sortAttributes *quickAttr  = attrGenerator(&quickSort, 0, 1, 2);
    
    sortAttributes *mergeColorAttr = attrGenerator(&mergeSort, 1, 0, 1);
    sortAttributes *quickColorAttr = attrGenerator(&quickSort, 1, 1, 1);
    sortAttributes *shellColorAttr = attrGenerator(&shellSort, 1, 2, 1);
    sortAttributes *bubbleColorAttr= attrGenerator(&bubbleSort, 1, 2, 2);
    sortAttributes *insertColorAttr  = attrGenerator(&insertSort, 1, 0, 2);

    // 运行各排序
    int tid_quick  = sortThreadRun(quickAttr);
    int tid_bubble = sortThreadRun(bubbleAttr);
    int tid_select = sortThreadRun(selectAttr);
    int tid_insert = sortThreadRun(insertAttr);

    int tid_insertColor= sortThreadRun(insertColorAttr);
    int tid_mergeColor = sortThreadRun(mergeColorAttr);
    int tid_quickColor = sortThreadRun(quickColorAttr);
    int tid_shellColor = sortThreadRun(shellColorAttr);
    int tid_bubbleColor= sortThreadRun(bubbleColorAttr);

    // 等待结束
    task_wait(tid_quick, NULL);
    task_wait(tid_bubble, NULL);
    task_wait(tid_select, NULL);
    task_wait(tid_insert, NULL);
    
    task_wait(tid_insertColor, NULL);
    task_wait(tid_mergeColor, NULL);
    task_wait(tid_quickColor, NULL);
    task_wait(tid_shellColor, NULL);
    task_wait(tid_bubbleColor, NULL);

    // 释放资源
    sortFree(quickAttr);
    sortFree(bubbleAttr);
    sortFree(selectAttr);
    sortFree(insertAttr);
    
    sortFree(insertColorAttr);
    sortFree(mergeColorAttr);
    sortFree(quickColorAttr);
    sortFree(shellColorAttr);
    sortFree(bubbleColorAttr);

    sleep(100);
    exit_graphic();
    printf("exited!");
}
// ----------------------------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------------------------
// 实验三 线程调度

// 需要将graphic.h中的X_DIVISION和Y_DIVISION宏定义更改为3和1
void Lab3_Go()
{
    init_graphic(0x0180);

    sortAttributes *bubbleAttr_left = attrGenerator(&bubbleSort, 0, 0, 0);
    sortAttributes *bubbleAttr_right = attrGenerator(&bubbleSort, 1, 2, 0);

    int tid_bubble_left  = sortThreadRun(bubbleAttr_left);
    int tid_bubble_right = sortThreadRun(bubbleAttr_right);

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

    while((key = getchar()))
    {
        curr_left_nice  = get_priority(tid_bubble_left) - NZERO;
        curr_right_nice = get_priority(tid_bubble_right) - NZERO;

        switch(key)
        {
            // UP键 减小nice值 调高左边进程优先级
            case 0x4800:
                curr_left_nice = (curr_left_nice <= -NZERO) ?
                                        -NZERO : curr_left_nice - 1;
                set_priority(tid_bubble_left, curr_left_nice + NZERO);
                refreshArea(left_start, levelHight * (curr_left_nice + NZERO),
                                SCREEN_WIDTH / 2, BLOCK_HIGHT, left_color);
                break;
            // DOWN键 增大nice值 调低左边进程优先级
            case 0x5000:
                curr_left_nice = (curr_left_nice >= NZERO - 1) ?
                                        NZERO - 1 : curr_left_nice + 1;
                set_priority(tid_bubble_left, curr_left_nice + NZERO);
                refreshArea(left_start, 0, SCREEN_WIDTH / 2,
                                levelHight * (curr_left_nice + NZERO), 0x000000);
                break;
            // RIGHT键 减小nice值 调高右边进程优先级
            case 0x4d00:
                curr_right_nice = (curr_right_nice <= -NZERO) ?
                                        -NZERO : curr_right_nice - 1;
                set_priority(tid_bubble_right, curr_right_nice + NZERO);
                refreshArea(SCREEN_WIDTH / 2, levelHight * (curr_right_nice + NZERO),
                                right_end, BLOCK_HIGHT, right_color);
                break;
            // LEFT键 增大nice值 调低右边进程优先级
            case 0x4b00:
                curr_right_nice = (curr_right_nice >= NZERO - 1) ?
                                        NZERO - 1 : curr_right_nice + 1;
                set_priority(tid_bubble_right, curr_right_nice + NZERO);
                refreshArea(SCREEN_WIDTH / 2, 0, right_end,
                                levelHight * (curr_right_nice + NZERO), 0x000000);
                break;
            default:
                continue;
        }
    };

    task_wait(tid_bubble_left, NULL);
    task_wait(tid_bubble_right, NULL);

    sortFree(bubbleAttr_left);
    sortFree(bubbleAttr_right);

    bubbleAttr_left = NULL;
    bubbleAttr_right = NULL;
}
// ----------------------------------------------------------------------------------------------------


// ----------------------------------------------------------------------------------------------------
// 实验四 线程同步
 
// 记录排序链表
// 头结点不为空的链表
struct sortAttr_queue
{
    sortAttributes *attr;
    struct sortAttr_queue *next;
} sortAttr_queue;

// 排序数量和链表头
#define MAX_SORT 8
int sortAttrCnt;
struct sortAttr_queue *sortAttr_queue_head;

// 信号量
int sem_empty, sem_full, sem_mutex;

void task_producer(void *arg);
void task_consumer(void *arg);
inline void add2queue(sortAttributes* attr);
inline sortAttributes* removeHead();
extern void control_priority(int tid_left, int tid_right);

#define p(x) sem_wait(x)
#define v(x) sem_signal(x)

void Lab4_Go()
{
    init_graphic(0x0180);

    sem_empty= sem_create(MAX_SORT);
    sem_full = sem_create(0);
    sem_mutex= sem_create(1);

    unsigned char *stack_producer, *stack_consumer;
    unsigned int stack_size = 1024 * 1024;
    stack_producer = (unsigned char *)malloc(stack_size);
    stack_consumer = (unsigned char *)malloc(stack_size);
    
    int tid_producer = task_create(stack_producer + stack_size, &task_producer, (void *)0);
    int tid_consumer = task_create(stack_consumer + stack_size, &task_consumer, (void *)0);

    control_priority(tid_producer, tid_consumer);

    task_wait(tid_producer, NULL);
    task_wait(tid_consumer, NULL);

    free(stack_producer);
    free(stack_consumer);

    stack_producer = NULL;
    stack_producer = NULL;
    return;
}

void task_producer(void *arg)
{
    int loc = 0;
    while (true)
    {
        p(sem_empty);
        sortAttributes *shellSortAttr = attrGenerator(&shellSort, loc % 2, loc % 8, 0);
        sortAttributes *mergeSortAttr = attrGenerator(&mergeSort, (loc + 1) % 2 , loc % 8 + 1, 0);
        
        drawLines(shellSortAttr->randNumber, BLOCK_HIGHT, loc % 8, 0, loc % 2);
        add2queue(shellSortAttr);
        v(sem_full);

        p(sem_empty);
        drawLines(mergeSortAttr->randNumber, BLOCK_HIGHT, loc % 8 + 1, 0, (loc + 1) % 2);
        add2queue(mergeSortAttr);
        v(sem_full);
        
        loc += 2;
        // sleep(1);
    }
}

void task_consumer(void *arg)
{
    while(true)
    {
        for (int i = 0; i < 3; ++i)
        {
            p(sem_full);
            sortAttributes *temp = removeHead();
            
            int tid = sortThreadRun(temp);
            task_wait(tid, NULL);
            
            sortFree(temp);
            v(sem_empty);
        }

    }
}

inline void add2queue(sortAttributes* attr)
{
    if(!sortAttr_queue_head)    // 第一次创建
    {
        sortAttr_queue_head = malloc(sizeof(sortAttr_queue));
        sortAttr_queue_head->attr = attr;
        sortAttr_queue_head->next = NULL;
        return;
    }

    struct sortAttr_queue *attr_runner = sortAttr_queue_head;

    while (attr_runner->next)
        attr_runner = attr_runner->next;

    attr_runner->next = malloc(sizeof(sortAttr_queue));
    attr_runner = attr_runner->next;
    attr_runner->attr = attr;
    attr_runner->next = NULL;
}

inline sortAttributes* removeHead()
{
    if(sortAttr_queue_head==NULL)
        return NULL;

    struct sortAttr_queue *head = sortAttr_queue_head;
    sortAttr_queue_head = sortAttr_queue_head->next;

    sortAttributes *attr = malloc(sizeof(sortAttributes));
    *attr = *head->attr;
    free(head);

    return attr;
}

void control_priority(int tid_left, int tid_right)
{
    int curr_left_nice  = -10;
    int curr_right_nice = 10;

    set_priority(tid_left, curr_left_nice + NZERO);
    set_priority(tid_right, curr_right_nice + NZERO);

    // 将屏幕竖直划分为40块，代表40级静态优先级
    double levelHight = SCREEN_HIGHT / (2 * NZERO);

    // 左边进度条X轴开始位置 和 右边进度条X轴结束位置
    int left_start = SCREEN_WIDTH - BLOCK_WIDTH;
    int right_end  = SCREEN_WIDTH;

    uint32_t left_color = 0xffff6f;
    uint32_t right_color = 0x2cff68;

    // 画出进度条
    refreshArea(left_start, levelHight * (curr_left_nice + NZERO),
                    SCREEN_WIDTH - BLOCK_WIDTH / 2, BLOCK_HIGHT, left_color);
    refreshArea(right_end - BLOCK_WIDTH / 2, levelHight * (curr_right_nice + NZERO),
                    right_end, BLOCK_HIGHT, right_color);

    int key = 0;
    while((key = getchar()))
    {
        curr_left_nice  = get_priority(tid_left) - NZERO;
        curr_right_nice = get_priority(tid_right) - NZERO;

        switch(key)
        {
            // UP键 减小nice值 调高左边进程优先级
            case 0x4800:
                curr_left_nice = (curr_left_nice <= -NZERO) ?
                                        -NZERO : curr_left_nice - 1;
                set_priority(tid_left, curr_left_nice + NZERO);
                refreshArea(left_start, levelHight * (curr_left_nice + NZERO),
                                left_start + BLOCK_WIDTH / 2, BLOCK_HIGHT, left_color);
                break;
            // DOWN键 增大nice值 调低左边进程优先级
            case 0x5000:
                curr_left_nice = (curr_left_nice >= NZERO - 1) ?
                                        NZERO - 1 : curr_left_nice + 1;
                set_priority(tid_left, curr_left_nice + NZERO);
                refreshArea(left_start, 0, left_start + BLOCK_WIDTH / 2,
                                levelHight * (curr_left_nice + NZERO), 0x000000);
                break;
            // RIGHT键 减小nice值 调高右边进程优先级
            case 0x4d00:
                curr_right_nice = (curr_right_nice <= -NZERO) ?
                                        -NZERO : curr_right_nice - 1;
                set_priority(tid_right, curr_right_nice + NZERO);
                refreshArea(right_end - BLOCK_WIDTH / 2, levelHight * (curr_right_nice + NZERO),
                                right_end, BLOCK_HIGHT, right_color);
                break;
            // LEFT键 增大nice值 调低右边进程优先级
            case 0x4b00:
                curr_right_nice = (curr_right_nice >= NZERO - 1) ?
                                        NZERO - 1 : curr_right_nice + 1;
                set_priority(tid_right, curr_right_nice + NZERO);
                refreshArea(right_end - BLOCK_WIDTH / 2, 0, right_end,
                                levelHight * (curr_right_nice + NZERO), 0x000000);
                break;
            default:
                continue;
        }
    };
}
// ----------------------------------------------------------------------------------------------------
