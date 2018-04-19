#ifndef _SORT_H_
#define _SORT_H_

#include <stdio.h>
#include <stdlib.h>
#include "time.h"
#include "syscall.h"
#include "graphics.h"

typedef void (*sortFunction)(int ar[], int sz, int crSort, int X_Loc, int Y_Loc);

// 传递给线程函数的结构体
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

// 线程睡眠
int msleep(const uint32_t msec);

// 排序线程函数
void sort_runner(void *pv);

// 构造排序所需要的参数
sortAttributes *attrGenerator(sortFunction sortFun, int colorSort, int X_Location, int Y_Location);

// 创建并运行指定排序线程的函数
inline
int sortThreadRun(sortAttributes *sortAttr)
{
    unsigned int stack_size = 1024 * 1024;
    sortAttr->stack_sort = (unsigned char*)malloc(stack_size);

    // 创建并运行线程
    int tid_sort = task_create(sortAttr->stack_sort + stack_size, &sort_runner, (void *)sortAttr);

    return tid_sort;
}

// 释放空间
void sortFree(sortAttributes *sortAttr);

// 交换
void swap(int *a, int *b);

// 冒泡
void bubbleSort(int ar[], int size, int colorSort, int X_Location, int Y_Location);

// 选择
void selectSort(int ar[], int size, int colorSort, int X_Location, int Y_Location);

// 插入
void insertSort(int ar[], int size, int colorSort, int X_Location, int Y_Location);

// 希尔
void shellSort(int ar[], int size, int colorSort, int X_Location, int Y_Location);

// 快排
void quickSort(int ar[], int size, int colorSort, int X_Location, int Y_Locateion);
void quickFun(int *a, int left, int right, int colorSort, int X_Location, int Y_Locateion, int size);

// 归并
void mergeSort(int ar[], int size, int colorSort, int X_Location, int Y_Location);

// 堆排
void heapSort(int ar[], int size, int colorSort, int X_Location, int Y_Location);

#endif // defined _SORT_H_