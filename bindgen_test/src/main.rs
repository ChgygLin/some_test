#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

// 包含从build.rs生成的绑定
include!(concat!(env!("OUT_DIR"), "/bindings.rs")); 

fn main() {
    println!("Hello, world!");
    println!("测试 mylib.h 的 Rust 绑定!");
    
    unsafe {
        // 使用add函数
        let sum = add(5, 3);
        println!("5 + 3 = {}", sum);
        
        // 创建点
        let p1 = create_point(10, 20, 0xFF0000); // 红色点
        let p2 = create_point(30, 40, 0x00FF00); // 绿色点
        
        println!("点1: ({}, {}) 颜色: {:X}", p1.x, p1.y, p1.color);
        println!("点2: ({}, {}) 颜色: {:X}", p2.x, p2.y, p2.color);
        
        // 计算距离
        let dist = distance(&p1, &p2);
        println!("两点之间的距离: {:.2}", dist);
        
        // 使用点数组
        let mut point_array = PointArray {
            num_points: 0,
            points: [Point { x: 0, y: 0, color: 0 }; 10]
        };
        
        // 填充点数组
        fill_point_array(&mut point_array, 5);
        
        println!("点数组中的点数: {}", point_array.num_points);
        for i in 0..point_array.num_points as usize {
            println!("点[{}]: ({}, {}) 颜色: {:X}",
                i, point_array.points[i].x, point_array.points[i].y, point_array.points[i].color);
        }
        
        // 使用包装的宏函数
        let max_val = max_int(42, 99);
        println!("max(42, 99) = {}", max_val);
        
        // 位操作函数
        let mut value: u32 = 0;
        set_bit_func(&mut value, 3);
        println!("设置第3位后的值: {:b}", value);
        
        let is_bit_set = test_bit_func(value, 3);
        println!("第3位是否设置: {}", is_bit_set);
        
        clear_bit_func(&mut value, 3);
        println!("清除第3位后的值: {:b}", value);
    }
}
