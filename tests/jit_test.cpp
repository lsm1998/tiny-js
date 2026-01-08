#include "jit.h"
#include "object.h"
#include <iostream>
#include <vector>

int main()
{
    JitCompiler compiler;

    // 测试 1: (10 + 20) - 5 = 25
    Chunk chunk1;
    chunk1.write(static_cast<uint8_t>(OpCode::OP_CONSTANT));
    chunk1.write(static_cast<uint8_t>(chunk1.addConstant(10.0)));
    chunk1.write(static_cast<uint8_t>(OpCode::OP_CONSTANT));
    chunk1.write(static_cast<uint8_t>(chunk1.addConstant(20.0)));
    chunk1.write(static_cast<uint8_t>(OpCode::OP_ADD));
    chunk1.write(static_cast<uint8_t>(OpCode::OP_CONSTANT));
    chunk1.write(static_cast<uint8_t>(chunk1.addConstant(5.0)));
    chunk1.write(static_cast<uint8_t>(OpCode::OP_SUB));
    chunk1.write(static_cast<uint8_t>(OpCode::OP_RETURN));

    // 测试 2: (20 * 5) / 4 = 25
    Chunk chunk2;
    chunk2.write(static_cast<uint8_t>(OpCode::OP_CONSTANT));
    chunk2.write(static_cast<uint8_t>(chunk2.addConstant(20.0)));
    chunk2.write(static_cast<uint8_t>(OpCode::OP_CONSTANT));
    chunk2.write(static_cast<uint8_t>(chunk2.addConstant(5.0)));
    chunk2.write(static_cast<uint8_t>(OpCode::OP_MUL));
    chunk2.write(static_cast<uint8_t>(OpCode::OP_CONSTANT));
    chunk2.write(static_cast<uint8_t>(chunk2.addConstant(4.0)));
    chunk2.write(static_cast<uint8_t>(OpCode::OP_DIV));
    chunk2.write(static_cast<uint8_t>(OpCode::OP_RETURN));

    std::cout << "=== 测试 1: (10 + 20) - 5 ===" << std::endl;
    if (const auto func1 = compiler.compile(&chunk1))
    {
        double args[1] = {0.0};
        const double result1 = func1(args);
        std::cout << "JIT 执行结果: " << result1 << std::endl;

        if (result1 == 25.0)
        {
            std::cout << "✅ 成功!" << std::endl;
        }
        else
        {
            std::cout << "❌ 失败!" << std::endl;
            std::cout << "期望结果: 25.0" << std::endl;
        }
    }
    else
    {
        std::cout << "JIT 编译失败" << std::endl;
    }

    std::cout << std::endl;
    std::cout << "=== 测试 2: (20 * 5) / 4 ===" << std::endl;
    if (const auto func2 = compiler.compile(&chunk2))
    {
        double args[1] = {0.0};
        const double result2 = func2(args);
        std::cout << "JIT 执行结果: " << result2 << std::endl;

        if (result2 == 25.0)
        {
            std::cout << "✅ 成功!" << std::endl;
        }
        else
        {
            std::cout << "❌ 失败!" << std::endl;
            std::cout << "期望结果: 25.0" << std::endl;
        }
    }
    else
    {
        std::cout << "JIT 编译失败" << std::endl;
    }

    return 0;
}
