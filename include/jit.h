#pragma once

#include "debug.h"
#include "object.h"
#include <bit>
#include <asmjit/asmjit.h>
#include <iostream>
#include "asmjit/arm/a64compiler.h"

using namespace asmjit;

class JitCompiler
{
    JitRuntime rt;

public:
    // 定义 JIT 函数类型，接受参数并返回结果
    using JitFn = double (*)(double* args);

    JitFn compile(const Chunk* chunk)
    {
        CodeHolder code;
        code.init(rt.environment());

        // 根据架构选择编译器
        if (rt.environment().is_family_x86())
        {
            return compileX86(chunk, code);
        }
        if (rt.environment().is_family_aarch64())
        {
            return compileAArch64(chunk, code);
        }
        std::cout << "Unsupported architecture for JIT compilation" << std::endl;
        return nullptr;
    }

private:
    JitFn compileX86(const Chunk* chunk, CodeHolder& code)
    {
        x86::Compiler cc(&code);

        FuncNode* func_node;
        Error err = cc.add_func_node(Out(func_node), FuncSignature::build<double, uint64_t>());
        if (err != Error::kOk)
        {
            std::cout << "Failed to create FuncNode: " << DebugUtils::error_as_string(err) << std::endl;
            return nullptr;
        }

        // 获取参数 - 使用虚拟寄存器
        x86::Gp args = cc.new_gp64();
        func_node->set_arg(0, args);

        // 处理字节码
        int ip = 0;
        bool jitFailed = false;

        while (ip < chunk->code.size() && !jitFailed)
        {
            const uint8_t instruction = chunk->code[ip++];
            debug_log("处理指令: {}", static_cast<int>(instruction));

            switch (instruction)
            {
            case static_cast<uint8_t>(OpCode::OP_CONSTANT):
                {
                    const uint8_t idx = chunk->code[ip++];
                    const double value = std::get<double>(chunk->constants[idx]);
                    x86::Vec xmm0 = cc.new_xmm();
                    x86::Gp temp = cc.new_gp64();
                    cc.mov(temp, *reinterpret_cast<const uint64_t*>(&value));
                    cc.movq(xmm0, temp);
                    cc.sub(x86::rsp, 8);
                    cc.movsd(x86::Mem(x86::rsp, 0), xmm0);
                    break;
                }
            case static_cast<uint8_t>(OpCode::OP_GET_LOCAL):
                {
                    const uint8_t idx = chunk->code[ip++];
                    x86::Vec xmm0 = cc.new_xmm();
                    cc.movsd(xmm0, x86::ptr(args, idx * 8));
                    cc.sub(x86::rsp, 8);
                    cc.movsd(x86::Mem(x86::rsp, 0), xmm0);
                    break;
                }
            case static_cast<uint8_t>(OpCode::OP_SET_LOCAL):
                {
                    const uint8_t idx = chunk->code[ip++];
                    x86::Vec xmm0 = cc.new_xmm();
                    cc.movsd(xmm0, x86::Mem(x86::rsp, 0));
                    cc.add(x86::rsp, 8);
                    cc.movsd(x86::ptr(args, idx * 8), xmm0);
                    break;
                }
            case static_cast<uint8_t>(OpCode::OP_ADD):
                {
                    debug_log("处理 OP_ADD");
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
            case static_cast<uint8_t>(OpCode::OP_MUL):
                {
                    debug_log("处理 OP_MUL");
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
            case static_cast<uint8_t>(OpCode::OP_RETURN):
                {
                    debug_log("处理 OP_RETURN");
                    const x86::Vec result = cc.new_xmm();
                    // 从栈中取出返回值
                    cc.movsd(result, x86::Mem(x86::rsp, 0));
                    cc.add(x86::rsp, 8);
                    // 返回结果
                    cc.ret(result);
                    goto finalize;
                }
            default:
                jitFailed = true;
                debug_log("不支持的操作: {}", static_cast<int>(instruction));
                break;
            }
        }

    finalize:
        if (jitFailed)
        {
            debug_log("JIT 编译失败，遇到不支持的操作");
            return nullptr;
        }

        cc.end_func();

        cc.finalize();

        // 检查 CodeHolder 是否包含任何代码
        if (code.sections().size() == 0 || code.sections()[0] == nullptr || code.sections()[0]->buffer_size() == 0)
        {
            debug_log("JIT 编译失败: 未生成代码");
            return nullptr;
        }

        JitFn fn;
        err = rt.add(&fn, &code);
        if (err != Error::kOk)
        {
            std::cout << "JIT compilation failed during runtime registration: " << DebugUtils::error_as_string(err) <<
                std::endl;
            return nullptr;
        }
        debug_log("JIT 编译完成");
        return fn;
    }

    JitFn compileAArch64(const Chunk* chunk, CodeHolder& code)
    {
        a64::Compiler cc(&code);

        // 开始定义函数
        FuncNode* func_node;
        Error err = cc.add_func_node(Out(func_node), FuncSignature::build<double, uint64_t>());
        if (err != Error::kOk)
        {
            std::cout << "Failed to create FuncNode: " << DebugUtils::error_as_string(err) << std::endl;
            return nullptr;
        }

        // 获取参数 - 使用虚拟寄存器
        const a64::Gp args = cc.new_gp64();
        func_node->set_arg(0, args);

        // 函数序言 - 分配足够的栈空间（使用固定大小的栈帧，避免动态调整sp）
        cc.sub(a64::regs::sp, a64::regs::sp, 128);  // 分配128字节的固定栈帧
        cc.stp(a64::regs::x29, a64::regs::x30, a64::ptr(a64::regs::sp, 0));
        cc.mov(a64::regs::x29, a64::regs::sp);

        // 使用一个虚拟寄存器作为栈指针偏移（避免直接修改sp导致对齐问题）
        a64::Gp stackOffset = cc.new_gp64();
        cc.mov(stackOffset, 16);  // 栈帧开始在sp+16处（因为x29和x30占据了0-15字节）

        // 处理字节码
        int ip = 0;
        bool jitFailed = false;

        while (ip < chunk->code.size() && !jitFailed)
        {
            switch (const uint8_t instruction = chunk->code[ip++])
            {
            case static_cast<uint8_t>(OpCode::OP_CONSTANT):
                {
                    const uint8_t idx = chunk->code[ip++];
                    const double value = std::get<double>(chunk->constants[idx]);
                    auto d0 = cc.new_vec_d();
                    cc.fmov(d0, value);
                    cc.str(d0, a64::ptr(a64::regs::sp, stackOffset));
                    cc.add(stackOffset, stackOffset, 8);
                    break;
                }
            case static_cast<uint8_t>(OpCode::OP_GET_LOCAL):
                {
                    const uint8_t idx = chunk->code[ip++];
                    auto d0 = cc.new_vec_d();
                    // 编译器中第一个参数索引是 1，而 VM 传递的 args 数组索引是 0
                    if (idx > 0)
                    {
                        cc.ldr(d0, a64::ptr(args, (idx - 1) * 8));
                    }
                    else
                    {
                        cc.fmov(d0, 0.0);
                    }
                    cc.str(d0, a64::ptr(a64::regs::sp, stackOffset));
                    cc.add(stackOffset, stackOffset, 8);
                    break;
                }
            case static_cast<uint8_t>(OpCode::OP_SET_LOCAL):
                {
                    debug_log("处理 OP_SET_LOCAL");
                    const uint8_t idx = chunk->code[ip++];
                    auto d0 = cc.new_vec_d();
                    cc.sub(stackOffset, stackOffset, 8);
                    cc.ldr(d0, a64::ptr(a64::regs::sp, stackOffset));
                    // 编译器中第一个参数索引是 1，而 VM 传递的 args 数组索引是 0
                    if (idx > 0)
                    {
                        cc.str(d0, a64::ptr(args, (idx - 1) * 8));
                    }
                    break;
                }
            case static_cast<uint8_t>(OpCode::OP_ADD):
                {
                    auto d0 = cc.new_vec_d();
                    auto d1 = cc.new_vec_d();
                    cc.sub(stackOffset, stackOffset, 8);
                    cc.ldr(d0, a64::ptr(a64::regs::sp, stackOffset));  // 弹出 b
                    cc.sub(stackOffset, stackOffset, 8);
                    cc.ldr(d1, a64::ptr(a64::regs::sp, stackOffset));  // 弹出 a
                    cc.fadd(d1, d1, d0);  // 计算 a + b
                    cc.str(d1, a64::ptr(a64::regs::sp, stackOffset));
                    cc.add(stackOffset, stackOffset, 8);
                    break;
                }
            case static_cast<uint8_t>(OpCode::OP_MUL):
                {
                    debug_log("处理 OP_MUL");
                    auto d0 = cc.new_vec_d();
                    auto d1 = cc.new_vec_d();
                    cc.sub(stackOffset, stackOffset, 8);
                    cc.ldr(d0, a64::ptr(a64::regs::sp, stackOffset));  // 弹出 b
                    cc.sub(stackOffset, stackOffset, 8);
                    cc.ldr(d1, a64::ptr(a64::regs::sp, stackOffset));  // 弹出 a
                    cc.fmul(d1, d1, d0);  // 计算 a * b
                    cc.str(d1, a64::ptr(a64::regs::sp, stackOffset));
                    cc.add(stackOffset, stackOffset, 8);
                    break;
                }
            case static_cast<uint8_t>(OpCode::OP_RETURN):
                {
                    auto result = cc.new_vec_d();
                    cc.sub(stackOffset, stackOffset, 8);
                    cc.ldr(result, a64::ptr(a64::regs::sp, stackOffset));
                    // 函数尾声
                    cc.ldp(a64::regs::x29, a64::regs::x30, a64::ptr(a64::regs::sp, 0));
                    cc.add(a64::regs::sp, a64::regs::sp, 128);
                    cc.fmov(a64::regs::d0, result);
                    cc.ret();
                    goto finalize;
                }
            default:
                jitFailed = true;
                debug_log("不支持的操作: {}", static_cast<int>(instruction));
                break;
            }
        }

    finalize:
        if (jitFailed)
        {
            debug_log("JIT 编译失败，遇到不支持的操作");
            return nullptr;
        }

        cc.end_func();
        cc.finalize();

        if (code.sections().size() == 0 || code.sections()[0] == nullptr || code.sections()[0]->buffer_size() == 0)
        {
            debug_log("JIT 编译失败: 未生成代码");
            return nullptr;
        }

        JitFn fn;
        err = rt.add(&fn, &code);
        if (err != Error::kOk)
        {
            std::cout << "JIT compilation failed during runtime registration: " << DebugUtils::error_as_string(err) << std::endl;
            return nullptr;
        }
        return fn;
    }
};
