/**
 * @file bindings.h
 * @brief Provides methods related to key/mouse bindings and window matching rules.
 */
#ifndef BINDINGS_H_
#define BINDINGS_H_


#include <regex.h>


#include "mywm-structs.h"

///Default Regex flag used when creating non literal Rules
#define DEFAULT_REGEX_FLAG REG_EXTENDED

/**Rule will match nothing */
#define MATCH_NONE          0
/**Rule will be interpreted literally as opposed to Regex */
#define LITERAL             1<<0
/**Rule will be will ignore case */
#define CASE_SENSITIVE    1<<1
/**Rule will match on window type name */
#define TYPE                1<<2
/**Rule will match on window class name */
#define CLASS               1<<3
/**Rule will match on window instance name */
#define RESOURCE            1<<4
/**Rule will match on window title/name */
#define TITLE               1<<5
/**Rule will match on above property of window literally */
#define MATCH_ANY_LITERAL   (TITLE|RESOURCE|CLASS|TYPE|LITERAL)
/**Rule will match on above property of window not literally */
#define MATCH_ANY_REGEX     (MATCH_ANY_LITERAL ^ LITERAL)
/**Rule subsitute env variables for designated strings (strings starting with '$')*/
#define ENV_VAR             1<<6
/**Negates the match*/
#define NEGATE             1<<7

/**
 * Determines if and when the control flow should abort processing a series of
 * Rules/Bindings
 * @see Rule
 * @see Bindings
 */
typedef enum {
    NO_PASSTHROUGH = 1,    //!< NO_PASSTHROUGH always abort after processing
    ALWAYS_PASSTHROUGH = 0, //!< ALWAYS_PASSTHROUGH never abort
    PASSTHROUGH_IF_TRUE = 2, //!< PASSTHROUGH_IF_TRUE abort if the BoundFunction return 0
    PASSTHROUGH_IF_FALSE = 3, //!< PASSTHROUGH_IF_FALSE abort if the BoundFunction did not return 0
} PassThrough;
/**
 * All the supported types of Functions that can be bound
*/
typedef enum {
    /**Don't call a function*/
    UNSET = 0,
    /**Start a chain binding*/
    CHAIN,
    /**Start a auto chain binding*/
    CHAIN_AUTO,

    /// & for bindings
    FUNC_BOTH,
    /// && for bindings
    FUNC_AND,
    /// || for bindings
    FUNC_OR,
    /// pipe(|) for bindings
    FUNC_PIPE,
    /**Call a function that takes no arguments*/
    NO_ARGS,
    /**Call a function that takes no arguments*/
    NO_ARGS_RETURN_INT,
    /**Call a function that takes a single int argument*/
    INT_ARG,
    /**Call a function that takes a single int argument*/
    INT_ARG_RETURN_INT,
    /**Call a function that takes a WindowInfo as an object */
    WIN_ARG,
    /**Call a function that takes a WindowInfo as an object */
    WIN_ARG_RETURN_INT,
    /**Call a function that takes a WindowInfo and an int as a parameter */
    WIN_INT_ARG,
    /**Call a function that takes a WindowInfo and an int as a parameter and returns an int */
    WIN_INT_ARG_RETURN_INT,
    /// Call a function that takes a non-function pointer as an argument
    VOID_ARG,
    /// Call a function that takes a non-function pointer as an argument
    VOID_ARG_RETURN_INT
} BindingType;

/**
 *Union holding the argument to a bounded function.
 */
typedef union {
    /**an int*/
    long longArg;
    /**an int*/
    int intArg;
    /**a pointer*/
    void* voidArg;
} BoundFunctionArg;

/**
 * Details which window to act on for Bindings
 */
typedef enum {
    /// Use the focused window for key bindings and the target/event window for mouse bindings
    DEFAULT = 0,
    /// Always use the focused window
    FOCUSED_WINDOW,
    /// Always use the target window (the target of the active master if set of the event window)
    TARGET_WINDOW
} WindowParamType;
/**
 * Used to bind a function with arguments to a Rule or Binding
 * such that when the latter is triggered the the bound function
 * is called with arguments
 */
