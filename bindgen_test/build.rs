use std::env;
use std::path::{PathBuf, Path};
use std::process::Command;

fn main() {
    // 告诉 cargo 如果头文件或源文件改变就重新运行脚本
    println!("cargo:rerun-if-changed=./src/libc/mylib.h");
    println!("cargo:rerun-if-changed=./src/libc/mylib.c");
    
    // 获取目标架构信息
    let target = env::var("TARGET").unwrap_or_else(|_| String::from("x86_64-unknown-linux-gnu"));
    println!("cargo:warning=当前编译目标: {}", target);
    
    // 获取cargo target目录
    let target_dir = get_target_dir();
    println!("cargo:warning=cargo target目录: {}", target_dir.display());
    
    // 使用格式化的目标架构路径
    let arch_target_dir = if target.contains("aarch64") {
        target_dir.join("aarch64-unknown-linux-gnu")
    } else {
        target_dir.clone()
    };
    
    // mylib路径
    let mylib_dir = arch_target_dir.join("mylib");
    println!("cargo:warning=mylib将放置在: {}", mylib_dir.display());
    
    // 为特定目标架构编译C库
    if target.contains("aarch64") {
        // 为aarch64交叉编译
        compile_for_aarch64(&mylib_dir);
    } else {
        // 为主机系统编译
        compile_for_host(&mylib_dir);
    }
    
    // 告诉链接器寻找我们的库
    println!("cargo:rustc-link-search={}", mylib_dir.display());  // 在CARGO_TARGET_DIR/mylib目录寻找库
    println!("cargo:rustc-link-lib=mylib");  // 链接 libmylib.so

    // 使用 bindgen 生成 Rust 绑定
    let bindings = bindgen::Builder::default()
        // 输入头文件
        .header("./src/libc/mylib.h")
        // 生成完整的文档
        .generate_comments(true)
        // 告诉 bindgen 为结构体生成 layout 信息
        .layout_tests(true)
        // 完成绑定生成
        .generate()
        .expect("无法生成绑定");

    // 写入输出文件
    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    
    // 读取生成的绑定，并添加unsafe关键字到extern块
    let binding_string = bindings.to_string();
    let binding_string_with_unsafe = binding_string.replace("extern \"C\"", "unsafe extern \"C\"");
    
    // 写入修改后的绑定
    std::fs::write(out_path.join("bindings.rs"), binding_string_with_unsafe)
        .expect("无法写入绑定");
}

// 为主机编译库
fn compile_for_host(mylib_dir: &Path) {
    // 创建目标目录
    std::fs::create_dir_all(mylib_dir).unwrap();
    
    // 获取输出路径
    let output_path = mylib_dir.join("libmylib.so").to_string_lossy().into_owned();
    println!("cargo:warning=为主机编译库并输出到: {}", output_path);
    
    // 编译库
    let status = Command::new("gcc")
        .args([
            "-shared", "-fPIC",
            "-o", &output_path,
            "src/libc/mylib.c",
            "-lm"
        ])
        .status()
        .expect("无法编译库");
    
    if !status.success() {
        panic!("编译库失败");
    }
}

// 为aarch64交叉编译库
fn compile_for_aarch64(mylib_dir: &Path) {
    // 创建目标目录
    std::fs::create_dir_all(mylib_dir).unwrap();
    
    // 获取输出路径
    let output_path = mylib_dir.join("libmylib.so").to_string_lossy().into_owned();
    println!("cargo:warning=为aarch64编译库并输出到: {}", output_path);
    
    // 使用交叉编译器编译库
    let status = Command::new("aarch64-linux-gnu-gcc")
        .args([
            "-shared", "-fPIC",
            "-o", &output_path,
            "src/libc/mylib.c",
            "-lm"
        ])
        .status()
        .expect("无法为aarch64编译库，请确保已安装aarch64-linux-gnu-gcc交叉编译器");
    
    if !status.success() {
        panic!("为aarch64编译库失败");
    }
}

fn get_target_dir() -> PathBuf {
    // 直接获取CARGO_TARGET_DIR环境变量
    match env::var("CARGO_TARGET_DIR") {
        Ok(dir) => {
            println!("cargo:warning=使用CARGO_TARGET_DIR环境变量: {}", dir);
            PathBuf::from(dir)
        }
        Err(e) => {
            println!("cargo:warning=无法获取CARGO_TARGET_DIR: {}，使用默认target目录", e);
            PathBuf::from(env::current_dir().unwrap_or(PathBuf::from("."))).join("target")
        }
    }
} 