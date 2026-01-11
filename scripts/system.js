println("开始执行");

// 获取环境变量
let path = getEnv("PATH");
println("PATH =", path);

// 设置环境变量
setEnv("TINY_JS_VAR", "HelloWorld");
println("TINY_JS_VAR =", getEnv("TINY_JS_VAR"));

println("1 类型=", typeof (1));
println("'string' 类型=", typeof ("string"));
println("true 类型=", typeof (true));
println("null 类型=", typeof (null));
println("[] 类型=", typeof ([]));
println("function(){} 类型=", typeof (function () {}));

sleep(1000);

println("休眠结束");

setTimeout(function () {
    println("两秒后执行的代码");
}, 2000);

setTimeout(() => {
    println("三秒后执行的代码");
}, 3000);

let id = setInterval(function () {
    println("每秒执行一次的代码");
}, 1000);

setTimeout(function () {
    clearInterval(id);
    println("停止定时器，id=", id);
}, 3500);