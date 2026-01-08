#include "object.h"
#include "parser.h"
#include "scanner.h"
#include <iostream>

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
    return 0;
}
