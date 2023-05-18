#include <Eigen/Dense>
#include <iostream>

using namespace std;


void svd_decompose(void);
void RVQ_test(void);

int main(int argc, char **argv)
{
    int rows,cols;

    rows = 2;
    cols = 3;

    Eigen::MatrixXf tmp_mat;

    // tmp_mat = Eigen::Matrix<float,rows,cols>::Zero();  不能直接使用动态变量初始化
    tmp_mat = Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic>();
    tmp_mat.resize(rows, cols);     // 使用resize的方式确定动态矩阵的维度


    // 赋值
    tmp_mat << 1, 2, 3,
                4, 5, 6;
    
    // 按行求均值
    Eigen::VectorXf vec_mean = tmp_mat.rowwise().mean();
    // [2, 5]
    // assert(vec_mean.size() == 2);
    // assert(vec_mean(0) == 2 && vec_mean(1) == 5);

    // 按列广播运算
    tmp_mat.colwise() -= vec_mean;
    // [-1, 0, 1,
    //    -1, 0, 1] 
    //assert(tmp_mat(0,0) == -1 && tmp_mat(0,1) == 0 && tmp_mat(0,2) == 1 && \
            tmp_mat(1,0) == -1 && tmp_mat(1,1) == 0 && tmp_mat(1,2) == 1 );
    

    // 按列求norm
    Eigen::VectorXf vec_norm1 = tmp_mat.colwise().squaredNorm();    // [2, 2, 2]
    Eigen::VectorXf vec_norm2 = tmp_mat.colwise().norm();           // [1.414, 0, 1.414]

    // Array更方便进行逐元素操作
    // .array()转为Array
    // .matrix()转回Matrix

    // 使用Array进行逐元素相除
    Eigen::VectorXf vec_div = (vec_norm2.array()  / vec_norm1.array()).matrix();
    // cout<<"Div: "<<endl<<vec_div<<endl;
    // [ 0.707107, -nan, 0.707107 ]

    svd_decompose();

    RVQ_test();

    return 0;
}

void svd_decompose(void)
{
    Eigen::MatrixXf H = Eigen::MatrixXf::Zero(3, 3);
	H << 0,1,1,1,1,0,1,0,1;
	cout << "Here is the matrix H:" << endl << H << endl << endl;

	Eigen::JacobiSVD<Eigen::MatrixXf> svd(H, Eigen::ComputeFullU | Eigen::ComputeFullV);

    // m = U*S*V^t 
	cout << "singular values :" << endl << svd.singularValues() << endl << endl;
	cout << "left singular vectors U matrix:" << endl << svd.matrixU() << endl << endl;
	cout << "right singular vectors V matrix:" << endl << svd.matrixV() << endl << endl;

    Eigen::Matrix3f R = svd.matrixV() * svd.matrixU().transpose();
    cout<<"R = V*U^t : " << endl << R << endl;
}

/*
// Eigen与Point类的互相转换
void Point3f_and_Vector3f(void)
{
//Point to eigen
//若pts为定义好的 vector<Point3d> pts_3d l类型并且已经赋值	
	vector<Eigen::Vector3d> pts3_eigen;

	for (int i = 0; i < pts_3d.size(); i++)
	{
		pts3_eigen.push_back(
		Eigen::Vector3d(pts_3d[i].x, pts_3d[i].y, pts_3d[i].z)
		);
	}

    //eigen to Point
    vector<cv::Point3d> point3;
	for (int i = 0; i < pts3_eigen.size(); i++)
	{
		point3.push_back(
		cv::Point3d(pts3_eigen[i](0, 0), pts3_eigen[i](1, 0), pts3_eigen[i](2, 0)));
	}
    
}
*/


// 旋转矩阵R、旋转向量V、四元数Q
void RVQ_test(void)
{
    // 旋转向量（轴角）：沿Z轴旋转45度
    Eigen::AngleAxisd rotation_vector ( M_PI/4, Eigen::Vector3d(0, 0, 1));
    cout << "rotation_vector axis = \n" << rotation_vector.axis() << "\n rotation_vector angle = " << rotation_vector.angle() /M_PI *180 << endl;

    // 旋转矩阵：沿Z轴旋转45度
    Eigen::Matrix3d rotation_matrix = Eigen::Matrix3d::Identity();
    rotation_matrix << 0.707,   -0.707,     0,
                        0.707,  0.707,      0,
                        0,      0,          1;
    cout << "rotation matrix = \n" << rotation_matrix << endl;

    // 四元数：沿Z轴旋转45度
    Eigen::Quaterniond quat = Eigen::Quaterniond(0, 0, 0.383, 0.924);
    cout << "四元数输出方法1: quaternion = \n" << quat.coeffs() << endl;    // coeffs的顺序是(x,y,z,w)，w为实部，前三者为虚部
    cout << "四元数输出方法2: \n x = " << quat.x() << "\n y = " << quat.y() << "\n z = " << quat.z() << "\n w = " << quat.w() << endl;

    // 欧拉角： 沿Z轴旋转45度
    Eigen::Vector3d euler_angles = Eigen::Vector3d(M_PI/4, 0, 0);   // ZYX顺序，即roll、pitch、yaw顺序
    cout << "Euler: yaw pitch roll = " << euler_angles.transpose() << endl; // 按照xyz顺序输出，故需要转置

    // -----------------------------------------------------------------------------------------------------------

    // 相互转化关系


    // 旋转向量转化为其他形式
    cout << "旋转向量转化为旋转矩阵方法1: rotation matrix = \n" << rotation_vector.toRotationMatrix() << endl;
    cout << "旋转向量转化为旋转矩阵方法2: rotation matrix = \n" << rotation_vector.matrix() << endl;

    quat = rotation_vector;
    cout << "旋转向量转化为四元数: quaternion = \n" << quat.coeffs() << endl; // (x,y,z,w)

    // 旋转矩阵转化为其他形式

    cout << "旋转矩阵转化为旋转向量: rotation_vector axis = \n" << rotation_vector.fromRotationMatrix(rotation_matrix).axis() << "\n rotation_vector angle = " << rotation_vector.fromRotationMatrix(rotation_matrix).angle() << endl;
    // 注意: fromRotationMatrix 只适用于旋转向量，不适用于四元数

    rotation_vector = rotation_matrix;
    cout << "旋转矩阵直接给旋转向量赋值初始化: rotation_vector axis = \n" << rotation_vector.axis() << "\n rotation_vector angle = " << rotation_vector.angle() << endl;

    euler_angles = rotation_matrix.eulerAngles(2, 1, 0);    // ZYX顺序
    cout << "旋转矩阵转化为欧拉角: yaw pitch roll = \n" << euler_angles.transpose() << endl;    // 按照xyz顺序输出，故需要转置

    quat = rotation_matrix;
    cout << "旋转矩阵转化为四元数: " << quat.coeffs() << endl;


    // 四元数转化为其他形式
    rotation_vector = quat;
    cout << "四元数转化为旋转向量: rotation_vector axis = " << rotation_vector.axis() << "\nrotation_vector angle = " << rotation_vector.angle() << endl;

    rotation_matrix = quat.toRotationMatrix();
    cout << "四元数转化为旋转矩阵方法1: rotation matrix = \n" << rotation_matrix << endl;

    rotation_matrix = quat.matrix();
    cout << "四元数转化为旋转矩阵方法2: rotation matrix = \n" << rotation_matrix << endl;

}