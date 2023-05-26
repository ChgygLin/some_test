#include <opencv2/opencv.hpp>
#include <chrono>
#include <iostream>
#include <fstream>
#include <string>

#define CL_TARGET_OPENCL_VERSION 120
#define __CL_ENABLE_EXCEPTIONS
#include <CL/cl.hpp>

using namespace cv;
using namespace std;


const char* kernel_source = R"(
__kernel void rotate(
                    __global const unsigned char* input_image,
                    __global unsigned char* output_image,
                    const int width,
                    const int height,
                    const float sin_theta,
                    const float cos_theta)
{
    // 目标图像的下标
    const int x = get_global_id(0);  
    const int y = get_global_id(1);  

    const int center_x = width / 2;
    const int center_y = height / 2;

    // 计算对应于原始图像的坐标
    const float new_x_float = (x - center_x) * cos_theta - (y - center_y) * sin_theta + center_x;
    const float new_y_float = (x - center_x) * sin_theta + (y - center_y) * cos_theta + center_y;


    // 不插值
    if (false)
    {
        const int new_x = (int)floor(new_x_float);
        const int new_y = (int)floor(new_y_float);
        if (new_x >= 0 && new_x < width && new_y >= 0 && new_y < height) 
        {
            const int output_index = (y * width + x)*3;
            const int input_index = (new_y * width + new_x)*3;

            output_image[output_index] = input_image[input_index];
            output_image[output_index+1] = input_image[input_index+1];
            output_image[output_index+2] = input_image[input_index+2];
        }
    }
    else
    {
        // 双线性插值
        const int x1 = (int)floor(new_x_float);
        const int x2 = x1 + 1;
        const int y1 = (int)floor(new_y_float);
        const int y2 = y1 + 1;

        const float dx = new_x_float - x1;
        const float dy = new_y_float - y1;

        if (x1 >= 0 && x2 < width && y1 >= 0 && y2 < height) 
        {
            const int input_index_11 = (y1 * width + x1)*3;
            const int input_index_12 = (y2 * width + x1)*3;
            const int input_index_21 = (y1 * width + x2)*3;
            const int input_index_22 = (y2 * width + x2)*3;

            const int output_index = (y * width + x)*3;

            for (int i=0; i<3; i++)
            {
                const float pixel_11 = (float)input_image[input_index_11+i];
                const float pixel_12 = (float)input_image[input_index_12+i];
                const float pixel_21 = (float)input_image[input_index_21+i];
                const float pixel_22 = (float)input_image[input_index_22+i];


                const float interpolated_pixel =
                    pixel_11 * (1 - dx) * (1 - dy) +
                    pixel_12 * dx * (1 - dy) +
                    pixel_21 * (1 - dx) * dy +
                    pixel_22 * dx * dy;

                output_image[output_index+i] = interpolated_pixel;
            }
        }
    }

}
)";

