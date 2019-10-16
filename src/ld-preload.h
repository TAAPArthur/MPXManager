

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <dlfcn.h>

#include <stdlib.h>

#define BASE_NAME(SymbolName) base_ ## SymbolName
#define TYPE_NAME(SymbolName) SymbolName ## _t
#define INTERCEPT(ReturnType, SymbolName, ...)  \
	typedef ReturnType (*TYPE_NAME(SymbolName))(__VA_ARGS__); \
	ReturnType SymbolName(__VA_ARGS__){ \
	void * const BASE_NAME(SymbolName) __attribute__((unused))= dlsym(RTLD_NEXT, # SymbolName); \

#define END_INTERCEPT }
#define BASE(SymbolName) ((TYPE_NAME(SymbolName))BASE_NAME(SymbolName))
