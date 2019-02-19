/**
 * @file bindings.c
 * \copybrief bindings.h
 */

/// \cond
#include <regex.h>
#include <strings.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
/// \endcond

#include "bindings.h"
#include "xsession.h"
#include "devices.h"
#include "masters.h"
#include "globals.h"
#include "logger.h"


ArrayList* getActiveChains(){
    return &getActiveMaster()->activeChains;
}
/**
 * Push a binding to the active master stack.
 * Should not be called directly
 * @param chain
 */
static void pushBinding(BoundFunction*chain){
    assert(chain);
    addToList(getActiveChains(),chain);
}
/**
 * Pop the last added binding from the master stack
 * @return
 */
static BoundFunction* popActiveBinding(){
    return pop(getActiveChains());
}
Binding* getActiveBinding(){
    return isNotEmpty(getActiveChains())?((BoundFunction*)getLast(getActiveChains()))->func.chainBindings:NULL;
}

int callBoundedFunction(BoundFunction*boundFunction,WindowInfo*winInfo){
///\cond
#define _BF_CASE(TYPE,FUNC_ARG,ARG...)\
    case TYPE: ((void (*)FUNC_ARG)boundFunction->func.func)(ARG);break; \
    case TYPE+1: return ((int (*)FUNC_ARG)boundFunction->func.funcReturnInt)(ARG);
///\endcond

    assert(boundFunction);
    LOG(LOG_LEVEL_ALL,"calling function %d\n",boundFunction->type);

    BoundFunctionArg arg=boundFunction->arg;

    if(boundFunction->dynamic==2)
        arg.intArg=callBoundedFunction(boundFunction->arg.voidArg, winInfo);

    else if(boundFunction->dynamic==1){
        if(winInfo==NULL)
            return 0;
        arg.voidArg=winInfo;
    }

    int result=0;
    switch(boundFunction->type){
        case UNSET:
            LOG(LOG_LEVEL_WARN,"calling unset function; nothing is happening\n");
            break;
        case CHAIN:
            assert(!boundFunction->func.chainBindings->endChain);
            startChain(boundFunction);
            break;
        case CHAIN_AUTO:
            startChain(boundFunction);
            return callBoundedFunction(&boundFunction->func.chainBindings->boundFunction, winInfo);
        case FUNC_AND:
            for(int i=0;i<boundFunction->arg.intArg;i++)
                if(!callBoundedFunction(&boundFunction->func.boundFunctionArr[i],winInfo))
                    return 0;
            return 1;
        case FUNC_BOTH:
            for(int i=0;i<boundFunction->arg.intArg;i++)
                if(callBoundedFunction(&boundFunction->func.boundFunctionArr[i],winInfo))
                    result=1;
            return result;
        case FUNC_OR:
            for(int i=0;i<boundFunction->arg.intArg;i++)
                if(callBoundedFunction(&boundFunction->func.boundFunctionArr[i],winInfo))
                    return 1;
            return 0;
        _BF_CASE(NO_ARGS,(void))
        _BF_CASE(INT_ARG,(int),arg.intArg)
        _BF_CASE(WIN_ARG,(WindowInfo*),arg.voidArg)
        _BF_CASE(WIN_INT_ARG,(WindowInfo*,int),winInfo,arg.intArg)
        _BF_CASE(VOID_ARG,(void*),arg.voidArg)

    }
    return 1;
}
///\cond
#define FOR_EACH_CHAIN(CODE...){ int index=0;\
    while(1){CODE;\
            if(chain[index++].endChain) \
                break; \
        } \
    }
///\endcond
void startChain(BoundFunction *boundFunction){
    Binding*chain=boundFunction->func.chainBindings;
    int mask=boundFunction->arg.intArg;
    LOG(LOG_LEVEL_TRACE,"starting chain. mask:%d\n",mask);
    if(mask)
        grabDevice(getDeviceIDByMask(mask),mask);
    FOR_EACH_CHAIN(
        if(!chain[index].detail)
            initBinding(&chain[index]);
        grabBinding(&chain[index])
    );
    pushBinding(boundFunction);
}
void endChain(){
    BoundFunction *boundFunction=popActiveBinding();
    Binding*chain=boundFunction->func.chainBindings;
    int mask=boundFunction->arg.intArg;
    if(mask)
        ungrabDevice(getDeviceIDByMask(mask));
    FOR_EACH_CHAIN(ungrabBinding(&chain[index]));
}
int getIDOfBindingTarget(Binding*binding){
    return binding->targetID==-1?
            isKeyboardMask(binding->mask)?
                getActiveMasterKeyboardID():getActiveMasterPointerID():
            binding->targetID;
}



