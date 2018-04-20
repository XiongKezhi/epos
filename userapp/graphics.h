/**
 * vim: filetype=c:fenc=utf-8:ts=4:et:sw=4:sts=4
 *
 * Copyright (C) 2015 Hong MingJian<hongmingjian@gmail.com>
 * All rights reserved.
 *
 * This file is part of the EPOS.
 *
 * Redistribution and use in source and binary forms are freely
 * permitted provided that the above copyright notice and this
 * paragraph and the following disclaimer are duplicated in all
 * such forms.
 *
 * This software is provided "AS IS" and without any express or
 * implied warranties, including, without limitation, the implied
 * warranties of merchantability and fitness for a particular
 * purpose.
 *
 */
#ifndef _GRAPHICS_H
#define _GRAPHICS_H

#include <inttypes.h>

typedef uint32_t COLORREF;
#define getRValue(c) ((uint8_t)(c))
#define getGValue(c) ((uint8_t)((c)>>8))
#define getBValue(c) ((uint8_t)((c)>>16))
#define getAValue(c) ((uint8_t)((c)>>24))
#define RGB(r,g,b)   ((COLORREF)((uint8_t)(r)|\
                                 ((uint8_t)(g)<<8)|\
                                 ((uint8_t)(b)<<16)))
#define RGBA(r,g,b,a) ((COLORREF)( (((uint32_t)(uint8_t)(a))<<24)|\
                                   RGB(r,g,b) ))

#define SCREEN_WIDTH g_graphic_dev.XResolution
#define SCREEN_HIGHT g_graphic_dev.YResolution

#define X_DIVISION  3
#define Y_DIVISION  1

#define BLOCK_WIDTH SCREEN_WIDTH / X_DIVISION
#define BLOCK_HIGHT SCREEN_HIGHT / Y_DIVISION

typedef struct RGB_STRUCT_Integer
{
    int ri;
    int gi;
    int bi;
} rgb_struct;

struct graphic_dev {
    uint16_t XResolution;
    uint16_t YResolution;
    uint16_t BytesPerScanLine;
    uint8_t  BitsPerPixel;
    uint8_t  NumberOfPlanes;
    uint8_t *FrameBuffer;
    uint32_t FrameBufferSize;
    int      Linear;
    int    (*pfnSwitchBank)(int bank);
};

extern struct graphic_dev g_graphic_dev;

/**
 * 列出系统支持的图形模式，包括分辨率和像素位数
 *
 * 注意：该函数只能在文本模式下面运行！
 *       否则，看不到任何输出！
 *
 */
int list_graphic_modes();

/**
 * 进入由mode指定的图形模式
 */
int init_graphic(int mode);

/**
 * 退回到原来的模式
 */
int exit_graphic();

/**
 * 进入图形模式后，用于在屏幕位置(x,y)以颜色cr打点
 */
void setPixel(int x, int y, COLORREF cr);

/**
 * 用颜色cr画一条从(x1,y1)到(x2,y2)的直线
 */
void line(int x1,int y1,int x2,int y2, COLORREF cr);

// 刷新指定局域
void refreshArea(int x1, int y1, int x2, int y2, COLORREF cr);

// 刷新全屏幕
void refreshAll(COLORREF cr);

// rgb转换为色相
int rgb2Hue(rgb_struct rgb);

// 色相转换为rgb
rgb_struct Hue2rgb(int hue);

// 求rgb值中的最大最小值
float maxRGB(float r, float g, float b);
float minRGB(float r, float g, float b);

// 用指定颜色的线画出当前数组
void drawLines(int ar[], int size, int X_Location, int Y_Location, int colorSort);
void drawLinesFromBottom(int ar[], int size, int X_Location, int Y_Location);

#endif /*_GRAPHICS_H*/
