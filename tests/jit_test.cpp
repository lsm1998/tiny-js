#include "object.h"
#include "parser.h"
#include "scanner.h"
#include <iostream>

#include "compiler.h"

constexpr auto SCRIPT = R"(
    function add(a, b) {
    return a + b;
}

function multiply(a, b) {
    return a * b;
}

let result = add(5, 10);

println("Addition Result: " + result);
    )";

int main()
{
    Scanner scanner(SCRIPT);
    const auto tokens = scanner.scanTokens();
    Parser parser(tokens);

    const auto stmts = parser.parse();

    VM vm;
    Compiler compiler(vm);
    ObjFunction* function = compiler.compile(stmts);
    for (const auto& byte : function->chunk.code)
    {
        std::cout << opCodeNames[byte] << "\n";
    }
    return 0;
}
