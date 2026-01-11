#ifndef TINY_JS_BASE_H
#define TINY_JS_BASE_H

#include "vm.h"

Value nativePrint(VM& vm, int argc, const Value* args);

Value nativePrintln(VM& vm, int argc, const Value* args);

Value nativeNow(VM& vm, int argc, const Value* args);

Value nativeSleep(VM& vm, int argc, const Value* args);

Value nativeGetEnv(VM& vm, int argc, const Value* args);

Value nativeSetEnv(VM& vm, int argc, const Value* args);

Value nativeExit(VM& vm, int argc, const Value* args);

Value nativeSetTimeout(VM& vm, int argc, const Value* args);

Value nativeSetInterval(VM& vm, int argc, const Value* args);

Value nativeClearInterval(VM& vm, int argc, const Value* args);

Value nativeTypeof(VM& vm, int argc, const Value* args);

#endif //TINY_JS_BASE_H
