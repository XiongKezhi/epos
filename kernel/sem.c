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
static void test_and_see()
{
    int count = 0;
    for (sem_t *runner = sem_link_head; runner != NULL; 
            runner = runner->next, ++count)
        printk("sem: id #%d\tvalue: %d\t\n", runner->sid, runner->value);
    printk("total :%d\n", count - 1);
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

    // printk("sem : id #%u\tvalue #%d\n", sem->sid, sem->value);
    return sem->sid;
}

int sys_sem_destroy(int semid)
{
    sem_t *sem_walker = sem_link_head;

    // 遍历信号量链表
    while(sem_walker->next != NULL && sem_walker->next->sid != semid)
        sem_walker = sem_walker->next;

    // 不存在此id
    if(sem_walker->next == NULL)
        return -1;

    // 保存要删除的结点，便于释放内存
    sem_t *sem_temp = sem_walker->next;
    // 重构链表
    sem_walker->next = sem_walker->next->next;
    // 释放空间
    kfree(sem_temp);
    sem_temp = NULL;

    test_and_see();
    return 0;
}

int sys_sem_wait(int semid)
{
    return -1;
}

int sys_sem_signal(int semid)
{
    return -1;
}

