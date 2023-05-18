// OpenCL C++测试代码
#define CL_HPP_ENABLE_EXCEPTIONS
#define CL_HPP_TARGET_OPENCL_VERSION 210

#include <CL/cl2.hpp>
#include <iostream>
#include <vector>




// 定义一个向量加法的内核函数
const char* kernel_source = R"(
__kernel void vector_add(__global const int* a, __global const int* b, __global int* c) {
    // 获取全局线程ID
    int gid = get_global_id(0);
    // 计算向量加法
    c[gid] = a[gid] + b[gid];
}
)";

int main() {
    // 定义向量的大小和数据
    const int N = 10;
    std::vector<int> a(N, 1); // 初始化为全1
    std::vector<int> b(N, 2); // 初始化为全2
    std::vector<int> c(N, 0); // 初始化为全0

    try {
        // 获取所有可用的平台
        std::vector<cl::Platform> platforms;
        cl::Platform::get(&platforms);

        // 选择第一个平台
        cl::Platform platform = platforms[0];
        std::cout << "Using platform: " << platform.getInfo<CL_PLATFORM_NAME>() << "\n";

        // 获取平台下的所有设备
        std::vector<cl::Device> devices;
        platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);

        // 选择第一个设备
        cl::Device device = devices[0];
        std::cout << "Using device: " << device.getInfo<CL_DEVICE_NAME>() << "\n";
        std::cout << "Device supports OpenCL version " << device.getInfo<CL_DEVICE_VERSION>() << std::endl;

        // 创建上下文和命令队列
        cl::Context context(device);
        cl::CommandQueue queue(context, device);

        // 创建内存对象
        cl::Buffer buffer_a(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(int) * N, a.data());
        cl::Buffer buffer_b(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(int) * N, b.data());
        cl::Buffer buffer_c(context, CL_MEM_WRITE_ONLY, sizeof(int) * N);

        // 创建程序对象
        cl::Program program(context, kernel_source);

        // 构建程序
        program.build("-cl-std=CL1.2");
        std::cout << "Use Program OpenCL version 1.2" << std::endl;
        // program.build({device});

        // 创建内核对象
        cl::Kernel kernel(program, "vector_add");

        // 设置内核参数
        kernel.setArg(0, buffer_a);
        kernel.setArg(1, buffer_b);
        kernel.setArg(2, buffer_c);

        // 执行内核
        queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(N), cl::NullRange);

        // 读取结果数据
        queue.enqueueReadBuffer(buffer_c, CL_TRUE, 0, sizeof(int) * N, c.data());

        // 打印结果数据
        std::cout << "Result: \n";
        for (int i = 0; i < N; i++) {
            std::cout << a[i] << " + " << b[i] << " = " << c[i] << "\n";
        }
    }
    catch (cl::Error& e) {
        // 捕获异常并打印错误信息
        std::cerr << "Error: " << e.what() << ", code: " << e.err() << "\n";
    }

    return 0;
}