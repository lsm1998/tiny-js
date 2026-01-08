#include <string>
#include "vm.h"

constexpr auto MAIN_FILE = "main.js";

int main(const int argc, char* argv[])
{
    VM vm;
    vm.initModule();
    vm.registerNative();
    vm.enableJIT(true);

    std::string entryFile = MAIN_FILE;
    if (argc > 1)
    {
        entryFile = argv[1];
    }

    vm.runWithFile(entryFile);
}
