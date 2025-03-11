#include "mylib.h"
#include <math.h>
#include <stdlib.h>

// 实现加法函数
int32_t add(int32_t a, int32_t b) {
    return a + b;
}

// 实现创建点的函数
Point create_point(int32_t x, int32_t y, uint32_t color) {
    Point p;
    p.x = x;
    p.y = y;
    p.color = color;
    return p;
}

// 实现计算两点之间距离的函数
double distance(const Point* p1, const Point* p2) {
    double dx = p1->x - p2->x;
    double dy = p1->y - p2->y;
    return sqrt(dx*dx + dy*dy);
}

// 实现填充点数组的函数
void fill_point_array(PointArray* array, uint32_t num_points) {
    // 确保不超过数组容量
    if (num_points > 10) {
        num_points = 10;
    }
    
    array->num_points = num_points;
    
    // 用一些示例值填充数组
    for (uint32_t i = 0; i < num_points; i++) {
        array->points[i].x = i * 10;
        array->points[i].y = i * 5;
        array->points[i].color = 0xFF000000 | (i * 0x101010); // 产生不同的颜色
    }
}

// 实现宏函数包装
int32_t max_int(int32_t a, int32_t b) {
    return MAX(a, b);
}

void set_bit_func(uint32_t* value, uint32_t bit) {
    SET_BIT(*value, bit);
}

void clear_bit_func(uint32_t* value, uint32_t bit) {
    CLEAR_BIT(*value, bit);
}

int test_bit_func(uint32_t value, uint32_t bit) {
    return TEST_BIT(value, bit);
} 