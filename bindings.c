
#include <regex.h>
#include <strings.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "bindings.h"
#include "devices.h"
#include "mywm-structs.h"
#include "defaults.h"
#include "mywm-util.h"
#include "logger.h"

void grabBindings(Binding*chain){
    Binding*binding=getEndOfChain(chain);
    if(binding->noGrab==NO_GRAB || binding->noGrab==GRAB_AUTO)
        return;
    //assert(binding->noGrab!=GRAB_AUTO);

    switch(binding->noGrab){
        case GRAB_KEYBOARD:
            grabActiveKeyboard();
            break;
        case GRAB_POINTER:
            grabActivePointer();
            break;
        case GRAB_POINTER_KEYBOARD:
            grabActiveKeyboard();
            grabActivePointer();
            break;
        case GRAB_KEY:
        case GRAB_BUTTON:
            for(int i=0;&chain[i]!=binding;i++){
                int mouse=binding->noGrab!=GRAB_KEY;
                grabActiveDetail(
                        mouse?chain[i].detail:chain[i].keyCode,
                        chain[i].mod,mouse
                );
        }
    }
}

void callBoundedFunction(BoundFunction*boundFunction,WindowInfo*info){
    assert(boundFunction);
    LOG(LOG_LEVEL_TRACE,"calling function %d\n",boundFunction->type);
    assert(boundFunction->type<NUMBER_OF_BINDING_OPTIONS);


    switch(boundFunction->type){
        case UNSET:
            break;
        case CHAIN:
            assert(!boundFunction->func.chainBindings->endChain);
            grabBindings(boundFunction->func.chainBindings);
            pushBinding(boundFunction->func.chainBindings);

            break;
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
        default:
            LOG(LOG_LEVEL_ERROR,"type is %d not valid\n",boundFunction->type);
            assert(0);
    }
}

Binding* getEndOfChain(Binding*chain){
    int end=-1;
    while(!chain[++end].endChain){}
    return &chain[end];
}

int doesBindingMatch(Binding*binding,int notMouse,int detail,int mods){
    return (binding->mod == anyModier|| binding->mod==mods) &&
            (notMouse && (binding->keyCode==0||binding->keyCode==detail) ||
            !notMouse && (binding->detail==0||binding->detail==detail));
}
int checkBindings(int keyCode,int mods,int bindingIndex,WindowInfo*info){
    int notMouse=bindingIndex<2;

    mods&=~IGNORE_MASK;

    LOG(LOG_LEVEL_TRACE,"key detected code: %d mod: %d mouse: %d index:%d len %d\n",keyCode,mods,!notMouse,bindingIndex,deviceBindingLengths[bindingIndex]);

    Binding* chainBinding=getActiveBinding();
    int bindingTriggered=0;
    while(chainBinding){
        int i;
        for(i=0;;i++){
            if(doesBindingMatch(&chainBinding[i],notMouse,keyCode,mods)){
                callBoundedFunction(&chainBinding[i].boundFunction,info);
                if(chainBinding[i].endChain){
                    //chain has ended
                    popActiveBinding();
                }
                if(!chainBinding[i].passThrough)
                    return 0;
                bindingTriggered+=1;
                if(chainBinding[i].endChain){
                    break;
                }
            }
            if(chainBinding[i].endChain){
                if(!chainBinding[i].noEndOnPassThrough)
                    popActiveBinding();

                break;
            }
        }
        if(chainBinding==getActiveBinding())
            break;
        else chainBinding=getActiveBinding();
    }
    for (int i = 0; i < deviceBindingLengths[bindingIndex]; i++){

        if(doesBindingMatch(&deviceBindings[bindingIndex][i],notMouse,keyCode,mods)){
            bindingTriggered+=1;
            callBoundedFunction(&deviceBindings[bindingIndex][i].boundFunction,info);
            if(!deviceBindings[bindingIndex][i].passThrough)
                return 0;
        }
    }
    LOG(LOG_LEVEL_TRACE,"Triggered %d bidings\n",bindingTriggered);
    return bindingTriggered;
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
    if(!rules->literal)
        return 1;
    if(!info)return 0;
    return (rules->ruleTarget & CLASS) && doesStringMatchRule(rules,info->className) ||
        (rules->ruleTarget & RESOURCE) && doesStringMatchRule(rules,info->instanceName) ||
        (rules->ruleTarget & TITLE) && doesStringMatchRule(rules,info->title) ||
        (rules->ruleTarget & TYPE) && doesStringMatchRule(rules,info->typeName);
}

int applyRule(Rules*rule,WindowInfo*info){
    if(doesWindowMatchRule(rule,info)){
        callBoundedFunction(&rule->onMatch,info);
        return 1;
    }
    return 0;
}

int applyEventRules(Node* head,WindowInfo*info){
    assert(head);
    if(!head)return 1;
    FOR_EACH(head,
        Rules *rule=getValue(head);
        if(applyRule(rule,info))
            if(!rule->passThrough)
                return 0;
        )
    return 1;
}

/*if(chain!=getActiveMaster()->lastBindingTriggered){
        Master *m=getActiveMaster();
        clearWindowCache(m);
        getActiveMaster()->lastBindingTriggered=chain;
    }*/
