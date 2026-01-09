println("开始执行");


sleep(1000);

println("休眠结束");

setTimeout(function () {
    println("两秒后执行的代码");
}, 2000);

setTimeout(function () {
    println("三秒后执行的代码");
}, 3000);

println("脚本执行完毕");