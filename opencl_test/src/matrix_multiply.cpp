// OpenCL C++测试代码
#define CL_HPP_ENABLE_EXCEPTIONS
#define CL_HPP_TARGET_OPENCL_VERSION 210

#include <CL/cl2.hpp>
#include <iostream>
#include <vector>




// 定义一个向量加法的内核函数
const char* kernel_source = R"(
__kernel void matrix_multiply(__global const float* a, __global const float* b, __global float* c) {
    // 获取全局线程ID
    int i = get_global_id(0);
    int j = get_global_id(1);

    float sum = 0.0f;
    for (int k=0; k<20; k++)
    {
        sum += a[i*20 + k] * b[k*30 + j];       // 计算矩阵乘法, 这里不能使用二位数组的下标写法
    }

    c[i*30 + j] = sum; 
}
)";

int main() {
    // 定义两个2x2的矩阵
    float* A = new float[30*20];
    float* B = new float[20*30];

    for (int i=0; i<20; i++)
        for(int j=0; j<30; j++)
            A[i*30 + j] = i+j;  

    for (int i=0; i<30; i++)
        for(int j=0; j<20; j++)
            B[i*20 + j] = i-j;
    
    float* C = new float[30*30];
    float *C_cpu = new float[30*30];

    float sum = 0.0f;
    for (int i=0; i<30; i++)
    {
        for (int j=0; j<30; j++)
        {
            for (int k=0; k<20; k++)
            {
                sum += A[i*20 + k] * B[k*30 + j];       // 计算矩阵乘法, 这里不能使用二位数组的下标写法
            }

            C_cpu[i*30 + j] = sum; 
            sum = 0;
        }
    }


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
        cl::Buffer buffer_a(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(float)*30*20, A);
        cl::Buffer buffer_b(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(float)*20*30, B);
        cl::Buffer buffer_c(context, CL_MEM_WRITE_ONLY, sizeof(float)*30*30);

        // 创建程序对象
        cl::Program program(context, kernel_source);

        try
        {
            // 构建程序
            program.build("-cl-std=CL1.2");
            std::cout << "Use Program OpenCL version 1.2" << std::endl;
            // program.build({device});
        }
        catch(cl::Error error)
        {
            // 如果编译失败，则输出错误信息
            std::string log = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device);
            std::cerr << "Kernel compilation failed:\n" << log << std::endl;
        }

        // 如果编译成功，则输出编译日志
        std::string log = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device);
        std::cout << "Kernel compilation succeeded:\n" << log << std::endl;



        // 创建内核对象
        cl::Kernel kernel(program, "matrix_multiply");

        // 设置内核参数
        kernel.setArg(0, buffer_a);
        kernel.setArg(1, buffer_b);
        kernel.setArg(2, buffer_c);

        // 执行内核
        queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(30,30), cl::NDRange(1, 1));

        // 读取结果数据
        queue.enqueueReadBuffer(buffer_c, CL_TRUE, 0, sizeof(float)*30*30, C);

        // 打印结果数据
        bool ok = true;
        for (int i = 0; i < 30; i++) 
        {
            for (int j = 0; j < 30; j++) 
            {
                // std::cout << C[i*30 + j] << " ";
                if ( C[i*30 + j] != C_cpu[i*30 + j] )
                {
                    std::cout << "matrix_multiply result error" << std::endl;
                    std::cout << i << "   " << j << std::endl;
                    
                    ok = false;
                    break;
                }
            }
        }

        if(ok)
            std::cout << "matrix_multiply OK" << std::endl;

    }
    catch (cl::Error& e) {
        // 捕获异常并打印错误信息
        std::cerr << "Error: " << e.what() << ", code: " << e.err() << "\n";
    }

    delete A;
    delete B;
    delete C;
    delete C_cpu;

    

    return 0;
}