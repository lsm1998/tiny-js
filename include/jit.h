#pragma once
#include "object.h"
#include <bit>
#include <asmjit/asmjit.h>
#include <iostream>

using namespace asmjit;

class JitCompiler
{
    JitRuntime rt;

public:
    // 定义 JIT 函数类型，接受参数并返回结果
    using JitFn = double (*)(double* args);

    JitFn compile(Chunk* chunk)
    {
        std::cout << "Attempting to JIT compile function with " << chunk->code.size() << " bytes of bytecode" << std::endl;
        for (int i = 0; i < chunk->code.size(); i++)
        {
            std::cout << "  Offset " << i << ": " << static_cast<int>(chunk->code[i]) << std::endl;
        }

        CodeHolder code;
        code.init(rt.environment());

        // 根据架构选择编译器
        if (rt.environment().is_family_x86()) {
            return compileX86(chunk, code);
        } else if (rt.environment().is_family_aarch64()) {
            return compileAArch64(chunk, code);
        } else {
            std::cout << "Unsupported architecture for JIT compilation" << std::endl;
            return nullptr;
        }
    }

private:
    JitFn compileX86(Chunk* chunk, CodeHolder& code) {
        x86::Compiler cc(&code);

        // 开始定义函数
        FuncNode* func_node;
        Error err = cc.add_func_node(Out(func_node), FuncSignature::build<double, uint64_t>());
        if (err != Error::kOk) {
            std::cout << "Failed to create FuncNode: " << DebugUtils::error_as_string(err) << std::endl;
            return nullptr;
        }

        std::cout << "func_node: " << func_node << std::endl;

        // 获取参数 - 使用虚拟寄存器
        x86::Gp args = cc.new_gp64();
        func_node->set_arg(0, args);

        // 处理字节码
        int ip = 0;
        bool jitFailed = false;

        while (ip < chunk->code.size() && !jitFailed)
        {
            uint8_t instruction = chunk->code[ip++];
            std::cout << "Processing instruction: " << static_cast<int>(instruction) << std::endl;

            switch (instruction)
            {
            case (uint8_t)OpCode::OP_GET_LOCAL:
                {
                    std::cout << "Processing OP_GET_LOCAL" << std::endl;
                    uint8_t idx = chunk->code[ip++];
                    x86::Vec xmm0 = cc.new_xmm();
                    // args 是 double* 类型，索引 idx 的参数是 args[idx]
                    cc.movsd(xmm0, x86::ptr(args, idx * 8));
                    // 压入到评估栈
                    cc.sub(x86::rsp, 8);
                    cc.movsd(x86::Mem(x86::rsp, 0), xmm0);
                    break;
                }
            case (uint8_t)OpCode::OP_ADD:
                {
                    std::cout << "Processing OP_ADD" << std::endl;
                    x86::Vec xmm0 = cc.new_xmm();
                    x86::Vec xmm1 = cc.new_xmm();
                    // 从栈中弹出两个操作数
                    cc.movsd(xmm0, x86::Mem(x86::rsp, 0));
                    cc.add(x86::rsp, 8);
                    cc.movsd(xmm1, x86::Mem(x86::rsp, 0));
                    cc.add(x86::rsp, 8);
                    // 相加
                    cc.addsd(xmm0, xmm1);
                    // 压回栈
                    cc.sub(x86::rsp, 8);
                    cc.movsd(x86::Mem(x86::rsp, 0), xmm0);
                    break;
                }
            case (uint8_t)OpCode::OP_MUL:
                {
                    std::cout << "Processing OP_MUL" << std::endl;
                    x86::Vec xmm0 = cc.new_xmm();
                    x86::Vec xmm1 = cc.new_xmm();
                    // 从栈中弹出两个操作数
                    cc.movsd(xmm0, x86::Mem(x86::rsp, 0));
                    cc.add(x86::rsp, 8);
                    cc.movsd(xmm1, x86::Mem(x86::rsp, 0));
                    cc.add(x86::rsp, 8);
                    // 相乘
                    cc.mulsd(xmm0, xmm1);
                    // 压回栈
                    cc.sub(x86::rsp, 8);
                    cc.movsd(x86::Mem(x86::rsp, 0), xmm0);
                    break;
                }
            case (uint8_t)OpCode::OP_RETURN:
                {
                    std::cout << "Processing OP_RETURN" << std::endl;
                    x86::Vec result = cc.new_xmm();
                    // 从栈中取出返回值
                    cc.movsd(result, x86::Mem(x86::rsp, 0));
                    cc.add(x86::rsp, 8);
                    // 返回结果
                    cc.ret(result);
                    goto finalize;
                }
            default:
                jitFailed = true;
                std::cout << "Unsupported operation: " << static_cast<int>(instruction) << std::endl;
                break;
            }
        }

    finalize:
        if (jitFailed)
        {
            std::cout << "JIT compilation failed due to unsupported operation" << std::endl;
            return nullptr;
        }

        // 结束函数定义
        cc.end_func();

        std::cout << "Attempting to finalize JIT compilation" << std::endl;
        cc.finalize();

        // 检查 CodeHolder 是否包含任何代码
        if (code.sections().size() == 0 || code.sections()[0] == nullptr || code.sections()[0]->buffer_size() == 0)
        {
            std::cout << "JIT compilation failed: No code generated" << std::endl;
            return nullptr;
        }

        JitFn fn;
        err = rt.add(&fn, &code);
        if (err != Error::kOk)
        {
            std::cout << "JIT compilation failed during runtime registration: " << DebugUtils::error_as_string(err) << std::endl;
            return nullptr;
        }
        std::cout << "JIT compilation succeeded" << std::endl;
        return fn;
    }

