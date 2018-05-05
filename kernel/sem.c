/**
 * vim: filetype=c:fenc=utf-8:ts=4:et:sw=4:sts=4
 */
#include <stddef.h>
#include "kernel.h"

// 信号量结构
typedef struct semaphore_type
{
    int sid;    // 信号量id
    int value;  // 计数器
    struct wait_queue *sem_queue;    // 等待队列
    struct semaphore_type *next;     // 信号量链表
} sem_t;

// 信号量链表：头结点透明的单链表
static sem_t *sem_link_head;

// 测试用遍历输出
// static void test_and_see()
// {
//     int count = 0;
//     for (sem_t *runner = sem_link_head; runner != NULL; 
//             runner = runner->next, ++count)
//         printk("sem: id #%d\tvalue: %d\t\n", runner->sid, runner->value);
//     printk("total :%d\n", count - 1);
// }

// 按信号量id查找目标信号量的上一个
static inline
sem_t* find_sem_prev(int semid)
{
    sem_t *sem_walker = sem_link_head;

    // 遍历信号量链表
    while(sem_walker->next != NULL && sem_walker->next->sid != semid)
        sem_walker = sem_walker->next;
    
    // 不存在
    if(sem_walker->next == NULL)
        return NULL;
    else
        return sem_walker;
}

int sys_sem_create(int value)
{
    // 创建一个信号量
    sem_t *sem = kmalloc(sizeof(sem_t));
    sem->value = value;
    sem->sem_queue = NULL;
    sem->next = NULL;
    
    // 初始化头结点
    if(sem_link_head == NULL)
    {
        sem_link_head = kmalloc(sizeof(sem_t));
        sem_link_head->sid = -1;
        sem_link_head->value = -1;
        sem->sem_queue = NULL;
        sem_link_head->next = NULL;
    }

    int id = 0;
    sem_t *sem_walker = sem_link_head;

    // 遍历信号量链表 找到最后一个
    while(sem_walker->next) 
    {
        ++id;
        sem_walker = sem_walker->next;
    }

    // 给末尾的空节点赋值
    sem->sid = id;
    sem_walker->next = sem;

    return sem->sid;
}

int sys_sem_destroy(int semid)
{
    // 查找
    sem_t *sem_target = find_sem_prev(semid);

    // 不存在此id
    if(find_sem_prev(semid) == NULL)
        return -1;

    // 保存要删除的结点，便于释放内存
    sem_t *sem_temp = sem_target->next;
    // 重构链表
    sem_target->next = sem_target->next->next;
    // 释放空间
    kfree(sem_temp);
    sem_temp = NULL;

    return 0;
}

int sys_sem_wait(int semid)
{
    printk("sys sem wait...\n");
    // 查找
    sem_t *sem_target = find_sem_prev(semid);

    // 不存在此id
    if(find_sem_prev(semid) == NULL)
        return -1;

    sem_target = sem_target->next;

    --sem_target->value;
    if(sem_target->value < 0) // 阻塞
    {
        // 插入到队尾
        struct wait_queue *wq_runner = sem_target->sem_queue;
        printk("sem: id #%d  value %d\n", sem_target->sid, sem_target->value);
        
        do
        {
            if(wq_runner)
                wq_runner = wq_runner->next;
        } while (wq_runner);

        if(wq_runner == sem_target->sem_queue)
        {
            sem_target->sem_queue = kmalloc(sizeof(struct wait_queue));
            sem_target->sem_queue->next = NULL;
        }
        else
        {
            wq_runner = kmalloc(sizeof(struct wait_queue));
            wq_runner->tsk = get_task(sys_task_getid());
            wq_runner->next = NULL;
        }

        if(!sem_target->sem_queue)
            printk("queue is NULL...\n");

        uint32_t flags;
        save_flags_cli(flags);
        printk("try block tid #%d...\n", sys_task_getid());
        sleep_on(&(sem_target->sem_queue));  // 进程睡眠
        restore_flags(flags);
    }
    printk("unblock tid #%d...\n", sys_task_getid());
    printk("sem: id #%d  value %d\n", sem_target->sid, sem_target->value);
    return 0;
}

int sys_sem_signal(int semid)
{
    printk("sys sem signal...\n");
    // 查找
    sem_t *sem_target = find_sem_prev(semid);

    // 不存在此id
    if(find_sem_prev(semid) == NULL)
        return -1;

    sem_target = sem_target->next;
    struct wait_queue *wq = sem_target->sem_queue;

    printk("sem: id #%d  value %d\n", sem_target->sid, sem_target->value);
    ++sem_target->value;
    if(sem_target->value <= 0) // 需要唤醒
    {
        // 取出队列头
        struct wait_queue *wq_head = wq;
        struct wait_queue *wake_target = wq_head;
        if(wake_target == NULL)
            printk("wake target NULL...\n");
        if(wake_target)
        {
            wq_head = wq_head->next;

            printk("try signal tid #%d...\n", wake_target->tsk->tid);
            uint32_t flags;
            save_flags_cli(flags);
            wake_up(&wake_target, 1);
            restore_flags(flags);
        }
    }
    printk("sem: id #%d  value %d\n", sem_target->sid, sem_target->value);
    printk("singaled...\n");
    return 0;
}

