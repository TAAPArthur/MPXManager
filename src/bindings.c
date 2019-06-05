/**
 * @file bindings.c
 * \copybrief bindings.h
 */


#include <assert.h>
#include <ctype.h>
#include <regex.h>
#include <string.h>

#include <xcb/xinput.h>

#include "bindings.h"
#include "devices.h"
#include "globals.h"
#include "logger.h"
#include "masters.h"
#include "xsession.h"


ArrayList* getActiveChains(){
    return &getActiveMaster()->activeChains;
}
/**
 * Push a binding to the active master stack.
 * Should not be called directly
 * @param chain
 */
static void pushBinding(BoundFunction* chain){
    assert(chain);
    addToList(getActiveChains(), chain);
}
/**
 * Pop the last added binding from the master stack
 * @return
 */
static BoundFunction* popActiveBinding(){
    return pop(getActiveChains());
}
Binding* getActiveBinding(){
    return isNotEmpty(getActiveChains()) ? ((BoundFunction*)getLast(getActiveChains()))->func.chainBindings : NULL;
}

int callBoundedFunction(BoundFunction* boundFunction, WindowInfo* winInfo){
    assert(boundFunction);
    LOG_RUN(LOG_LEVEL_VERBOSE, dumpBoundFunction(boundFunction));
    BoundFunctionArg arg = boundFunction->arg;
    if(boundFunction->dynamic == 1){
        if(winInfo == NULL)
            return 0;
        arg.voidArg = winInfo;
    }
    int result = 1;
    switch(boundFunction->type){
        case UNSET:
            LOG(LOG_LEVEL_WARN, "calling unset function; nothing is happening\n");
            break;
        case CHAIN:
            assert(!boundFunction->func.chainBindings->endChain);
            startChain(boundFunction);
            break;
        case CHAIN_AUTO:
            startChain(boundFunction);
            result = callBoundedFunction(&boundFunction->func.chainBindings->boundFunction, winInfo);
            break;
        case FUNC_AND:
            for(int i = 0; i < boundFunction->arg.intArg; i++)
                if(!callBoundedFunction(&boundFunction->func.boundFunctionArr[i], winInfo)){
                    result = 0;
                    break;
                }
            break;
        case FUNC_BOTH:
            result = 0;
            for(int i = 0; i < boundFunction->arg.intArg; i++)
                if(callBoundedFunction(&boundFunction->func.boundFunctionArr[i], winInfo))
                    result = 1;
            break;
        case FUNC_OR:
            result = 0;
            for(int i = 0; i < boundFunction->arg.intArg; i++)
                if(callBoundedFunction(&boundFunction->func.boundFunctionArr[i], winInfo)){
                    result = 1;
                    break;
                }
            break;
        case FUNC_PIPE:
            result = callBoundedFunction(&boundFunction->func.boundFunctionArr[0], winInfo);
            for(int i = 1; i < boundFunction->arg.intArg; i++){
                boundFunction->func.boundFunctionArr[i].arg.intArg = result;
                result = callBoundedFunction(&boundFunction->func.boundFunctionArr[i], winInfo);
            }
            break;
        case NO_ARGS:
            boundFunction->func.func();
            break;
        case NO_ARGS_RETURN_INT:
            result = boundFunction->func.funcReturnInt();
            break;
        case INT_ARG:
            boundFunction->func.func(arg.intArg);
            break;
        case INT_ARG_RETURN_INT:
            result = boundFunction->func.funcReturnInt(arg.intArg);
            break;
        case INT_INT_ARG:
            boundFunction->func.func(arg.intArr[0], arg.intArr[1]);
            break;
        case INT_INT_ARG_RETURN_INT:
            result = boundFunction->func.funcReturnInt(arg.intArr[0], arg.intArr[1]);
            break;
        case WIN_ARG:
            boundFunction->func.func(arg.voidArg);
            break;
        case WIN_ARG_RETURN_INT:
            result = boundFunction->func.funcReturnInt(arg.voidArg);
            break;
        case WIN_INT_ARG:
            if(winInfo)
                boundFunction->func.func(winInfo, arg.intArg);
            break;
        case WIN_INT_ARG_RETURN_INT:
            if(winInfo)
                result = boundFunction->func.funcReturnInt(winInfo, arg.intArg);
            break;
        case VOID_ARG:
            boundFunction->func.func(arg.voidArg);
            break;
        case VOID_ARG_RETURN_INT:
            result = boundFunction->func.funcReturnInt(arg.voidArg);
            break;
    }
    // preserves magnitude when negateResult is false
    return boundFunction->negateResult ? !result : result;
}

#define _FOR_EACH_CHAIN(CODE...){ int index=0;\
    while(1){CODE;\
            if(chain[index++].endChain) \
                break; \
        } \
    }