void cpu_rotate(unsigned char* input_image, unsigned char* output_image, int width, int height, float sin_theta, float cos_theta)
{

    int center_x = width/2;
    int center_y = height/2;

    // 目标图像的下标
    for(int x = 0; x < width; x++)
    {
        for(int y=0; y< height; y++)
        {
            // 计算对应于原始图像的坐标
            const float new_x_float = (x - center_x) * cos_theta - (y - center_y) * sin_theta + center_x;
            const float new_y_float = (x - center_x) * sin_theta + (y - center_y) * cos_theta + center_y;

            if (false)
            {
                const int new_x = (int)floor(new_x_float);
                const int new_y = (int)floor(new_y_float);
                if (new_x >= 0 && new_x < width && new_y >= 0 && new_y < height) 
                {
                    const int output_index = (y * width + x)*3;
                    const int input_index = (new_y * width + new_x)*3;

                    output_image[output_index] = input_image[input_index];
                    output_image[output_index+1] = input_image[input_index+1];
                    output_image[output_index+2] = input_image[input_index+2];
                }
            }
            else
            {
                // 双线性插值
                const int x1 = (int)floor(new_x_float);
                const int x2 = x1 + 1;
                const int y1 = (int)floor(new_y_float);
                const int y2 = y1 + 1;

                const float dx = new_x_float - x1;
                const float dy = new_y_float - y1;

                if (x1 >= 0 && x2 < width && y1 >= 0 && y2 < height) 
                {
                    const int input_index_11 = (y1 * width + x1)*3;
                    const int input_index_12 = (y2 * width + x1)*3;
                    const int input_index_21 = (y1 * width + x2)*3;
                    const int input_index_22 = (y2 * width + x2)*3;

                    const int output_index = (y * width + x)*3;

                    for (int i=0; i<3; i++)
                    {
                        const float pixel_11 = (float)input_image[input_index_11+i];
                        const float pixel_12 = (float)input_image[input_index_12+i];
                        const float pixel_21 = (float)input_image[input_index_21+i];
                        const float pixel_22 = (float)input_image[input_index_22+i];


                        const float interpolated_pixel =
                            pixel_11 * (1 - dx) * (1 - dy) +
                            pixel_12 * dx * (1 - dy) +
                            pixel_21 * (1 - dx) * dy +
                            pixel_22 * dx * dy;

                        output_image[output_index+i] = interpolated_pixel;
                    }
                }
            }
        }
    }
}

void rotate_test(const Mat& input_image)
{
    int w = input_image.cols;
    int h = input_image.rows;
    const float theta = 45.f;
    float pi = 3.1415926535;
    float sin_theta = sin(pi / 180.0 * theta);
    float cos_theta = cos(pi / 180.0 * theta);

    std::vector<unsigned char> image_data(input_image.data, input_image.data + input_image.total() * input_image.channels());

    // test cpu_rotate
    std::vector<unsigned char> cpu_output_data(image_data.size());
    cpu_rotate(image_data.data(), cpu_output_data.data(), w, h, sin_theta, cos_theta);

    cv::Mat cpu_output_image(input_image.rows, input_image.cols, CV_8UC3);
    std::memcpy(cpu_output_image.data, cpu_output_data.data(), image_data.size());

    cv::namedWindow("cpu_rotate", cv::WINDOW_NORMAL);
    cv::moveWindow("cpu_rotate", 600, 100);
    cv::imshow("cpu_rotate", cpu_output_image);
    cv::waitKey(1);


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
        cl::CommandQueue queue(context, device, CL_QUEUE_PROFILING_ENABLE);

        // 创建内存对象
        cl::Buffer inputBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, image_data.size(), image_data.data());
        cl::Buffer outputBuffer(context, CL_MEM_WRITE_ONLY, image_data.size());

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
        cl::Kernel kernel(program, "rotate");

        // 设置内核参数
        kernel.setArg(0, inputBuffer);
        kernel.setArg(1, outputBuffer);
        kernel.setArg(2, w);
        kernel.setArg(3, h);
        kernel.setArg(4, sin_theta);
        kernel.setArg(5, cos_theta);


        // 执行内核
        queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(w, h), cl::NullRange);
        // queue.finish();

        std::vector<unsigned char> output_data(image_data.size());
        cl_int err = queue.enqueueReadBuffer(outputBuffer, CL_TRUE, 0, image_data.size(), output_data.data());

 
        cv::Mat output_image(input_image.rows, input_image.cols, CV_8UC3);
        std::memcpy(output_image.data, output_data.data(), image_data.size());

        cv::namedWindow("opencl_rotate", cv::WINDOW_NORMAL);
        cv::moveWindow("opencl_rotate", 1100, 100);
        cv::imshow("opencl_rotate", output_image);
        cv::waitKey(0);

    }
    catch (cl::Error& e) {
        // 捕获异常并打印错误信息
        std::cerr << "Error: " << e.what() << ", code: " << e.err() << "\n";
    }

}


int main(void)
{

    Mat img = imread("./3.jpg");

    cv::namedWindow("img", cv::WINDOW_NORMAL);
    cv::moveWindow("img", 100, 100);
    cv::imshow("img", img);
    cv::waitKey(1);


    rotate_test(img);

    return 0;
}
