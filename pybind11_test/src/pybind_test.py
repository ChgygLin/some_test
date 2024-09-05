# import cv2
import numpy as np
import my_module

# 创建一个测试图像
img = np.random.rand(10, 10, 3)

# 调用C++函数处理图像
np_matrix, point1, point2 = my_module.process_image(img)

# 打印返回结果
print("3x3 Matrix:\n", np_matrix)
print("Point 1:", point1)
print("Point 2:", point2)



# apt install pybind11-dev
# ln -sf /usr/lib/x86_64-linux-gnu/libstdc++.so.6 /home/xxx/anaconda3/envs/xxx/bin/../lib/libstdc++.so.6
# python pybind_test.py

