/*
 * bindings.c
 *
 *  Created on: Jul 1, 2018
 *      Author: arthur
 */

#include <regex.h>
#include <strings.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "bindings.h"
#include "wmfunctions.h"
#include "mywm-structs.h"
#include "mywm-util.h"
#include "wmfunctions.h"

Binding*bindings[4];
int bindingLengths[4];
const int bindingMasks[4]={XI_KeyPressMask,XI_KeyReleaseMask,XI_ButtonPressMask,XI_ButtonReleaseMask};


void callBoundedFunction(BoundFunction*boundFunction,WindowInfo*info){
    LOG(LOG_LEVEL_TRACE,"calling function %d\n",boundFunction->type);
    assert(boundFunction->type!=UNSET);

    switch(boundFunction->type){
        case NO_ARGS:
            boundFunction->func.funcNoArg();
            break;
        case INT_ARG:
            boundFunction->func.funcInt(boundFunction->arg.intArg);
            break;
        case WIN_ARG:
            boundFunction->func.funcVoidArg(info);
            break;
        case VOID_ARG:
            boundFunction->func.funcVoidArg(boundFunction->arg.voidArg);
            break;
        case CHAIN:
            getActiveMaster()->activeChainBinding=boundFunction->func.chainBindings;
            if(boundFunction->arg.intArg)
                grabDevice(getActiveMaster()->id,CHAIN_BINDING_GRAB_MASKS);
            break;
        default:
            LOG(LOG_LEVEL_ERROR,"type is %d not valid\n",boundFunction->type);
            assert(0);

    }
}

int triggerBinding(Binding*key,WindowInfo*info){
    LOG(LOG_LEVEL_DEBUG,"Binding matched, triggering function\n");
    if(key!=getActiveMaster()->lastBindingTriggered){
        Master *m=getActiveMaster();
        clearWindowCache(m);
        getActiveMaster()->lastBindingTriggered=key;
    }
    callBoundedFunction(&key->boundFunction,info);
    return key->passThrough;
}
void checkBindings(int keyCode,int mods,int bindingType,WindowInfo*info){

    int bindingIndex=bindingType-XI_KeyPress;
    int notMouse=bindingType<XI_ButtonPress;

    mods&=~IGNORE_MASK;

    LOG(LOG_LEVEL_TRACE,"key detected code: %d mod: %d mouse: %d index:%d len%d\n",keyCode,mods,!notMouse,bindingIndex,bindingLengths[bindingIndex]);
#define CHECK_KEY(K) if( (K.mod == XIAnyModifier|| K.mod==mods) && \
    (notMouse && (K.keyCode==XIAnyKeycode||K.keyCode==keyCode) || \
    !notMouse && (K.detail==XIAnyButton||K.detail==keyCode)))\
    if(!triggerBinding(&K,info)) \
        return;


    Master* m=getActiveMaster();
    Binding* chainBinding=m->activeChainBinding;
    if(chainBinding){
        for(int i=0;chainBinding[i].boundFunction.type;i++){
            CHECK_KEY(chainBinding[i])
        }
    }
    LOG(LOG_LEVEL_ALL,"looping through bindings\n");
    for (int i = 0; i < bindingLengths[bindingIndex]; i++){
        CHECK_KEY(bindings[bindingIndex][i])
    }
}

int doesStringMatchRule(Rules*rule,char*str){
    if(!str)
        return 0;
    if(rule->ruleTarget & LITERAL){
        if(rule->ruleTarget & CASE_INSENSITIVE)
            return strcasecmp(rule->literal,str)==0;
        else return strcmp(rule->literal,str)==0;
    }
    else{
        assert(rule->regexExpression!=NULL);
        return !regexec(rule->regexExpression, str, 0, NULL, 0);
    }
}
int doesWindowMatchRule(Rules *rules,WindowInfo*info){
    if(rules->ruleTarget==MATCH_NONE)
        return 1;
    if(!info)return 0;
    return (rules->ruleTarget & CLASS) && doesStringMatchRule(rules,info->className) ||
        (rules->ruleTarget & RESOURCE) && doesStringMatchRule(rules,info->instanceName) ||
        (rules->ruleTarget & TITLE) && doesStringMatchRule(rules,info->title) ||
        (rules->ruleTarget & TYPE) && doesStringMatchRule(rules,info->typeName);
}

int applyRule(Rules*rule,WindowInfo*info){
    if(doesWindowMatchRule(rule,info)){
        if(rule->ruleTarget != CHAIN)
            callBoundedFunction(&rule->onMatch,info);
        return 1;
    }

    return 0;
}

int applyEventRules(int type,WindowInfo*info){
    assert(type>=0);
    assert(type<NUMBER_OF_EVENT_RULES);
    Node*head=eventRules[type];
    if(!head)
        return 1;
    FOR_EACH(head,
        Rules *rule=getValue(head);
        if(applyRule(rule,info))
            if(!rule->passThrough)
                return 0;
        )
    return 1;
}


void grabButtons(Binding*binding,int numKeys,int mask){
    for (int i = 0; i < numKeys; i++)
        if(!binding[i].noGrab)
            grabButton(XIAllMasterDevices,binding[i].detail,
                binding[i].mod,mask);
}
void grabKeys(Binding*binding,int numKeys,int mask){
    for (int i = 0; i < numKeys; i++){
        binding[i].keyCode=XKeysymToKeycode(dpy, binding[i].detail);
        if(!binding[i].noGrab)
            grabKey(XIAllMasterDevices,binding[i].keyCode,binding[i].mod,mask);
    }
}
