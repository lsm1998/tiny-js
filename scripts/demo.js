// let util = require("util.js");
//
//
// let add = util[0];
//
// print("2 + 3 = " + add(2, 3));

let ts = 100;

println("ts= " + ts);

ts = 101;

println(ts);

println(now());

function hello() {
    println("Hello, World!");
}

hello();


class Person {
    init(name) {
        this.name = name;
    }

    sayHello() {
        println("Hello, I am " + this.name);
    }
}