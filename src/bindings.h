/**
 * @file bindings.h
 * @brief Provides methods related to key/mouse bindings and window matching rules.
 */
#ifndef BINDINGS_H_
#define BINDINGS_H_

/// \cond
#include <regex.h>
/// \endcond
#include "mywm-util.h"

///Default Regex flag used when creating non literal Rules
#define DEFAULT_REGEX_FLAG REG_EXTENDED

/**Rule will match nothing */
#define MATCH_NONE          0
/**Rule will be interpreted literally as opposed to Regex */
#define LITERAL             1<<0
/**Rule will be will ignore case */
#define CASE_INSENSITIVE    1<<1
/**Rule will match on window type name */
#define TYPE                1<<2
/**Rule will match on window class name */
#define CLASS               1<<3
/**Rule will match on window instance name */
#define RESOURCE            1<<4
/**Rule will match on window title/name */
#define TITLE               1<<5
/**Rule will match on above property of window literally */
#define MATCH_ANY_LITERAL   ((1<<6) -1)
/**Rule will match on above property of window not literally */
#define MATCH_ANY_REGEX     (MATCH_ANY_LITERAL - LITERAL)

/**
 * Determines if and when the control flow should abort processing a series of
 * Rules/Bindings
 * @see Rule
 * @see Bindings
 */
typedef enum{
    NO_PASSTHROUGH=0,      //!< NO_PASSTHROUGH always abort after processing
    ALWAYS_PASSTHROUGH=1,  //!< ALWAYS_PASSTHROUGH never abort
    PASSTHROUGH_IF_TRUE=2, //!< PASSTHROUGH_IF_TRUE abort if the BoundFunction return 0
    PASSTHROUGH_IF_FALSE=3,//!< PASSTHROUGH_IF_FALSE abort if the BoundFunction did not return 0
}PassThrough;
/**
 * All the supported types of Functions that can be bound
*/
typedef enum{
    /**Don't call a function*/
    UNSET=0,
    /**Start a chain binding*/
    CHAIN,
    /**Start a auto chain binding*/
    AUTO_CHAIN,
    /// && for bindings
    FUNC_AND,
    /// || for bindings
    FUNC_OR,
    /**Call a function that takes no arguments*/
    NO_ARGS,
    /**Call a function that takes no arguments*/
    NO_ARGS_RETURN_INT,
    /**Call a function that takes a single int argument*/
    INT_ARG,
    /**Call a function that takes a single int argument*/
    INT_ARGS_RETURN_INT,
    /**Call a function that takes a WindowInfo as an object */
    WIN_ARG,
    /**Call a function that takes a WindowInfo as an object */
    WIN_ARG_RETURN_INT,
    /// Call a function that takes a non-function pointer as an argument
    VOID_ARG,
    /// Call a function that takes a non-function pointer as an argument
    VOID_ARGS_RETURN_INT
    /// Call a function that takes a non-function pointer as an argument
}BindingType;

/**
 * Used to bind a function with arguments to a Rule or Binding
 * such that when the latter is triggered the the bound function
 * is called with arguments
 */
typedef struct bound_function_struct{
    /**Union of supported functions*/
    union{
        /**Function*/
        void (*func)();
        int (*funcReturnInt)();
        /**An array of Bindings terminated with a by the last member having .endChain==1*/
        struct binding_struct *chainBindings;

        struct bound_function_struct *boundFunctionArr;

    } func;
    /**Arguments to pass into the bound function*/
    union{
        /**an int*/
        int intArg;
        /**a pointer*/
        void* voidArg;
        ///func pointer
        void(*compositeFunc);
        ///func pointer
        int(*compositeFuncReturnInt);
    } arg;
    /**The type of Binding which is used to call the right function*/
    BindingType type;
    ///If true, then then the variable passed to callBoundedFuncion() will be pass to func
    /// instead of the arg
    char dynamic;
} BoundFunction;
///used for key/mouse bindings
typedef struct binding_struct{
    /**Modifer to match on*/
    unsigned int mod;
    /**Either Button or KeySym to match**/
    int buttonOrKey;
    /**Function to call on match**/
    BoundFunction boundFunction;
    /**Whether more bindings should be considered*/
    int passThrough;
    /**Whether a device should be grabbed or not*/
    int noGrab;
    ///The mask to grab; if 0 the mask will automatically be determined
    int mask;
    /**The target of the grab;; Defaukt: is ALL_MASTER*/
    int targetID;
    /**If in an array, this property being set to 1 signifies the end*/
    char endChain;
    /**If a binding is pressed that doesn't match the chain, don't end it*/
    char noEndOnPassThrough;

    /**Converted detail of key bindings. Should not be set manually*/
    int detail;
} Binding;

