#include <opencv2/opencv.hpp>
#include <chrono>
#include <iostream>
#include <fstream>
#include <string>

#define CL_HPP_ENABLE_EXCEPTIONS
#define CL_HPP_TARGET_OPENCL_VERSION 210
#include <CL/cl2.hpp>

using namespace cv;
using namespace std;

class TimeMeasurement
{
public:
  typedef std::chrono::time_point<std::chrono::high_resolution_clock> TimePoint;

  static bool enabled;

  static bool debug;

  static void start(const std::string& name);

  static float stop(const std::string& name);

private:
  static std::map<std::string, TimePoint> startPoints;
};

bool TimeMeasurement::enabled = false;
bool TimeMeasurement::debug = true;

map<string, TimeMeasurement::TimePoint> TimeMeasurement::startPoints = map<string, TimeMeasurement::TimePoint>();

void TimeMeasurement::start(const std::string& name)
{
  startPoints[name] = chrono::high_resolution_clock::now();
}


float TimeMeasurement::stop(const std::string& name)
{
  auto end = chrono::high_resolution_clock::now();
  chrono::duration<double> elapsed_s = end - startPoints[name];
  if (debug)
  {
    cout << name << " " << 1000 * elapsed_s.count() << " ms" << endl;
  }
  return elapsed_s.count();
}


const char* kernel_source = R"(
__kernel void remap(
                    __global const unsigned char* input_image,
                    __global unsigned char* output_image,
                    const int width,
                    const int height,
                    __global const float* x_map,
                    __global const float* y_map)
{
    // 目标图像的下标
    const int x = get_global_id(0);  
    const int y = get_global_id(1);  

    // 对于每个像素，计算它在新图像中的位置，并使用双线性插值的方式进行插值
    int new_x = x_map[y*width + x];
    int new_y = y_map[y*width + x];

    if (new_x >= 0 && new_x < width && new_y >= 0 && new_y < height) 
    {
        for (int c = 0; c < 3; c++) 
        {
            float v1 = input_image[(new_y * width + new_x) * 3 + c];
            float v2 = input_image[(new_y * width + new_x + 1) * 3 + c];
            float v3 = input_image[((new_y + 1) * width + new_x) * 3 + c];
            float v4 = input_image[((new_y + 1) * width + new_x + 1) * 3 + c];

            float dx = x_map[y*width + x] - new_x;
            float dy = y_map[y*width + x] - new_y;

            float w1 = (1 - dx) * (1 - dy);
            float w2 = dx * (1 - dy);
            float w3 = (1 - dx) * dy;
            float w4 = dx * dy;

            output_image[(y * width + x) * 3 + c] = w1 * v1 + w2 * v2 + w3 * v3 + w4 * v4;
        }
    }
}
)";


Mat get_H()
{
    std::vector<Point2f> image_points;

    image_points.push_back(Point2f(100, 100));    // 0
    image_points.push_back(Point2f(1800, 100));   // 4
    image_points.push_back(Point2f(100, 900));   // 25
    image_points.push_back(Point2f(1800, 900));  // 29


    //
    std::vector<Point2f> court_points;

    court_points.push_back(Point2f(100, 100));    // 0
    court_points.push_back(Point2f(1800, 100));   // 4
    court_points.push_back(Point2f(300, 900));   // 25
    court_points.push_back(Point2f(1600, 900));  // 29

    Mat H = findHomography(image_points, court_points);

    return H;
}

void get_xy_map(Mat H, Mat &x_map, Mat &y_map, int width, int height)
{
    initUndistortRectifyMap(Mat::eye(3, 3, CV_32F), Mat(), Mat(), Mat::eye(3, 3, CV_32F), Size(width, height), CV_32FC1, x_map, y_map);

    Mat H_inv = H.inv();
    x_map = (H_inv.at<double>(0, 0) * x_map + H_inv.at<double>(0, 1) * y_map + H_inv.at<double>(0, 2)) / (H_inv.at<double>(2, 0) * x_map + H_inv.at<double>(2, 1) * y_map + H_inv.at<double>(2, 2));
    y_map = (H_inv.at<double>(1, 0) * x_map + H_inv.at<double>(1, 1) * y_map + H_inv.at<double>(1, 2)) / (H_inv.at<double>(2, 0) * x_map + H_inv.at<double>(2, 1) * y_map + H_inv.at<double>(2, 2));
}

