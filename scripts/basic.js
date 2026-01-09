// 1. 变量声明和类型测试
println("1. 变量声明和类型测试:");
let num = 10;
const str = "Hello";
var boolVal = true;

println("数字变量: " + num);
println("字符串变量: " + str);
println("布尔变量: " + boolVal);
println("======================");

// 2. 算术运算测试
println("2. 算术运算测试:");
let a = 10;
let b = 3;
println("加法: " + a + " + " + b + " = " + (a + b));
println("减法: " + a + " - " + b + " = " + (a - b));
println("乘法: " + a + " * " + b + " = " + (a * b));
println("除法: " + a + " / " + b + " = " + (a / b));
println("取余: " + a + " % " + b + " = " + (a % b));
println("======================");

// 3. 字符串操作测试
println("3. 字符串操作测试:");
let s1 = "Hello";
let s2 = "World";
println("字符串拼接: " + s1 + " " + s2);
println("字符串长度: " + s1.length);
println("字符访问: " + s1.at(1));
println("子字符串: " + s1.substring(1, 4));
println("======================");

// 4. 数组测试
println("4. 数组操作测试:");
let arr = [1, 2, 3, 4, 5];
println("数组长度: " + arr.length);
println("数组元素: " + arr);
println("访问元素: arr[2] = " + arr.at(2));

arr.push(6);
println("添加元素后: " + arr);

arr.pop();
println("删除元素后: " + arr);
println("======================");

// 5. 条件判断测试
println("5. 条件判断测试:");
let x = 10;
let y = 20;
if (x < y) {
    println("x < y: true");
} else {
    println("x < y: false");
}

let result = x > 5 ? "大于5" : "小于等于5";
println("三元运算符: " + result);
println("======================");

// 6. 循环测试
println("6. 循环测试:");

// for 循环
println("for 循环:");
for (let i = 0; i < 5; i++) {
    print(i + " ");
}
println("======================");

// while 循环
println("while 循环:");
let count = 0;
while (count < 5) {
    print(count + " ");
    count++;
}
println("======================");

// 7. 函数测试
println("7. 函数测试:");

function add(a, b) {
    return a + b;
}

function greet(name) {
    return "Hello, " + name + "!";
}

println("add(10, 20) = " + add(10, 20));
println("greet('World') = " + greet('World'));
println("======================");

// 8. 阶乘函数（递归）
function factorial(n) {
    if (n === 0 || n === 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

println("8. 递归函数测试:");
println("factorial(5) = " + factorial(5));
println("======================");

println("=== 所有基础测试完成 ===");