void startChain(BoundFunction* boundFunction){
    Binding* chain = boundFunction->func.chainBindings;
    int mask = boundFunction->arg.intArg;
    LOG(LOG_LEVEL_DEBUG, "starting chain; mask:%d\n", mask);
    if(mask)
        grabDevice(getDeviceIDByMask(mask), mask);
    _FOR_EACH_CHAIN(
        if(!chain[index].detail)
        initBinding(&chain[index]);
        grabBinding(&chain[index])
    );
    pushBinding(boundFunction);
}
void endChain(){
    BoundFunction* boundFunction = popActiveBinding();
    Binding* chain = boundFunction->func.chainBindings;
    int mask = boundFunction->arg.intArg;
    if(mask)
        ungrabDevice(getDeviceIDByMask(mask));
    _FOR_EACH_CHAIN(ungrabBinding(&chain[index]));
    LOG(LOG_LEVEL_DEBUG, "ending chain");
}
int getIDOfBindingTarget(Binding* binding){
    return binding->targetID == -1 ?
           isKeyboardMask(binding->mask) ?
           getActiveMasterKeyboardID() : getActiveMasterPointerID() :
           binding->targetID;
}



int doesBindingMatch(Binding* binding, int detail, int mods, int mask, int keyRepeat){
    return (getCurrentMode() == ALL_MODES || getCurrentMode()& binding->mode) && (binding->mask & mask || mask == 0) &&
           (binding->mod == WILDCARD_MODIFIER || binding->mod == mods) &&
           (binding->detail == 0 || binding->detail == detail) && (!keyRepeat || keyRepeat == !binding->noKeyRepeat);
}

static WindowInfo* getWindowToActOn(Binding* binding, WindowInfo* target){
    switch(binding->windowTarget){
        default:
        case DEFAULT:
            return isKeyboardMask(binding->mask) ? getFocusedWindow() : target;
        case FOCUSED_WINDOW:
            return getFocusedWindow();
        case TARGET_WINDOW:
            return target;
    }
}
int checkBindings(int keyCode, int mods, int mask, WindowInfo* winInfo, int keyRepeat){
    mods &= ~IGNORE_MASK;
    LOG(LOG_LEVEL_VERBOSE, "detail: %d mod: %d mask: %d\n", keyCode, mods, mask);
    Binding* chainBinding = getActiveBinding();
    int bindingsTriggered = 0;
    while(chainBinding){
        int i = 0;
        do {
            if(doesBindingMatch(&chainBinding[i], keyCode, mods, mask, keyRepeat)){
                LOG(LOG_LEVEL_TRACE, "Calling chain binding\n");
                int result = callBoundedFunction(&chainBinding[i].boundFunction, getWindowToActOn(&chainBinding[i], winInfo));
                if(chainBinding[i].endChain){
                    LOG(LOG_LEVEL_DEBUG, "chain is ending because exit key was pressed\n");
                    //chain has ended
                    endChain();
                }
                if(!passThrough(result, chainBinding[i].passThrough))
                    return 0;
                bindingsTriggered += 1;
            }
            else if(chainBinding[i].endChain)
                if(!chainBinding[i].noEndOnPassThrough){
                    LOG(LOG_LEVEL_TRACE, "chain is ending because external key was pressed\n");
                    endChain();
                }
                else {
                    LOG(LOG_LEVEL_TRACE, "chain is not ending despite external key was pressed\n");
                }
        } while(!chainBinding[i++].endChain);
        if(chainBinding == getActiveBinding())
            break;
        else chainBinding = getActiveBinding();
    }
    FOR_EACH(Binding*, binding, getDeviceBindings()){
        if(doesBindingMatch(binding, keyCode, mods, mask, keyRepeat)){
            bindingsTriggered += 1;
            LOG(LOG_LEVEL_TRACE, "Calling non-chain binding\n");
            if(!passThrough(callBoundedFunction(&binding->boundFunction, getWindowToActOn(binding, winInfo)), binding->passThrough))
                return 0;
        }
    }
    LOG(LOG_LEVEL_TRACE, "Triggered %d bindings\n", bindingsTriggered);
    return bindingsTriggered;
}

int doesStringMatchRule(Rule* rule, char* str){
    if(!str)
        return 0;
    if(!rule->init)
        initRule(rule);
    if(rule->ruleTarget & LITERAL){
        if(rule->ruleTarget & CASE_SENSITIVE)
            return strcmp(rule->literal, str) == 0;
        else
            return strcasecmp(rule->literal, str) == 0;
    }
    else {
        assert(rule->regexExpression != NULL);
        return !regexec(rule->regexExpression, str, 0, NULL, 0);
    }
}
int doesWindowMatchRule(Rule* rules, WindowInfo* winInfo){
    int match = !(rules->ruleTarget & NEGATE);
    if(rules->filterMatch.type != UNSET){
        if(!callBoundedFunction(&rules->filterMatch, winInfo))
            return 0;
    }
    if(!rules->literal)
        return match;
    if(!winInfo)return !match;
    return  match == ((rules->ruleTarget & CLASS) && doesStringMatchRule(rules, winInfo->className) ||
                      (rules->ruleTarget & RESOURCE) && doesStringMatchRule(rules, winInfo->instanceName) ||
                      (rules->ruleTarget & TITLE) && doesStringMatchRule(rules, winInfo->title) ||
                      (rules->ruleTarget & TYPE) && doesStringMatchRule(rules, winInfo->typeName));
}
int passThrough(int result, PassThrough pass){
    switch(pass){
        case NO_PASSTHROUGH:
            return 0;
        case ALWAYS_PASSTHROUGH:
            return 1;
        case PASSTHROUGH_IF_TRUE:
            return result;
        case PASSTHROUGH_IF_FALSE:
            return !result;
    }
    LOG(LOG_LEVEL_WARN, "invalid pass through option %d\n", pass);
    return 0;
}