int doesBindingMatch(Binding*binding,int detail,int mods,int mask){
    return (binding->mask & mask || mask==0) && (binding->mod == WILDCARD_MODIFIER|| binding->mod==mods) &&
            (binding->detail==0||binding->detail==detail);
}

static WindowInfo*getWindowToActOn(Binding*binding,WindowInfo*target){
    switch(binding->windowTarget){
        default:
        case DEFAULT:
            return isKeyboardMask(binding->mask)?getFocusedWindow():target;
        case FOCUSED_WINDOW:
            return getFocusedWindow();
        case TARGET_WINDOW:
            return target;
    }
}
int checkBindings(int keyCode,int mods,int mask,WindowInfo*winInfo){

    mods&=~IGNORE_MASK;

    LOG(LOG_LEVEL_TRACE,"detail: %d mod: %d mask: %d\n",keyCode,mods,mask);

    Binding* chainBinding=getActiveBinding();
    int bindingsTriggered=0;
    while(chainBinding){
        int i=0;
        do{
            if(doesBindingMatch(&chainBinding[i],keyCode,mods,mask)){
                LOG(LOG_LEVEL_DEBUG,"Calling chain binding\n");
                int result=callBoundedFunction(&chainBinding[i].boundFunction,getWindowToActOn(&chainBinding[i],winInfo));
                if(chainBinding[i].endChain){
                    LOG(LOG_LEVEL_DEBUG,"chain is ending because exit key was pressed\n");
                    //chain has ended
                    endChain();
                }
                if(!passThrough(result,chainBinding[i].passThrough))
                    return 0;
                bindingsTriggered+=1;
            }
            else if(chainBinding[i].endChain)
                if(!chainBinding[i].noEndOnPassThrough){
                    LOG(LOG_LEVEL_TRACE,"chain is ending because external key was pressed\n");
                    endChain();
                }
                else {
                    LOG(LOG_LEVEL_TRACE,"chain is not ending despite external key was pressed\n");
                }
        }while(!chainBinding[i++].endChain);
        if(chainBinding==getActiveBinding())
            break;
        else chainBinding=getActiveBinding();
    }
    FOR_EACH(Binding*binding,getDeviceBindings(),
        if(doesBindingMatch(binding,keyCode,mods,mask)){
            bindingsTriggered+=1;
            LOG(LOG_LEVEL_DEBUG,"Calling non-chain binding\n");
            if(!passThrough(callBoundedFunction(&binding->boundFunction,getWindowToActOn(binding,winInfo)),binding->passThrough))
               return 0;
        }
    )
    LOG(LOG_LEVEL_TRACE,"Triggered %d bindings\n",bindingsTriggered);
    return bindingsTriggered;
}

int doesStringMatchRule(Rule*rule,char*str){
    if(!str)
        return 0;
    if(!rule->init)
        initRule(rule);
    if(rule->ruleTarget & LITERAL){
        if(rule->ruleTarget & CASE_SENSITIVE)
            return strcmp(rule->literal,str)==0;
        else 
            return strcasecmp(rule->literal,str)==0;
    }
    else{
        assert(rule->regexExpression!=NULL);
        return !regexec(rule->regexExpression, str, 0, NULL, 0);
    }
}
int doesWindowMatchRule(Rule *rules,WindowInfo*winInfo){
    if(!rules->literal)
        return 1;
    if(!winInfo)return 0;
    return (rules->ruleTarget & CLASS) && doesStringMatchRule(rules,winInfo->className) ||
        (rules->ruleTarget & RESOURCE) && doesStringMatchRule(rules,winInfo->instanceName) ||
        (rules->ruleTarget & TITLE) && doesStringMatchRule(rules,winInfo->title) ||
        (rules->ruleTarget & TYPE) && doesStringMatchRule(rules,winInfo->typeName);
}
int passThrough(int result,PassThrough pass){
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
    LOG(LOG_LEVEL_WARN,"invalid pass through option %d\n",pass);
    return 0;
}
int applyRule(Rule*rule,WindowInfo*winInfo){
    assert(rule);
    if(doesWindowMatchRule(rule,winInfo))
        return passThrough(callBoundedFunction(&rule->onMatch,winInfo),rule->passThrough);
    return 1;
}

int applyRules(ArrayList* head,WindowInfo*winInfo){
    assert(head);
    FOR_EACH(Rule *rule,head,
        assert(rule);
        if(!applyRule(rule,winInfo))
            return 0;
        )
    return 1;
}
ArrayList*getDeviceBindings(){
    static ArrayList deviceBindings;
    return &deviceBindings;
}
void addBindings(Binding*bindings,int num){
    for(int i=0;i<num;i++)
        addBinding(&bindings[i]);
}
void addBinding(Binding*binding){
    addToList(getDeviceBindings(), binding);
}
/**
 * Convience method for grabBinding() and ungrabBinding()
 * @param binding
 * @param ungrab
 * @return
 */