void cpu_remap(unsigned char* input_image, unsigned char* output_image, float* x_map, float* y_map, int width, int height)
{
    // 对于每个像素，计算它在新图像中的位置，并使用双线性插值的方式进行插值
    for (int y = 0; y < height; y++) 
    {
        for (int x = 0; x < width; x++) 
        {
            int new_x = x_map[y*width + x];
            int new_y = y_map[y*width + x];

            if (new_x >= 0 && new_x < width && new_y >= 0 && new_y < height) 
            {
                for (int c = 0; c < 3; c++) 
                {
                    float v1 = input_image[(new_y * width + new_x) * 3 + c];
                    float v2 = input_image[(new_y * width + new_x + 1) * 3 + c];
                    float v3 = input_image[((new_y + 1) * width + new_x) * 3 + c];
                    float v4 = input_image[((new_y + 1) * width + new_x + 1) * 3 + c];

                    float dx = x_map[y*width + x] - new_x;
                    float dy = y_map[y*width + x] - new_y;

                    float w1 = (1 - dx) * (1 - dy);
                    float w2 = dx * (1 - dy);
                    float w3 = (1 - dx) * dy;
                    float w4 = dx * dy;

                    output_image[(y * width + x) * 3 + c] = w1 * v1 + w2 * v2 + w3 * v3 + w4 * v4;
                }
            }
        }
    }
}

void show_img(string img_name, Mat img, int ix, int iy)
{
    int x = 100 + ix*450;
    int y = 100 + iy*400;
    cv::namedWindow(img_name, cv::WINDOW_NORMAL);
    cv::moveWindow(img_name, x, y);
    cv::imshow(img_name, img);
}


void remap_test(const Mat& input_image, Mat &x_map, Mat &y_map)
{
    int w = input_image.cols;
    int h = input_image.rows;
    cl::size_type input_size = input_image.total() * input_image.elemSize();


    // std::vector<unsigned char> image_data(input_image.data, input_image.data + input_image.total() * input_image.channels());

    // test cpu_remap
    Mat dst(input_image.size(), input_image.type());
    TimeMeasurement::start("cpu_remap");
    cpu_remap(input_image.data, dst.data, x_map.ptr<float>(), y_map.ptr<float>(), w, h);
    TimeMeasurement::stop("cpu_remap");
    show_img("cpu_remap", dst, 0, 1);

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
        cl::Kernel kernel(program, "remap");

        
        TimeMeasurement::start("opencl_remap");
        // 创建内存对象, 7ms
        cl::Buffer inputBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, input_size, input_image.data);
        cl::Buffer outputBuffer(context, CL_MEM_WRITE_ONLY, input_size);
        cl::Buffer x_map_buffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, x_map.total() * x_map.elemSize(), x_map.ptr<float>());
        cl::Buffer y_map_buffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, y_map.total() * y_map.elemSize(), y_map.ptr<float>());

        // 设置内核参数
        kernel.setArg(0, inputBuffer);
        kernel.setArg(1, outputBuffer);
        kernel.setArg(2, w);
        kernel.setArg(3, h);
        kernel.setArg(4, x_map_buffer);
        kernel.setArg(5, y_map_buffer);

        // 执行内核
        queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(w, h), cl::NullRange);

        // 1ms
        cl_int err = queue.enqueueReadBuffer(outputBuffer, CL_TRUE, 0, input_size, dst.data);
        TimeMeasurement::stop("opencl_remap");
        

        show_img("opencl_remap", dst, 1, 1);
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
    show_img("img", img, 0, 0);


    int width, height;
    width = img.cols;
    height = img.rows;

    
    Mat H = get_H();

    Mat dst(Size(width, height), img.type());
    TimeMeasurement::start("opencv_warpPerspective");
    warpPerspective(img, dst, H, img.size());
    TimeMeasurement::stop("opencv_warpPerspective");
    show_img("opencv_warpPerspective", dst, 1, 0);


    // 计算x_map和y_map
    Mat x_map, y_map;
    get_xy_map(H, x_map, y_map, width, height);


    // 使用remap函数重新warp图片
    TimeMeasurement::start("opencv_remap");
    remap(img, dst, x_map, y_map, INTER_LINEAR);
    TimeMeasurement::stop("opencv_remap");
    show_img("opencv_remap", dst, 2, 0);


    remap_test(img, x_map, y_map);

    return 0;
}