typedef struct bound_function_struct {
    /**Union of supported functions*/
    union {
        /**Function*/
        void (*func)();
        int (*funcReturnInt)();
        /**An array of Bindings terminated with a by the last member having .endChain==1*/
        struct binding_struct* chainBindings;
        ///Used with and/or bindings
        struct bound_function_struct* boundFunctionArr;

    } func;
    /**Arguments to pass into the bound function*/
    BoundFunctionArg arg;
    /**The type of Binding which is used to call the right function*/
    BindingType type;
    ///If true, then then the variable passed to callBoundedFuncion() will be pass to func
    /// instead of the arg
    char dynamic;
    /// negate the result of func
    char negateResult;
} BoundFunction;
///used for key/mouse bindings
typedef struct binding_struct {
    /**Modifer to match on*/
    unsigned int mod;
    /**Either Button or KeySym to match**/
    int buttonOrKey;
    /**Function to call on match**/
    BoundFunction boundFunction;
    /**Whether more bindings should be considered*/
    PassThrough passThrough;
    /**Whether a device should be grabbed or not*/
    int noGrab;
    ///The mask to grab; if 0 the mask will automatically be determined
    int mask;
    /**The target of the grab;; Default: is ALL_MASTER*/
    int targetID;
    /**If in an array, this property being set to 1 signifies the end*/
    char endChain;
    /**If a binding is pressed that doesn't match the chain, don't end it*/
    char noEndOnPassThrough;
    /// Details which window to pass to the boundFunction
    WindowParamType windowTarget;

    /**Converted detail of key bindings. Should not be set manually*/
    int detail;
} Binding;

///trigger some function when certain conditions are met
typedef struct {
    /**Literal expression*/
    char* literal;
    /**What the rule should match*/
    int ruleTarget;
    /**Function to be called when rule is matched*/
    BoundFunction onMatch;
    /**match will only succeed if filterMatch is not set or returns true*/
    BoundFunction filterMatch;
    /**If false then subsequent rules won't be checked if this rule matches*/
    int passThrough;
    /// When applying rules, the return value will be negated if this field is set. A return value of 0, indicates the calling method should (cleanup and) abort
    int negateResult;
    /**Compiled regex used for matching with non-literal rules*/
    regex_t* regexExpression;
    /** boolean indicating whether the rule has been initilized or not
     *  @see initRule
    */
    char init;
} Rule;


/**
 * Takes 1 or 2 args and creates a BoundFunction
 *
 *
 * @param ... the params to the function, the 1st is the function, 2nd (if present) is
 * the argument to bind to the function
 */
#define BIND(...) _BIND_HELPER(__VA_ARGS__, _BIND, _BIND2,_BIND1)(__VA_ARGS__,0)

/**
 * @copybrief BIND
 *
 * Macro to create a BoundFunction with its result negate
 *
 * @see BIND
 * @see BoundFunction:negateResult
 *
 */
#define NBIND(...) _BIND_HELPER(__VA_ARGS__, _NBIND, _BIND2,_BIND1)(__VA_ARGS__,1)

#define _BIND_HELPER(_1,_2,_3,NAME,...) NAME

#define _BIND_TYPES(F)\
    _Generic((F), \
            void(*)(void):0,\
            int(*)(void):0,\
            void(*)(int):0,\
            int(*)(int):0,\
            default:1 \
            )

#define _BIND1(F,N) _BIND(F,0,_BIND_TYPES(F),N)

#define _BIND2(F,A,N) _BIND(F,A,0,N)

#define _BIND(F,A,D,N){.func=(void (*)()) F, .arg={.longArg=(long)A},.dynamic=D, .negateResult=N,.type=\
   _Generic((F), \
        void (*)(void):NO_ARGS, \
        int (*)(void):NO_ARGS_RETURN_INT, \
        void (*)(int): INT_ARG, \
        int (*)(int): INT_ARG_RETURN_INT, \
        void (*)(WindowInfo*): WIN_ARG, \
        int (*)(WindowInfo*): WIN_ARG_RETURN_INT, \
        void (*)(WindowInfo*,int): WIN_INT_ARG, \
        int (*)(WindowInfo*, int): WIN_INT_ARG_RETURN_INT, \
        void (*)(Layout*): VOID_ARG, \
        void (*)(Rule*): VOID_ARG, \
        void (*)(char*): VOID_ARG, \
        void (*)(void*): VOID_ARG, \
        int (*)(Layout*): VOID_ARG_RETURN_INT, \
        int (*)(Rule*): VOID_ARG_RETURN_INT, \
        int (*)(char*): VOID_ARG_RETURN_INT, \
        int (*)(void*): VOID_ARG_RETURN_INT \
)}



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
#define AUTO_CHAIN_GRAB(GRAB,AUTO,...){.func={.chainBindings=(Binding*)(Binding[]){__VA_ARGS__}},.arg={.intArg=GRAB},.type=AUTO?CHAIN_AUTO:CHAIN}
/**
 * Used to create a Keybinding that will server as the terminator for the chain.
 * When this binding is triggered the chain will end
 */
#define END_CHAIN(...){__VA_ARGS__,.endChain=1}

/**
 * When called, call the BoundFunctions will be triggered until one of them returns false
 * @param B
 */
#define AND(B...) _UNION(FUNC_AND,B)

/**
 * When called, call the BoundFunctions will be triggered until one of them returns false
 * @param B
 */
#define BOTH(B...) _UNION(FUNC_BOTH,B)

