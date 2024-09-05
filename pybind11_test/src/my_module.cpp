// my_module.cpp
#include <opencv2/opencv.hpp>
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>

namespace py = pybind11;


std::tuple<py::array_t<double>, std::tuple<int, int>, std::tuple<int, int>> process_image(py::array_t<uint8_t> input_image) 
{
    // 获取 Numpy 数组的缓冲区信息
    py::buffer_info buf = input_image.request();

    int rows = buf.shape[0];
    int cols = buf.shape[1];
    int channels = buf.ndim == 3 ? buf.shape[2] : 1;

    // 检查 Numpy 数组的结构是否匹配 OpenCV 格式
    if (buf.ndim != 2 && buf.ndim != 3) {
        throw std::runtime_error("Incompatible numpy array dimensions.");
    }

    // 将 Numpy 数组转换为 OpenCV Mat
    cv::Mat mat(rows, cols, CV_8UC(channels), buf.ptr);

    // 执行处理并返回结果
    cv::Mat result = cv::Mat::eye(3, 3, CV_64F);  // 创建一个3x3的单位矩阵

    cv::Point pt1(100, 200);  // 示例点1
    cv::Point pt2(300, 400);  // 示例点2

    // 转换 Mat 为 Numpy 数组
    py::array_t<double> np_matrix = py::array_t<double>({3, 3}, result.ptr<double>());

    // 返回 Numpy 数组和两个点（元组形式）
    return std::make_tuple(np_matrix, std::make_tuple(pt1.x, pt1.y), std::make_tuple(pt2.x, pt2.y));
}


PYBIND11_MODULE(my_module, m) 
{
    m.def("process_image", &process_image, "A function to process an image");
}