int _grabUngrabBinding(Binding*binding,int ungrab){
    if(!binding->noGrab){
        if (!ungrab)
            return grabDetail(getIDOfBindingTarget(binding),binding->detail,binding->mod,binding->mask);
        else
            return ungrabDetail(getIDOfBindingTarget(binding), binding->detail,binding->mod, isKeyboardMask(binding->mask));
    }
    else
        return 1;
}
int grabBinding(Binding*binding){
    return _grabUngrabBinding(binding,0);
}
int ungrabBinding(Binding*binding){
    return _grabUngrabBinding(binding,1);
}
int getKeyCode(int keysym){
    return XKeysymToKeycode(dpy, keysym);
}
void initBinding(Binding*binding){
    if(binding->targetID==BIND_TO_ALL_MASTERS)
        binding->targetID=XCB_INPUT_DEVICE_ALL_MASTER;
    else if(binding->targetID==BIND_TO_ALL_DEVICES)
        binding->targetID=XCB_INPUT_DEVICE_ALL;

    if(!binding->mask)
        binding->mask=DEFAULT_BINDING_MASKS;
    if(binding->buttonOrKey && isKeyboardMask(binding->mask))
        binding->detail=getKeyCode(binding->buttonOrKey);
    else binding->detail=binding->buttonOrKey;

    assert(binding->detail||!binding->buttonOrKey);

}
void initRule(Rule*rule){
    if(rule->ruleTarget & ENV_VAR){
        int bSize=strlen(rule->literal)+1;
        char*buffer=malloc(bSize);
        char*tempBuffer=malloc(bSize);
        int foundVar=0;
        int bCount=0;
        int varStart;
        for(int i=0;;i++){
            if(rule->literal[i]=='$'){
                if(i==0||rule->literal[i-1]!='\\'){
                    foundVar=1;
                    varStart=i+1;
                }
            }
            else if(foundVar){
                if(!rule->literal[i]|| !(isalnum(rule->literal[i])||rule->literal[i]=='_')){
                    foundVar=0;
                    int varSize=i-varStart;
                    tempBuffer=realloc(tempBuffer,varSize+1);
                    strncpy(tempBuffer,&rule->literal[varStart],varSize);
                    tempBuffer[varSize]=0;
                    char*var=getenv(tempBuffer);
                    if(!var)
                        LOG(LOG_LEVEL_WARN,"Env var %s is not set\n",tempBuffer);
                    else{
                        int newSize=bCount+strlen(var);
                        if(newSize>=bSize){
                            bSize=newSize*2;
                            buffer=realloc(buffer,bSize);
                        }
                        strcpy(&buffer[bCount],var);
                        bCount=newSize;
                    }
                }
            }
            if(!foundVar){
                if(rule->literal[i]=='\\')i++;
                buffer[bCount++]=rule->literal[i];
            }
            if(!rule->literal[i])break;
        }
        free(tempBuffer);
        rule->literal=buffer;
    }
    if(rule->literal && (rule->ruleTarget & LITERAL) == 0){
        rule->regexExpression=malloc(sizeof(regex_t));
        regcomp(rule->regexExpression,rule->literal,
                (DEFAULT_REGEX_FLAG)|((rule->ruleTarget & CASE_SENSITIVE)?0:REG_ICASE));
    }
    rule->init=1;
}

/// Holds a Node list of rules that will be applied in response to various conditions
static ArrayList eventRules[NUMBER_OF_EVENT_RULES];

ArrayList* getEventRules(int i){
    return &eventRules[i];
}
void removeRule(int i,Rule*rule){
    ArrayList*rules=getEventRules(i);
    int index=indexOf(rules,rule,sizeof(Rule));
    if(index!=-1)
        removeFromList(rules,index);
}
void prependRule(int i,Rule*rule){
    prependToList(getEventRules(i), rule);
}
void appendRule(int i,Rule*rule){
    addToList(getEventRules(i), rule);
}
void clearAllRules(void){
    for(unsigned int i=0;i<NUMBER_OF_EVENT_RULES;i++)
        clearList(getEventRules(i));
}
/*if(chain!=getActiveMaster()->lastBindingTriggered){
        Master *m=getActiveMaster();
        clearWindowCache(m);
        getActiveMaster()->lastBindingTriggered=chain;
    }*/
