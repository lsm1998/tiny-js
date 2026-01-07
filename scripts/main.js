// 单行注释测试
/*
    多行注释测试
*/
var util = require("util.js");

let add = util[0];

println("2 + 3 = " + add(2, 3));

let str = "Hello, ";

function greet(name) {
    return str + name + "!";
}

println(greet("World"));

str = "Count: ";
for (var i = 0; i < 5; i++) {
    str = str + "(" + i + ") ";
    println(str);
}

// str = "当前时间戳=" + now();
// print(str);
//
// var list = [1, 2, 3];
// print(list);
//
// list[0] = "hello";
// print(list[0]);

// class Person {
//     constructor(name, age) {
//         this.name = name;
//         this.age = age;
//     }
//
//     introduce() {
//         return "My name is " + this.name + " and I am " + this.age + " years old.";
//     }
// }
//
// var alice = new Person("Alice", 30);
// print(alice.introduce());