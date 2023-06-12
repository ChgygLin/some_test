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