///trigger some function when certain conditions are met
typedef struct{
    /**Literal expression*/
    char* literal;
    /**What the rule should match*/
    int ruleTarget;
    /**Function to be called when rule is matched*/
    BoundFunction onMatch;
    /**If false then subsequent rules won't be checked if this rule matches*/
    int passThrough;
    /**Compiled regex used for matching with non-literal rules*/
    regex_t* regexExpression;
} Rule;


/**
 * Takes 1 or 2 args and creates a BoundFunction
 *
 *
 * @param ... the params to the function, the 1st is the function, 2nd (if present) is
 * the argument to bind to the function
 */
#define BIND(...) BIND_HELPER(__VA_ARGS__, _BIND, BIND2,BIND1)(__VA_ARGS__)

///\cond
#define BIND1(F) _BIND(F,0,\
    _Generic((F), \
            void(*)(void):0,\
            int(*)(void):0,\
            default:1 \
            ))

#define BIND2(F,A) _BIND(F,A,0)
#define BIND_HELPER(_1,_2,_3,NAME,...) NAME

#define _BIND(F,A,D) (BoundFunction){.func=(void (*)()) F, .arg={.voidArg=(void*)A},.dynamic=D, .type=\
   _Generic((F), \
        struct binding_struct:CHAIN, \
        void (*)(void):NO_ARGS, \
        int (*)(void):NO_ARGS_RETURN_INT, \
        void (*)(int): INT_ARG, \
        int (*)(int): INT_ARGS_RETURN_INT, \
        void (*)(WindowInfo*): WIN_ARG, \
        int (*)(WindowInfo*): WIN_ARG_RETURN_INT, \
        void (*)(void*): VOID_ARG, \
        int (*)(void*): VOID_ARGS_RETURN_INT \
)}
///\endcond


/**
 * Used to Create a BoundedFuntion that when triggered will start a chain binding.
 * @param bindings list of Bindings that will make up the chain
 * @see BoundedFuntion
 */
#define CHAIN(bindings...) AUTO_CHAIN_GRAB(DEFAULT_CHAIN_BINDING_GRAB_MASK,0,bindings)
/**
 * when the chain starts, the first bound function will be automatically called
 */
#define AUTO_CHAIN(bindings...) AUTO_CHAIN_GRAB(DEFAULT_CHAIN_BINDING_GRAB_MASK,1,bindings)
/**
 * Used to Create a BoundedFuntion that when triggered will start a chain binding.
 * When the chain is started, the active pointer/keyboard will be grabbed
 * @param GRAB whether or not to grab the active pointer/keyboard when the chain starts
 * @param ... list of Bindings that will make up the chain
 * @see BoundedFuntion
 */
#define CHAIN_GRAB(GRAB, ...) AUTO_CHAIN_GRAB(GRAB,0,__VA_ARGS__)
/**
 *
 * @param GRAB wheter to grab the chain or not
 * @param AUTO when the chain is actiated, auto trigger the first binding
 * @param ... the members of the chain
 */
#define AUTO_CHAIN_GRAB(GRAB,AUTO,...) {.func={.chainBindings=(Binding*)(Binding[]){__VA_ARGS__}},.arg={.intArg=GRAB},.type=AUTO?AUTO_CHAIN:CHAIN}
/**
 * Used to create a Keybinding that will server as the terminator for the chain.
 * When this binding is triggered the chain will end
 */
#define END_CHAIN(...) {__VA_ARGS__,.endChain=1}

/**
 * When called, call the BoundFunctions will be triggered until one of them returns false
 * @param B
 */
#define AND(B...) {.func={.boundFunctionArr=(BoundFunction*)(BoundFunction[]){B}},.arg={.intArg=LEN(((BoundFunction[]){B}))},.type=FUNC_AND}

/**
 * When called, call the BoundFunctions will be triggered until one of them returns true
 * @param B
 */
#define OR(B...) {.func={.boundFunctionArr=(BoundFunction*)(BoundFunction[]){B}},.arg={.intArg=LEN(((BoundFunction[]){B}))},.type=FUNC_OR}



/**
 * Creates a Rule that will match anything such that when applyRules is called func will
 * be called unless another Rule terminates the loop first
 * @param func the function (that takes no args and returns void) to call.
 * @see CREATE_WILDCARD
 */
#define CREATE_DEFAULT_EVENT_RULE(func) CREATE_WILDCARD(BIND(func),.passThrough=1)
/**
 * Creates a Rule that will match anything such that when applyRules is called BoundFunction will
 * be called unless another Rule terminates the loop first
 *
 */
#define CREATE_WILDCARD(...) {NULL,0,__VA_ARGS__}
/**
 * Macro to create a rule
 */
#define CREATE_RULE(expression,target,func) {expression,target,func}
/**
 * Macro to create a rule that will be interpreted literally
 */
#define CREATE_LITERAL_RULE(E,T,F) {E,(T|LITERAL),F}

/**
 * Compiles rules
 * @param R the rule to compile
 */
