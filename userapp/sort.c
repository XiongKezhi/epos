#include "sort.h"

int msleep(const uint32_t msec)
{
    struct timespec ts = {0, 1000000 * msec};
    return nanosleep(&ts, NULL);
}

void sort_runner(void *pv)
{
    // 取出需执行的函数及其参数
    sortAttributes *attr = (sortAttributes *)pv;
    // 执行该函数
    attr->sortFun(attr->randNumber, attr->size, attr->colorSort, attr->X_Location, attr->Y_Location);
    // 线程退出
    task_exit(0);
}

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

void sortFree(sortAttributes *sortAttr)
{
    free(sortAttr->randNumber);
    free(sortAttr->stack_sort);
    free(sortAttr);
    sortAttr = NULL;
}

void swap(int *a, int *b)
{
    *b = *a + *b;
    *a = *b - *a;
    *b = *b - *a;
}

void bubbleSort(int ar[], int size, int colorSort, int X_Location, int Y_Location)
{
    for (int i = 0; i < size; ++i)
    {
        for (int j = size; j > i; --j)
            if(ar[j] < ar[j-1])
                swap(ar + j, ar + j - 1);

        drawLines(ar, size, X_Location, Y_Location, colorSort);
    }
}

void selectSort(int ar[], int size, int colorSort, int X_Location, int Y_Location)
{
    for (int i = 0; i < size; i++)
	{
		int minIndex = i;	
        for (int j = size - 1; j > i; j--)
			if (ar[j] < ar[minIndex])
				minIndex = j;
        swap(ar + minIndex, ar + i);

        drawLines(ar, size, X_Location, Y_Location, colorSort);
    }
}

void insertSort(int ar[], int size, int colorSort, int X_Location, int Y_Location)
{ 
	for (int i = 0; i < size; i++)
    {
		for (int j = i; j > 0 && ar[j] < ar[j-1]; j--)
            swap(ar + j, ar + j - 1);

        drawLines(ar, size, X_Location, Y_Location, colorSort);
    }
    
}

void shellSort(int ar[], int size, int colorSort, int X_Location, int Y_Location)
{
    int gap, i, j, temp;
    for (gap = size / 2; gap > 0; gap /= 2){
        for (i = gap; i < size; i++){
            for (j = i - gap; j >= 0 && ar[j]>ar[j + gap];j-=gap){
                temp = ar[j];
                ar[j] = ar[j + gap];
                ar[j + gap] = temp;
            }
        }
        msleep(2000);
        drawLines(ar, size, X_Location, Y_Location, colorSort);
    }
}


void quickSort(int ar[], int size, int colorSort, int X_Location, int Y_Locateion)
{
    quickFun(ar, 0, size, colorSort, X_Location, Y_Locateion, size);
    return;
}
void quickFun(int ar[], int left, int right, int colorSort, int X_Location, int Y_Location, int size)
{
    drawLines(ar, size, X_Location, Y_Location, colorSort);
    if (left >= right) 
    {
        return ;
    }
    int i = left;
    int j = right;
    int key = ar[left];
     
    while(i < j)
    {
        while(i < j && key <= ar[j])
            j--;
         
        ar[i] = ar[j];
         
        while(i < j && key >= ar[i])
            i++;

        ar[j] = ar[i];
    }
    
    ar[i] = key;
    quickFun(ar, left, i - 1, colorSort, X_Location, Y_Location, size);
    quickFun(ar, i + 1, right, colorSort, X_Location, Y_Location, size);
}

void mergeSort(int ar[], int size, int colorSort, int X_Location, int Y_Location)  
{  
    int i,next,left_min,left_max,right_min,right_max;  
    int *temp = (int *)malloc(size * sizeof(int));  

    for(i=1; i<size; i*=2)  
    {  

        for(left_min=0; left_min<size-i; left_min = right_max)  
        {  
            right_min = left_max = left_min + i;  
            right_max = left_max + i;  

            if(right_max>size)  
            {  
                right_max = size;  
            }  

            next = 0;  

            while(left_min<left_max&&right_min<right_max)  
            {  
                if(ar[left_min] < ar[right_min])  
                {  
                    temp[next++] = ar[left_min++];  
                      
                }  
                else  
                {  
                    temp[next++] = ar[right_min++];  
                }  
            }  

            while(left_min < left_max)  
                ar[--right_min] = ar[--left_max];  

            while(next>0)  
                ar[--right_min] = temp[--next];        
        }
        msleep(500);
        drawLines(ar, size, X_Location, Y_Location, colorSort);
    }  
}  

void percDown(int a[],int i,int N)  
{  
    int child;  
    int temp;  
  
    for(temp=a[i];(2*i)<N;i=child)  
    {  
        child=2*i;  
        if(child!=N-1&&a[child+1]>a[child])  
            child++;  
        if(temp<a[child])  
            a[i]=a[child];  
        else  
            break;  
    }  
    a[i]=temp;  
}  
void heapSort(int ar[], int size, int colorSort, int X_Location, int Y_Location)  
{  
    int i;  
    for(i=size/2;i>0;i--)
    {  
        percDown(ar,i,size);
        drawLines(ar, size, X_Location, Y_Location, colorSort);
 
    }  
    for(i=size-1;i>0;i--)  
    {  
        swap(&ar[0],&ar[i]);
        percDown(ar,0,i);
        drawLines(ar, size, X_Location, Y_Location, colorSort);
    }  
}