    JitFn compileAArch64(Chunk* chunk, CodeHolder& code) {
        a64::Compiler cc(&code);

        // 开始定义函数
        FuncNode* func_node;
        Error err = cc.add_func_node(Out(func_node), FuncSignature::build<double, uint64_t>());
        if (err != Error::kOk) {
            std::cout << "Failed to create FuncNode: " << DebugUtils::error_as_string(err) << std::endl;
            return nullptr;
        }

        std::cout << "func_node: " << func_node << std::endl;

        // 获取参数 - 使用虚拟寄存器
        a64::Gp args = cc.new_gp64();
        func_node->set_arg(0, args);

        // 处理字节码
        int ip = 0;
        bool jitFailed = false;

        while (ip < chunk->code.size() && !jitFailed)
        {
            uint8_t instruction = chunk->code[ip++];
            std::cout << "Processing instruction: " << static_cast<int>(instruction) << std::endl;

            switch (instruction)
            {
            case (uint8_t)OpCode::OP_GET_LOCAL:
                {
                    std::cout << "Processing OP_GET_LOCAL" << std::endl;
                    uint8_t idx = chunk->code[ip++];
                    auto d0 = cc.new_vec_d();
                    // args 是 double* 类型，索引 idx 的参数是 args[idx]
                    cc.ldr(d0, a64::ptr(args, idx * 8));
                    // 压入到评估栈
                    cc.sub(a64::regs::sp, 8);
                    cc.str(d0, a64::ptr(a64::regs::sp, 0));
                    break;
                }
            case (uint8_t)OpCode::OP_ADD:
                {
                    std::cout << "Processing OP_ADD" << std::endl;
                    auto d0 = cc.new_vec_d();
                    auto d1 = cc.new_vec_d();
                    // 从栈中弹出两个操作数
                    cc.ldr(d0, a64::ptr(a64::regs::sp, 0));
                    cc.add(a64::regs::sp, 8);
                    cc.ldr(d1, a64::ptr(a64::regs::sp, 0));
                    cc.add(a64::regs::sp, 8);
                    // 相加
                    cc.fadd(d0, d1, d0);
                    // 压回栈
                    cc.sub(a64::regs::sp, 8);
                    cc.str(d0, a64::ptr(a64::regs::sp, 0));
                    break;
                }
            case (uint8_t)OpCode::OP_MUL:
                {
                    std::cout << "Processing OP_MUL" << std::endl;
                    auto d0 = cc.new_vec_d();
                    auto d1 = cc.new_vec_d();
                    // 从栈中弹出两个操作数
                    cc.ldr(d0, a64::ptr(a64::regs::sp, 0));
                    cc.add(a64::regs::sp, 8);
                    cc.ldr(d1, a64::ptr(a64::regs::sp, 0));
                    cc.add(a64::regs::sp, 8);
                    // 相乘
                    cc.fmul(d0, d1, d0);
                    // 压回栈
                    cc.sub(a64::regs::sp, 8);
                    cc.str(d0, a64::ptr(a64::regs::sp, 0));
                    break;
                }
            case (uint8_t)OpCode::OP_RETURN:
                {
                    std::cout << "Processing OP_RETURN" << std::endl;
                    auto result = cc.new_vec_d();
                    // 从栈中取出返回值
                    cc.ldr(result, a64::ptr(a64::regs::sp, 0));
                    cc.add(a64::regs::sp, 8);
                    // 返回结果
                    cc.ret(result);
                    goto finalize;
                }
            default:
                jitFailed = true;
                std::cout << "Unsupported operation: " << static_cast<int>(instruction) << std::endl;
                break;
            }
        }

    finalize:
        if (jitFailed)
        {
            std::cout << "JIT compilation failed due to unsupported operation" << std::endl;
            return nullptr;
        }

        // 结束函数定义
        cc.end_func();

        std::cout << "Attempting to finalize JIT compilation" << std::endl;
        cc.finalize();

        // 检查 CodeHolder 是否包含任何代码
        if (code.sections().size() == 0 || code.sections()[0] == nullptr || code.sections()[0]->buffer_size() == 0)
        {
            std::cout << "JIT compilation failed: No code generated" << std::endl;
            return nullptr;
        }

        JitFn fn;
        err = rt.add(&fn, &code);
        if (err != Error::kOk)
        {
            std::cout << "JIT compilation failed during runtime registration: " << DebugUtils::error_as_string(err) << std::endl;
            return nullptr;
        }
        std::cout << "JIT compilation succeeded" << std::endl;
        return fn;
    }
};