/**
 * When called, call the BoundFunctions will be triggered until one of them returns true
 * @param B
 */
#define OR(B...) _UNION(FUNC_OR,B)

/**
 * When called, call the BoundFunctions in order and pass the output as input to the next
 * @param B
 */
#define PIPE(B...) _UNION(FUNC_PIPE,B)

/// Helper macro for AND, OR and BOTH
#define _UNION(TYPE,B...){.func={.boundFunctionArr=(BoundFunction*)(BoundFunction[]){B}},.arg={.intArg=LEN(((BoundFunction[]){B}))},.type=TYPE}



/**
 * Creates a Rule that will match anything such that when applyRules is called func will
 * be called unless another Rule terminates the loop first
 * @param func the function (that takes no args and returns void) to call.
 * @see CREATE_WILDCARD
 */
#define CREATE_DEFAULT_EVENT_RULE(func) CREATE_WILDCARD(BIND(func),.passThrough=ALWAYS_PASSTHROUGH)
/**
 * Creates a Rule that will match anything such that when applyRules is called BoundFunction will
 * be called unless another Rule terminates the loop first
 *
 */
#define CREATE_WILDCARD(...){NULL,0,__VA_ARGS__}



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
void initBinding(Binding* binding);
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
int getIDOfBindingTarget(Binding* binding);

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
int doesBindingMatch(Binding* binding, int detail, int mods, int mask);


/**
 * Calls the bound function
 * @param boundFunction - details the function to trigger
 * @param winInfo - arg to pass into to function if the type is winArgs
 * @see BoundFunction
 */
int callBoundedFunction(BoundFunction* boundFunction, WindowInfo* winInfo);


/**
 * Check all bindings of a given type to see if they match the params
 * @param detail either a KeyCode or a mouse button
 * @param mods modifier mask
 * @param bindingType
 * @param winInfo the relevant window
 * @return 1 if a binding was matched that had passThrough == false
 */
int checkBindings(int detail, int mods, int bindingType, WindowInfo* winInfo);


/**
 * Starts a chain binding
 * The chain binding indicated by the boundFunction is add the the stack
 * of active bindings for the active master.
 * The device is grabbed corrosponding to the int arg in the boundFunction
 * All members of the chain are grabbed unless noGrab is true
 * @param boundFunction
 */
void startChain(BoundFunction* boundFunction);
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
int grabBinding(Binding* binding);
/**
 * Wrapper around ungrabDetail()
 * @param binding
 * @return 0 iff the grab was successful
 */
int ungrabBinding(Binding* binding);


/**
 *
 * @param chain the given chain
 * @return the end of the chain
 */
Binding* getEndOfChain(Binding* chain);

/**
 * Returns 1 iff the str matches the regex/literal expression.
 * The only aspect of type that is compared is the literal bit. The other masks
 * have to be checked separately
 * @param rule the rule to match against
 * @param str the str to match
 * @return 1 iff the str could cause the rule to be trigger
 */
int doesStringMatchRule(Rule* rule, char* str);
/**
 *
 * @param rules the rule to match against
 * @param winInfo the window to match
 * @return true if the window caused the rule to be triggered
 */
int doesWindowMatchRule(Rule* rules, WindowInfo* winInfo);
/**
 *
 * @param result the result of callBoundFunction
 * @param pass the pass through value of the rule or binding
 * @return 1 iff the code should proceed or 0 iff it should return immediatly
 */
int passThrough(int result, PassThrough pass);

/**
 * Iterates over head and applies the given rules in order stopping when a rule matches
 * and its passThrough value is false
 * @param head a list of Rules
 * @param winInfo
 * @return 1 if no rule was matched that had passThrough == false or the result (perhaps negated) of the boundFunction of the last rule to match the given window
 * @see doesWindowMatchRule
 * @see callBoundedFunction
 */
int applyRules(ArrayList* head, WindowInfo* winInfo);

/**
 * Adds a binding to the head of deviceBindings
 * @param binding
 */
void addBinding(Binding* binding);
/**
 * Adds num bindings to the head of deviceBindings
 * @param bindings array of bindings to add
 * @param num number of bindings to add
 * @see addBinding
 */
void addBindings(Binding* bindings, int num);

/**
 * @return the last Chain added the active chain stack
 */
Binding* getActiveBinding();
/**
 * Returns the list of chains the active master is currently triggering
 */
ArrayList* getActiveChains();

/**
 * Returns a Node list of all regisitered bidings
 * @see deviceBindings;
*/

ArrayList* getDeviceBindings();

/**
 * Initilize a rule
 * @param rule
 */
void initRule(Rule* rule);


/// @return true
static inline int returnTrue(void){
    return 1;
}
/// @return false
static inline int returnFalse(void){
    return 0;
}
#endif
