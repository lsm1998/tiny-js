#include "jit.h"
#include "object.h"
#include "parser.h"
#include <iostream>

#include "compiler.h"
#include "scanner.h"

void testWithChunk()
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
    }
    else
    {
        std::cerr << "JIT 编译失败" << std::endl;
    }

    std::cout << std::endl;
    std::cout << "=== 测试 2: (20 * 5) / 4 ===" << std::endl;
    if (const auto func2 = compiler.compile(&chunk2))
    {
        double args[1] = {0.0};
        const double result2 = func2(args);
        std::cout << "JIT 执行结果: " << result2 << std::endl;
    }
    else
    {
        std::cerr << "JIT 编译失败" << std::endl;
    }

    // 测试 3: 10 % 3 = 1
    Chunk chunk3;
    chunk3.write(static_cast<uint8_t>(OpCode::OP_CONSTANT));
    chunk3.write(static_cast<uint8_t>(chunk3.addConstant(10.0)));
    chunk3.write(static_cast<uint8_t>(OpCode::OP_CONSTANT));
    chunk3.write(static_cast<uint8_t>(chunk3.addConstant(3.0)));
    chunk3.write(static_cast<uint8_t>(OpCode::OP_MOD));
    chunk3.write(static_cast<uint8_t>(OpCode::OP_RETURN));

    std::cout << std::endl;
    std::cout << "=== 测试 3: 10 % 3 ===" << std::endl;
    if (const auto func3 = compiler.compile(&chunk3))
    {
        double args[1] = {0.0};
        const double result3 = func3(args);
        std::cout << "JIT 执行结果: " << result3 << std::endl;
    }
    else
    {
        std::cerr << "JIT 编译失败" << std::endl;
    }
}

void testWithScript()
{
    JitCompiler compiler;

    // 测试脚本: return (15 + 10) * 2;
    const std::string script = R"(
        return (15 + 10) * 2;
    )";

    Scanner scanner(script);
    const auto tokens = scanner.scanTokens();
    Parser parser(tokens);
    const auto stmts = parser.parse();


    VM vm;
    Compiler comp(vm);

    ObjFunction* function = comp.compile(stmts);
    auto chunk = function->chunk;

    std::cout << "=== 测试脚本: return (15 + 10) * 2; ===" << std::endl;
    if (const auto func = compiler.compile(&chunk))
    {
        double args[1] = {0.0};
        const double result = func(args);
        std::cout << "JIT 执行结果: " << result << std::endl;
    }
    else
    {
        std::cerr << "JIT 编译失败" << std::endl;
    }
}

int main()
{
    testWithChunk();

    testWithScript();
}
