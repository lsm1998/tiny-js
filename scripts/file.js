var f = File("test.txt", "w");
f.write("Hello from MiniJS!");
f.close();


f = File("test.txt", "r");
var content = f.read();
f.close();
println(content);

println("文件大小: " + f.size());

// Clean up
f = File("test.txt");
println("删除结果: " + f.remove());