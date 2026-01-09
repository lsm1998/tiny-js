#ifndef TINY_JS_COMMON_H
#define TINY_JS_COMMON_H

#include <variant>
#include <string>
#include <sstream>
#include <fstream>
#include <cmath>
#include <cstdint>
#include <memory>

// #define DEBUG

struct Chunk;

struct Obj;

using Value = std::variant<std::monostate, bool, double, Obj*>;

inline std::string readFile(const std::string& path)
{
    std::ifstream t(path);
    if (!t.is_open()) return "";
    std::stringstream buffer;
    buffer << t.rdbuf();
    return buffer.str();
}

#endif //TINY_JS_COMMON_H
