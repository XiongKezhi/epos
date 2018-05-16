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
    struct wait_queue *sem_queue;    // 等待队列 头结点为空
    struct semaphore_type *next;     // 信号量链表
} sem_t;

// 信号量链表：头结点为空的单链表
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
    if(!sem_link_head)
        return NULL;

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
    // 设置信号量链表
    sem->value = value;
    sem->next = NULL;
    // 设置等待队列
    sem->sem_queue = kmalloc(sizeof(struct wait_queue));
    sem->sem_queue->tsk = NULL;
    sem->sem_queue->next = NULL;

    // 初始化头结点
    if(sem_link_head == NULL)
    {
        sem_link_head = kmalloc(sizeof(sem_t));
        sem_link_head->sid = -1;
        sem_link_head->value = -1;
        sem_link_head->next = NULL;
    }

    int id = 1;
    sem_t *sem_walker = sem_link_head;

    uint32_t flags;
    save_flags_cli(flags);
    // 遍历信号量链表 找到最后一个
    while(sem_walker->next) 
    {
        ++id;
        sem_walker = sem_walker->next;
    }
    restore_flags(flags);

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

    uint32_t flags;
    save_flags_cli(flags);
    // 保存要删除的结点，释放内存
    sem_t *sem_temp = sem_target->next;
    sem_target->next = sem_target->next->next;
    kfree(sem_temp);
    sem_temp = NULL;
    restore_flags(flags);

    return 0;
}

int sys_sem_wait(int semid)
{
    // printk("sys sem wait, semid #%d...\n", semid);
    // 查找
    sem_t *sem_target = find_sem_prev(semid);
    sem_target = sem_target->next;

    // 不存在此id
    if(!sem_target)
        return -1;

    --sem_target->value;
    if(sem_target->value < 0) // 阻塞
    {
        // 插入到等待队列队尾
        struct wait_queue *wq_runner = sem_target->sem_queue;
        // printk("sem: id #%d  value %d\n", sem_target->sid, sem_target->value);
        
        uint32_t flags;
        save_flags_cli(flags);
        do
        {
            if(wq_runner)
                wq_runner = wq_runner->next;
        } while (wq_runner);

        wq_runner = kmalloc(sizeof(struct wait_queue));
        wq_runner->tsk = get_task(sys_task_getid());
        wq_runner->next = NULL;
        restore_flags(flags);

        // printk("try block tid #%d...\n", sys_task_getid());
        save_flags_cli(flags);
        sleep_on(&(sem_target->sem_queue));  // 进程睡眠
        restore_flags(flags);
    }
    // printk("unblock tid #%d...\n", sys_task_getid());
    // printk("sem: id #%d  value %d\n", sem_target->sid, sem_target->value);
    return 0;
}

int sys_sem_signal(int semid)
{
    // printk("sys sem signal, semid %d...\n", semid);
    // 查找
    sem_t *sem_target = find_sem_prev(semid);

    // 不存在此id
    if(find_sem_prev(semid) == NULL)
        return -1;
    
    uint32_t flags;
    save_flags_cli(flags);
    sem_target = sem_target->next;
    struct wait_queue *wq = sem_target->sem_queue;
    restore_flags(flags);

    ++sem_target->value;
    if(sem_target->value <= 0) // 需要唤醒
    {
       // printk("try signal tid #%d...\n", wake_target->tsk->tid);
       save_flags_cli(flags);
       wake_up(&wq, 1);
       restore_flags(flags);
    }
    // printk("sem: id #%d  value %d\n", sem_target->sid, sem_target->value);
    // printk("singaled...\n");
    return 0;
}

