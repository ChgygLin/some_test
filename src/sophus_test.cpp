
#include <Eigen/Core>

#include <sophus/se3.hpp>

using namespace std;



int main(int argc, char** argv)
{
    // 沿Z轴转90度的旋转矩阵
    Eigen::Matrix3d R = Eigen::AngleAxisd(M_PI/2, Eigen::Vector3d(0, 0, 1)).toRotationMatrix();

    // ----------------------------- SO(3)操作 -----------------------------------
    // 从旋转矩阵构造SO(3)
    Sophus::SO3d SO3_R(R);

    // 从向量构造SO(3),不是旋转向量, 似乎不支持该方法了
    // Sophus::SO3d SO3_v(0.0, 0.0, M_PI/2);

    // 从四元数构造SO(3)
    Eigen::Quaterniond q(R);
    Sophus::SO3d SO3_q(q);


    // 输出SO(3)
    cout << "SO(3) from matrix: \n" << SO3_R.matrix() <<endl<<endl;
    cout << "SO(3) from quaternion: \n" << SO3_q.matrix() <<endl<<endl;

    // 使用对数映射获得他的李代数
    Eigen::Vector3d so3 = SO3_R.log();
    cout << "so3 = \n" << so3.transpose() <<endl<<endl;

    // hat为向量到反对称矩阵
    cout<<"so3 hat= \n" << Sophus::SO3d::hat(so3)  <<endl<<endl;

    // vee为反对称矩阵到向量
    cout<<"so3 hat vee= \n" << Sophus::SO3d::vee(Sophus::SO3d::hat(so3)).transpose() <<endl<<endl;


    // 增量扰动模型的更新
    Eigen::Vector3d update_so3(1e-4, 0, 0);
    Sophus::SO3d SO3_updated = Sophus::SO3d::exp(update_so3)*SO3_R;  //  左乘更新

    cout<<"SO3 updated = \n" << SO3_updated.matrix() <<endl<<endl;


    // ----------------------------- SE(3)操作 -----------------------------------
    Eigen::Vector3d t(1, 0, 0);     // 沿X轴平移1
    Sophus::SE3d SE3_Rt(R, t);      // 从R,t构造SE(3)
    Sophus::SE3d SE3_qt(q, t);      // 从q,t构造SE(3)

    cout<< "SE3 from R,t= \n" << SE3_Rt.matrix() << endl<<endl;
    cout<< "SE3 from q,t= \n" << SE3_qt.matrix() << endl<<endl;


    Eigen::Matrix<double, 6, 1> se3 = SE3_Rt.log();
    cout<<"se3 = \n" << se3.transpose() <<endl<<endl;

    // se(3)中平移在前，旋转在后
    cout<<"se3 hat = \n" << Sophus::SE3d::hat(se3) <<endl<<endl;
    cout<<"se3 hat vee = \n" << Sophus::SE3d::vee(Sophus::SE3d::hat(se3)).transpose() <<endl<<endl;


    // 更新
    Eigen::Matrix<double, 6, 1> update_se3;
    update_se3.setZero();
    update_se3(0, 0) = 1e-4d;

    Sophus::SE3d SE3_updated = Sophus::SE3d::exp(update_se3)*SE3_Rt;
    cout<<"SE3 updated = \n" << SE3_updated.matrix() <<endl<<endl;

    return 0;
}