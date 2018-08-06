/**
 * @file bindings.h
 * @brief Provides methods related to key/mouse bindings and window matching rules
 *
 */
#ifndef BINDINGS_H_
#define BINDINGS_H_

#include "mywm-structs.h"

/**
 * Used to create a binding to a function that takes no arguments and
 * returns void
 * @param F - the function to bind to
 */
#define BIND_TO_FUNC(F)    {.func=F,.type=NO_ARGS}

/**
 * Used to create a binding to a function that returns void and takes a
 * single int as an argument
 * @param F the function to bind to
 * @param I the int to pass to the function
 */
#define BIND_TO_INT_FUNC(F,I)    {.func={.funcInt=F},.arg={.intArg=I},.type=INT_ARG}

/**
 * Used to create a binding to a function that returns void and takes a
 * single pointer as an argument
 * @param F the function to bind to
 * @param V a pointer that will be passed to F when the function is called
 */
#define BIND_TO_VOID_FUNC(F,V) {.func={.funcVoidArg=F},.arg={.voidArg=V},.type=VOID_ARG}

/**
 * Used to create a binding to a function that returns void and takes a
 * single char* as an argument
 * @param F the function to bind to
 * @param S char* pointer that will be passed to F when the function is called
 */
#define BIND_TO_STR_FUNC(F,S) BIND_TO_VOID_FUNC(F,S)

/**
 * Used to create a binding to a function that returns void and takes a
 * single WindowInfo* as an argument.
 *
 * The window passed to the function will change with the context.
 * Generally the active window will be passed to the function
 * @param F the function to bind to
 */
#define BIND_TO_WIN_FUNC(F)    {.func={.funcVoidArg=(void (*)(void *))F},.type=WIN_ARG}

/**
 * Used to create a binding to a function that returns void and takes a
 * single Rule*[] as an argument.
 *
 * @param F the function to bind to
 * @param the [] to pass
 */
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

/**
 * Calls the bound function
 * @param boundFunction
 * @param info
 * @see BoundFunction
 */
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
int doesBindingMatch(Binding*binding,int detail,int mods);
int doesStringMatchRule(Rules*rule,char*str);
int doesWindowMatchRule(Rules *rules,WindowInfo*info);
int applyRule(Rules*rule,WindowInfo*info);

/**
 * Iterates over head and applies the stored rules in order stopping when a rule matches
 * and its passThrough value is false
 * @param head
 * @param info
 * @return 1 if no rule was matched that had passThrough == false
 */
int applyEventRules(Node* head,WindowInfo*info);


int checkBindings(int keyCode,int mods,int bindingType,WindowInfo*info);
#endif
