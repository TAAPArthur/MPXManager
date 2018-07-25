
#ifndef BINDINGS_H_
#define BINDINGS_H_



#include "mywm-structs.h"

extern Node* eventRules[];



#define BIND_TO_FUNC(F)    {.func=F,.type=NO_ARGS}
#define BIND_TO_INT_FUNC(F,I)    {.func={.funcInt=F},.arg={.intArg=I},.type=INT_ARG}

#define BIND_TO_VOID_FUNC(F,S) {.func={.funcVoidArg=F},.arg={.voidArg=S},.type=VOID_ARG}
#define BIND_TO_STR_FUNC(F,S) BIND_TO_VOID_FUNC(F,S)

#define BIND_TO_WIN_FUNC(F)    {.func={.funcVoidArg=(void (*)(void *))F},.type=WIN_ARG}

#define BIND_TO_RULE_FUNC(F,...) {.func={.funcRuleArg=F},.arg={.voidArg=(Rules*)(Rules[]){__VA_ARGS__}},.type=VOID_ARG}

#define CHAIN(...) CHAIN_GRAB(1,__VA_ARGS__)
#define CHAIN_GRAB(GRAB, ...) {.func={.chainBindings=(Binding*)(Binding[]){__VA_ARGS__}},.arg={.intArg=GRAB},.type=CHAIN}
#define END_CHAIN(...) {__VA_ARGS__,.endChain=1}

#define CREATE_DEFAULT_EVENT_RULE(func) CREATE_WILDCARD(BIND_TO_FUNC(func),.passThrough=1)
#define CREATE_WILDCARD(...) {NULL,0,__VA_ARGS__}
#define CREATE_RULE(expression,target,func) {expression,target,func}
#define CREATE_LITERAL_RULE(E,T,F) {E,(T|LITERAL),F}

#define COMPILE_RULE(R) \
     if((R->ruleTarget & LITERAL) == 0){\
        R->regexExpression=malloc(sizeof(regex_t));\
        assert(0==regcomp(R->regexExpression,R->literal, \
                (DEFAULT_REGEX_FLAG)|((R->ruleTarget & CASE_INSENSITIVE)?REG_ICASE:0)));\
    }


void callBoundedFunction(BoundFunction*boundFunction,WindowInfo*info);
void grabBindings(Binding*chain);
Binding* getEndOfChain(Binding*chain);

/**
 * Comapres Bindings with detail and mods to see if it should be triggered.
 * If bindings->mod ==anyModier, then it will match any modiers.
 * If bindings->detail or keycode is 0 it will match any detail
 * @param binding
 * @param notMouse whether detail refress to a keycode or a button
 * @param detail either a keycode or a button
 * @param mods modiers
 * @return Returns true if the binding should be triggered based on the combination of detail and mods
 */
int doesBindingMatch(Binding*binding,int notMouse,int detail,int mods);
int doesStringMatchRule(Rules*rule,char*str);
int doesWindowMatchRule(Rules *rules,WindowInfo*info);
int applyRule(Rules*rule,WindowInfo*info);

int applyEventRules(Node* head,WindowInfo*info);


int checkBindings(int keyCode,int mods,int bindingType,WindowInfo*info);
#endif
