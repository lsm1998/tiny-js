// 1. 变量声明和类型测试
println("1. 变量声明和类型测试:");
var num = 10;
const str = "Hello";
var boolVal = true;

println("数字变量: " + num);
println("字符串变量: " + str);
println("布尔变量: " + boolVal);
println("======================");

// 2. 算术运算测试
println("2. 算术运算测试:");
var a = 10;
var b = 3;
println("加法: " + a + " + " + b + " = " + (a + b));
println("减法: " + a + " - " + b + " = " + (a - b));
println("乘法: " + a + " * " + b + " = " + (a * b));
println("除法: " + a + " / " + b + " = " + (a / b));
println("取余: " + a + " % " + b + " = " + (a % b));
println("======================");

// 3. 字符串操作测试
println("3. 字符串操作测试:");
var s1 = "Hello";
var s2 = "World";
println("字符串拼接: " + s1 + " " + s2);
println("字符串长度: " + s1.length);
println("字符访问: " + s1.at(1));
println("子字符串: " + s1.substring(1, 4));
println("======================");

// 4. 数组测试
println("4. 数组操作测试:");
var arr = [1, 2, 3, 4, 5];
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
var x = 10;
var y = 20;
if (x < y) {
    println("x < y: true");
} else {
    println("x < y: false");
}

var result = x > 5 ? "大于5" : "小于等于5";
println("三元运算符: " + result);
println("======================");

// 6. 循环测试
println("6. 循环测试:");
println("while 循环:");
var count = 0;
while (count < 5) {
    println(count);
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

var multiply = (x, y) => x * y;
println("multiply(5, 4) = " + multiply(5, 4));

var square = x => x * x;
println("square(6) = " + square(6));

var noParam = () => "No parameters";
println("noParam() = " + noParam());

println("======================");

// 8. 阶乘函数（递归）
function factorial(n) {
    if (n == 0 || n == 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

println("8. 递归函数测试:");
println("factorial(5) = " + factorial(5));
println("======================");


class Person {
    constructor(name, age) {
        this.name = name;
        this.age = age;
    }

    introduce() {
        return "My name is " + this.name + " and I am " + this.age + " years old.";
    }
}

println("9. 类和对象测试:");
var person = new Person("Alice", 30);
println(person.introduce());
println("======================");

println("10. 导入导出测试:");

import {PI} from "util.js";

println("PI = " + PI);
println("======================");

println("11. 系统函数测试:");
var timestamp = now();
println("当前时间戳: " + timestamp);
setTimeout(function () {
    println("1秒后执行的定时任务");
}, 1000);
var intervalId = setInterval(() => {
    println("每隔1秒执行的定时任务");
}, 1000);
setTimeout(() => {
    clearInterval(intervalId);
    println("已停止每隔1秒执行的定时任务");
}, 5000);
println("======================");

println("=== 所有基础测试完成 ===");