#define COMPILE_RULE(R) \
     if((R->ruleTarget & LITERAL) == 0){\
        R->regexExpression=malloc(sizeof(regex_t));\
        assert(0==regcomp(R->regexExpression,R->literal, \
                (DEFAULT_REGEX_FLAG)|((R->ruleTarget & CASE_INSENSITIVE)?REG_ICASE:0)));\
    }


///@{Special ids that will be replaced on a call to initBinding()
/// with X11 special ids
#define BIND_TO_ALL_MASTERS 0
#define BIND_TO_ALL_DEVICES 1
///@}
/**
 * Initializes the detail field. And converts are local special IDs
 * to X11 special ids for the targetID
 * @param binding
 */
void initBinding(Binding*binding);
/**
 *
 * @param keysym
 * @return the key code for the given keysym
 */
int getKeyCode(int keysym);
/**
 * Returns either the active keyboard or mouse of if the targetID is -1
 * else the value held by binding->id
 * @param binding
 * @return the id of the device to grab
 */
int getIDOfBindingTarget(Binding*binding);

 /**
  * Comapres Bindings with detail and mods to see if it should be triggered.
  * If bindings->mod ==anyModier, then it will match any modiers.
  * If bindings->detail or keycode is 0 it will match any detail
  * @param binding
  * @param detail either a keycode or a button
  * @param mods modiers
  * @param mask
  * @return Returns true if the binding should be triggered based on the combination of detail and mods
  */
 int doesBindingMatch(Binding*binding,int detail,int mods,int mask);


/**
 * Calls the bound function
 * @param boundFunction - details the function to trigger
 * @param winInfo - arg to pass into to function if the type is winArgs
 * @see BoundFunction
 */
int callBoundedFunction(BoundFunction*boundFunction,WindowInfo*winInfo);


/**
 * Check all bindings of a given type to see if they match the params
 * @param detail either a KeyCode or a mouse button
 * @param mods modifier mask
 * @param bindingType
 * @param winInfo the relevant window
 * @return 1 if a binding was matched that had passThrough == false
 */
int checkBindings(int detail,int mods,int bindingType,WindowInfo*winInfo);


/**
 * Starts a chain binding
 * The chain binding indicated by the boundFunction is add the the stack
 * of active bindings for the active master.
 * The device is grabbed corrosponding to the int arg in the boundFunction
 * All members of the chain are grabbed unless noGrab is true
 * @param boundFunction
 */
void startChain(BoundFunction *boundFunction);
/**
 * Ends the active chain for the active master
 * Undoes all of startChain()
 */
void endChain();

/**
 * Wrapper around grabDetail()
 * @param binding
 * @return 0 iff the grab was successful
 */
int grabBinding(Binding*binding);
/**
 * Wrapper around ungrabDetail()
 * @param binding
 * @return 0 iff the grab was successful
 */
int ungrabBinding(Binding*binding);


/**
 *
 * @param chain the given chain
 * @return the end of the chain
 */
Binding* getEndOfChain(Binding*chain);

/**
 * Returns 1 iff the str matches the regex/literal expression.
 * The only aspect of type that is compared is the literal bit. The other masks
 * have to be checked separately
 * @param rule the rule to match against
 * @param str the str to match
 * @return 1 iff the str could cause the rule to be trigger
 */
int doesStringMatchRule(Rule*rule,char*str);
/**
 *
 * @param rules the rule to match against
 * @param winInfo the window to match
 * @return true if the window caused the rule to be triggered
 */
int doesWindowMatchRule(Rule *rules,WindowInfo*winInfo);
/**
 *
 * @param result the result of callBoundFunction
 * @param pass the pass through value of the rule or binding
 * @return 1 iff the code should proceed or 0 iff it should return immediatly
 */
int passThrough(int result,PassThrough pass);
/**
 * Checks to see a the window matches the rule and applies the rule if it does
 * If this functions returns 0 subsequent rules via applyRules will be skipped
 * @param rule the rule to match against
 * @param winInfo the window to match
 * @return 1 iff the rule matched the window and the function return value
 * indicated a success as defined by the rule
 * @see doesWindowMatchRule
 * @see callBoundedFunction
 */
int applyRule(Rule*rule,WindowInfo*winInfo);

/**
 * Iterates over head and applies the stored rules in order stopping when a rule matches
 * and its passThrough value is false
 * @param head
 * @param winInfo
 * @return 1 if no rule was matched that had passThrough == false
 */
int applyRules(Node* head,WindowInfo*winInfo);

/**
 * Adds a binding to the head of deviceBindings
 * @param binding
 */
void addBinding(Binding*binding);

/**
 * @return the last node added to the activeChain stack
 */
Node* getActiveBindingNode();
/**
 * @return the last Chain added the active chain stack
 */
Binding* getActiveBinding();


#endif