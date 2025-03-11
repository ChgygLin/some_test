#ifndef MYLIB_H
#define MYLIB_H

#ifdef __cplusplus
extern "C" {
#endif

// 定义一些基本类型的别名
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef int int32_t;

// 定义一个简单的结构体
typedef struct {
    int32_t x;
    int32_t y;
    uint32_t color;
} Point;

// 定义一个包含数组的结构体
typedef struct {
    uint32_t num_points;
    Point points[10];  // 固定大小的数组
} PointArray;

// 定义一些函数
// 简单的加法函数
int32_t add(int32_t a, int32_t b);

// 创建点的函数
Point create_point(int32_t x, int32_t y, uint32_t color);

// 计算两点之间距离的函数
double distance(const Point* p1, const Point* p2);

// 填充点数组的函数
void fill_point_array(PointArray* array, uint32_t num_points);

// 定义一个宏函数(bindgen通常不会自动处理这些)
#define MAX(a, b) ((a) > (b) ? (a) : (b))

// 使用位操作的宏, 在bindgen中不会自动处理这些宏, 需要手动在rust代码中定义
#define SET_BIT(value, bit) ((value) |= (1 << (bit)))
#define CLEAR_BIT(value, bit) ((value) &= ~(1 << (bit)))
#define TEST_BIT(value, bit) (((value) & (1 << (bit))) != 0)

// 包装的宏函数 (用于bindgen)
int32_t max_int(int32_t a, int32_t b);
void set_bit_func(uint32_t* value, uint32_t bit);
void clear_bit_func(uint32_t* value, uint32_t bit);
int test_bit_func(uint32_t value, uint32_t bit);

#ifdef __cplusplus
}
#endif

#endif // MYLIB_H 