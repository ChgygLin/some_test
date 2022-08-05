#include <Eigen/Dense>
#include <iostream>

using namespace std;


void svd_decompose(void);

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