int applyRules(ArrayList* head, WindowInfo* winInfo){
    assert(head);
    FOR_EACH(Rule*, rule, head){
        assert(rule);
        if(doesWindowMatchRule(rule, winInfo)){
            int result = callBoundedFunction(&rule->onMatch, winInfo);
            if(!passThrough(result, rule->passThrough))
                return rule->negateResult ? !result : result;
        }
    }
    return 1;
}
ArrayList* getDeviceBindings(){
    static ArrayList deviceBindings;
    return &deviceBindings;
}
void addBindings(Binding* bindings, int num){
    for(int i = 0; i < num; i++)
        addBinding(&bindings[i]);
}
void addBinding(Binding* binding){
    addToList(getDeviceBindings(), binding);
}
/**
 * Convince method for grabBinding() and ungrabBinding()
 * @param binding
 * @param ungrab
 * @return
 */
int _grabUngrabBinding(Binding* binding, int ungrab){
    if(!binding->noGrab){
        if(!ungrab)
            return grabDetail(getIDOfBindingTarget(binding), binding->detail, binding->mod, binding->mask);
        else
            return ungrabDetail(getIDOfBindingTarget(binding), binding->detail, binding->mod, isKeyboardMask(binding->mask));
    }
    else
        return 1;
}
int grabBinding(Binding* binding){
    return _grabUngrabBinding(binding, 0);
}
int ungrabBinding(Binding* binding){
    return _grabUngrabBinding(binding, 1);
}
int getKeyCode(int keysym){
    return XKeysymToKeycode(dpy, keysym);
}
void initBinding(Binding* binding){
    if(binding->targetID == BIND_TO_ALL_MASTERS)
        binding->targetID = XCB_INPUT_DEVICE_ALL_MASTER;
    else if(binding->targetID == BIND_TO_ALL_DEVICES)
        binding->targetID = XCB_INPUT_DEVICE_ALL;
    if(!binding->mask)
        binding->mask = DEFAULT_BINDING_MASKS;
    if(!binding->mode)
        binding->mode = DEFAULT_MODE;
    if(binding->buttonOrKey && isKeyboardMask(binding->mask))
        binding->detail = getKeyCode(binding->buttonOrKey);
    else binding->detail = binding->buttonOrKey;
    assert(binding->detail || !binding->buttonOrKey);
}
char* expandVar(char* str){
    int bSize = strlen(str) + 1;
    char* buffer = malloc(bSize);
    char* tempBuffer = malloc(bSize);
    int foundVar = 0;
    int bCount = 0;
    int varStart;
    for(int i = 0;; i++){
        if(str[i] == '$'){
            foundVar = 1;
            varStart = i + 1;
        }
        else if(foundVar){
            if(!str[i] || !(isalnum(str[i]) || str[i] == '_')){
                foundVar = 0;
                int varSize = i - varStart;
                tempBuffer = realloc(tempBuffer, varSize + 1);
                strncpy(tempBuffer, &str[varStart], varSize);
                tempBuffer[varSize] = 0;
                char* var = getenv(tempBuffer);
                if(!var)
                    LOG(LOG_LEVEL_WARN, "Env var %s is not set\n", tempBuffer);
                else {
                    LOG(LOG_LEVEL_VERBOSE, "replacing %.*s with %s\n", varSize, &str[varStart], var);
                    int newSize = bCount + strlen(var);
                    if(newSize >= bSize){
                        bSize = newSize * 2;
                        buffer = realloc(buffer, bSize);
                    }
                    strcpy(&buffer[bCount], var);
                    bCount = newSize;
                }
            }
        }
        if(!foundVar){
            if(str[i] == '\\')i++;
            if(bCount >= bSize){
                bSize *= 2;
                buffer = realloc(buffer, bSize);
            }
            buffer[bCount++] = str[i];
        }
        if(!str[i])
            break;
    }
    free(tempBuffer);
    return buffer;
}
void initRule(Rule* rule){
    if(rule->ruleTarget & ENV_VAR){
        rule->literal = expandVar(rule->literal);
    }
    if(rule->literal && (rule->ruleTarget & LITERAL) == 0){
        rule->regexExpression = malloc(sizeof(regex_t));
        regcomp(rule->regexExpression, rule->literal,
                (DEFAULT_REGEX_FLAG) | ((rule->ruleTarget & CASE_SENSITIVE) ? 0 : REG_ICASE));
    }
    rule->init = 1;
}
