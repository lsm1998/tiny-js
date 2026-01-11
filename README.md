# Tiny-JS

Tiny-JS 是一个用 C++20 实现的轻量级类 JavaScript 的脚本语言。

## 项目特点

- **轻量级设计**：专注于核心 JavaScript 功能的实现
- **C++20 特性**：充分利用现代 C++ 语言特性
- **完整的解释器架构**：包含扫描器（Scanner）、解析器（Parser）、编译器（Compiler）和虚拟机（VM）
- **支持的语言特性**：
  - 变量声明（var、let、const）
  - 函数定义和调用
  - 类定义和实例化
  - 数组和字符串操作
  - 模块系统（import/export）
  - 匿名函数和箭头函数
  - 基本数据类型（数字、字符串、布尔值、数组、对象）
  - 控制流语句（if、while、for）
  - 垃圾回收（GC）
  - JIT 编译支持（简单指令）

## 项目结构

```
tiny-js/
├── include/          # 头文件目录
│   ├── ast.h        # 抽象语法树定义
│   ├── compiler.h   # 编译器接口
│   ├── object.h     # 对象系统定义
│   ├── parser.h     # 解析器接口
│   ├── scanner.h    # 扫描器接口
│   ├── token.h      # 标记定义
│   ├── vm.h         # 虚拟机接口
│   ├── common.h     # 通用工具和类型
│   └── native/      # 原生方法定义
│       ├── base.h    # 基础原生方法
│       └── require.h # 模块加载原生方法
├── src/             # 源代码目录
│   ├── compiler.cpp # 编译器实现
│   ├── parser.cpp   # 解析器实现
│   ├── scanner.cpp  # 扫描器实现
│   ├── vm.cpp       # 虚拟机实现
│   └── native/      # 原生方法实现
│       ├── base.cpp    # 基础原生方法
│       └── require.cpp # 模块加载原生方法
├── tests/           # 测试目录
│   └── scanner_test.cpp # 扫描器测试
├── scripts/         # 示例脚本
│   ├── demo.js      # 演示脚本
│   ├── class.js     # 类示例
│   ├── array.js     # 数组示例
│   ├── util.js      # 工具函数
│   └── main.js      # 主入口脚本
├── libs/            # 标准库
│   ├── array.js     # 数组标准库
│   └── obj.js       # 对象标准库
├── CMakeLists.txt   # CMake 构建文件
└── main.cpp         # 项目主入口
```

## 构建和运行

### 构建要求

- C++20 兼容的编译器（如 GCC 10+、Clang 11+、MSVC 2019+）
- CMake 4.1 或更高版本

### 构建步骤

1. 克隆项目到本地
2. 创建构建目录：
   ```bash
   mkdir build && cd build
   ```
3. 运行 CMake 配置：
   ```bash
   cmake ..
   ```
4. 构建项目：
   ```bash
   cmake --build .
   ```

### 运行示例

构建完成后，可执行文件将生成在 `scripts` 目录下。运行默认脚本：

```bash
cd scripts
./tiny_js
```

或者运行指定脚本：

```bash
cd scripts
./tiny_js demo.js
```

## JavaScript 支持的功能

### 变量声明

```javascript
let x = 10;
const y = 20;
```

### 函数定义和调用

```javascript
function add(a, b) {
    return a + b;
}

println(add(2, 3)); // 输出 5
```

### 类定义

```javascript
class Person {
    init(name) {
        this.name = name;
    }

    sayHello() {
        println("Hello, I am " + this.name);
    }
}

let person = Person("Alice");
person.sayHello(); // 输出 "Hello, I am Alice"
```

### 数组操作

```javascript
let arr = [1, 2, 3];
arr.push(4);
println(arr.length); // 输出 4
```

### 模块加载

```javascript
import {add, PI} from "util.js";
let result = add(2, 3);
println(result); // 输出 5
println(PI); // 输出 3.14159
```

## 内置函数

- `println(value)/print(value)`：打印值到控制台
- `now()`：获取当前时间戳
- `sleep(ms)`： 休眠指定毫秒数
- `setTimeout(fn, ms)`：延时执行函数
- `setInterval(fn, ms)`：定时执行函数
- `clearInterval(id)`：清除定时执行
- `setEnv(key, value)`：设置环境变量
- `getEnv(key)`：获取环境变量
- `typeof(obj)`：获取值的类型
- `Object.keys(obj)`：获取对象的键列表（暂未实现）
- `Array.isArray(value)`：检查值是否为数组（暂未实现）

## 垃圾回收

Tiny-JS 实现了标记-清除（Mark and Sweep）垃圾回收算法，自动管理内存。

## 扩展项目

您可以通过以下方式扩展 tiny-js：

1. 在 `src/native/` 目录下添加新的原生方法
2. 在 `include/native/` 目录下声明原生方法接口
3. 在 `VM::registerNative()` 方法中注册新的原生方法

## 许可证

MIT License
