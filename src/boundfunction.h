/**
 * @file boundfunction.h
 * @brief FunctionWrapper and Rules
 */
#ifndef MPX_BOUND_FUNCTION_H
#define MPX_BOUND_FUNCTION_H

#include <assert.h>
#include <stdint.h>
#include <sys/types.h>

#include <functional>
#include <memory>
#include <type_traits>

#include "globals.h"
#include "masters.h"
#include "mywm-structs.h"
#include "windows.h"

/**
 * Creates a BoundFunction with a name based on F
 *
 * @param F the function to call
 *
 */
#define DEFAULT_EVENT(F){F, DEFAULT_EVENT_NAME(F), PASSTHROUGH_IF_TRUE}

/**
 * Macro to get the default name for a BoundFunction backed by F
 * @param F the function to call
 *
 */
#define DEFAULT_EVENT_NAME(F) "_" #F
/**
 * Creates a BoundFunction with a name based on F
 *
 * @param F the function to call
 *
 * @return
 */
#define USER_EVENT(F){F, "_" #F,PASSTHROUGH_IF_TRUE}
/**
 * Like DEFAULT_EVENT but the passThrough is p
 * @param F
 * @param P
 *
 * @return
 */
#define PASSTHROUGH_EVENT(F,P){F, "_" #F,P}
/**
 *
 * @return a suitable BoundFunction name based on the name of the calling method
 */
#define FUNC_NAME (std::string("_")+std::string(__FUNCTION__).substr(3))

/**
 * Determines if and when the control flow should abort processing a series of
 * BoundFunction/Bindings
 */
enum PassThrough {
    /// ALWAYS_PASSTHROUGH never abort
    ALWAYS_PASSTHROUGH = 0,
    /// PASSTHROUGH_IF_TRUE abort if the BoundFunction return 0
    PASSTHROUGH_IF_TRUE,
    /// PASSTHROUGH_IF_FALSE abort if the BoundFunction did not return 0
    PASSTHROUGH_IF_FALSE,
    /// NO_PASSTHROUGH always abort after processing
    NO_PASSTHROUGH,
} ;
/**
 * Helper method to determine if the control flow should proceed like normal or abort
 *
 * @param passThrough
 * @param result the literal result to compare against
 *
 * @return 0 iff the caller should abort
 */
static inline bool shouldPassThrough(PassThrough passThrough, int result) {
    switch(passThrough) {
        default:
        case NO_PASSTHROUGH:
            return 0;
        case ALWAYS_PASSTHROUGH:
            return 1;
        case PASSTHROUGH_IF_TRUE:
            return result;
        case PASSTHROUGH_IF_FALSE:
            return !result;
    }
}
/**
 * Arguments used to pass into boundFunction
 */
struct BoundFunctionArg {
    /// @{ Fields that can be passed to a compitable BoundFunction
    /// If the underlying function can't accept a subset of fields, it won't be called and 0 will be returned
    /// NULL means unset but 0 is a valid value of integer
    WindowInfo* winInfo = NULL;
    Master* master = getActiveMaster();
    Workspace* workspace = NULL;
    Monitor* monitor = NULL;
    uint32_t integer = winInfo ? winInfo->getID() : workspace ? workspace->getID() : 0;
    std::string* string = NULL;
    /// @}
};

/**
 * Type of BoundFunction
 */
enum BoundFunctionType {
    VOID_TYPE = 0,
    STRING_TYPE = 1,
    OTHER_TYPE = 2
};

/**
 * Holds a generic function
 */
struct GenericFunctionWrapper {
    /// @{
    /// Used to distinguished different types of supported functions
    template<typename V>
    using enable_if_winInfo = typename std::enable_if_t<std::is_same<V, WindowInfo*>::value, int>;
    template<typename V>
    using enable_if_win = typename std::enable_if_t<std::is_convertible<V, WindowID>::value, int>;
    template<typename V>
    using enable_if_master = typename std::enable_if_t<std::is_same<V, Master*>::value, int>;
    template<typename V>
    using enable_if_workspace = typename std::enable_if_t<std::is_same<V, Workspace*>::value, int>;
    template<typename V>
    using enable_if_monitor = typename std::enable_if_t<std::is_same<V, Monitor*>::value, int>;
    template<typename V>
    using enable_if_integer = typename std::enable_if_t<std::is_convertible<V, int>::value, int>;
    template<typename V>
    using enable_if_string = typename std::enable_if_t<std::is_same<V, std::string*>::value, int>;
    /// @}

    /// describes the argument this function can process
    const BoundFunctionType type;

    /**
     * @param type describes the argument this function can process
     */
    GenericFunctionWrapper(BoundFunctionType type = VOID_TYPE): type(type) {};
    /**
     * Calls the stored function
     *
     * @param arg
     * @return the result of the function
     */
    virtual int call(const BoundFunctionArg& arg)const = 0;
};
#define _RETURN_VALUE_ALWAYS(X) return func(arg.X)
#define _RETURN_VALUE(X)  return arg.X ? func(arg.X) : 0
#define _RETURN_IF_CALLED(X) \
        if((bool)arg.X) func(arg.X); \
        return arg.X ? 1 : 0;
#define _RETURN_TRUE(X) func(arg.X); return 1;

#define _BFCALL(X, EXPR) \
    template<typename V = P> \
    _CAT(enable_if_, X)<V> _call(const BoundFunctionArg& arg) const { \
        EXPR(X) ;\
    }

/**
 * Wrapper for a function that takes a single parameter and returns void
 *
 *
 * @tparam R
 * @tparam P
 */
template <typename R, typename P = void>
struct FunctionWrapper: GenericFunctionWrapper {
    static_assert(std::is_convertible<R, int>::value);
    static_assert(std::is_same<P, std::string*>::value || std::is_convertible<P, int>::value ||
        std::is_base_of<WMStruct, std::remove_pointer_t<P>>::value);
    /// generic function
    std::function<R(P)>func;
    /**
     * @param func the function to call
     */
    FunctionWrapper(std::function<R(P)>func): GenericFunctionWrapper(std::is_same<P, std::string*>::value ? STRING_TYPE :
            OTHER_TYPE), func(func) {}

    /**
     * Needed to compile and support void arguments
     * @param func
     */
    FunctionWrapper(std::function<R()> func): func(func) {}
    /**
     * @param func the function to call
     */
    FunctionWrapper(R(*func)(P)): func(func) {}

    int call(const BoundFunctionArg& arg)const override {
        return _call(arg);
    }

    _BFCALL(winInfo, _RETURN_VALUE);
    _BFCALL(master, _RETURN_VALUE);
    _BFCALL(monitor, _RETURN_VALUE);
    _BFCALL(workspace, _RETURN_VALUE);
    _BFCALL(integer, _RETURN_VALUE_ALWAYS);
    _BFCALL(string, _RETURN_VALUE);
};
/**
 * Wrapper for a function that takes a single parameter and returns void
 *
 * @tparam P WindowInfo, Master, Monitor, or Workspace pointer
 */
template <typename P>
struct FunctionWrapper<void, P>: GenericFunctionWrapper {
    /// generic function holder
    std::function<void(P)>func;
    /**
     * @param func
     */
    FunctionWrapper(std::function<void(P)>func): GenericFunctionWrapper(std::is_same<P, std::string*>::value ? STRING_TYPE :
            OTHER_TYPE), func(func) {}

    int call(const BoundFunctionArg& arg)const override {
        return _call(arg);
    }

    _BFCALL(winInfo, _RETURN_IF_CALLED);
    _BFCALL(master, _RETURN_IF_CALLED);
    _BFCALL(monitor, _RETURN_IF_CALLED);
    _BFCALL(workspace, _RETURN_IF_CALLED);
    _BFCALL(integer, _RETURN_TRUE);
    _BFCALL(string, _RETURN_IF_CALLED);
};
/**
 * Wrapper for a function that takes no parameters and returns an int (or something that can be converted to an int)
 */
template <typename R>
struct FunctionWrapper<R, void>: GenericFunctionWrapper {
    static_assert(std::is_convertible<R, int>::value);
    /// generic function holder
    std::function<R()>func;
    /**
     * @param func the function to call
     */
    FunctionWrapper(std::function<R()>func): func(func) {}
    int call(const BoundFunctionArg& arg __attribute__((unused))) const override {
        return func();
    }
};
/**
 * Wrapper for a function that takes no parameters and returns void
 */
template <>
struct FunctionWrapper<void, void>: GenericFunctionWrapper {
    /// generic function holder
    std::function<void()>func;
    /**
     * @param func the function to call
     */
    FunctionWrapper(std::function<void()>func): func(func) {}
    int call(const BoundFunctionArg& arg __attribute__((unused))) const override {
        func();
        return 1;
    }
};

/**
 * A function wrapper
 *
 * This function is a wrapper that holds a callable object that takes a
 * WindowInfo,Master, Workspace or Monitor pointer (or void) and returns an
 * int, something convertible to an int or void
 *
 * The arguments are not specified at creation but are dynamically passed in.
 * If a method expects a given type, and a NULL pointer is passed, the function
 * won't be called and 0 (or void) will be returned
 */
struct BoundFunction {
    /**
     * Generic function wrapper;
     * A null function is equivalent to a return 1;
     */
    const std::shared_ptr<GenericFunctionWrapper> func = NULL;
    /**
     * Determines if the caller should proceed after calling this function
     */
    const PassThrough passThrough = PASSTHROUGH_IF_TRUE;
    /**
     * The human readable name of this function.
     * Each BoundFunction should have a unique name.
     */
    const std::string name;
    /**
     * Creates an empty BoundFunction
     *
     * @param passThrough
     */
    BoundFunction(PassThrough passThrough = PASSTHROUGH_IF_TRUE): passThrough(passThrough) {}

    /**
     *
     * @tparam F function
     * @param _func the function to call that is of type R(P)
     * @param name
     * @param passThrough
     */
    template <typename F>
    BoundFunction(F _func, std::string name = "",
        PassThrough passThrough = PASSTHROUGH_IF_TRUE): func(new FunctionWrapper(std::function(_func))),
        passThrough(passThrough),
        name(name) {}


    /// @{
    /**
     * Creates a BoundFunction that when called will return _func(P)
     * Note that value is copied;
     *
     * @tparam R the return parameter or void
     * @tparam P the single argument or void
     * @param _func the function to call that is of type R(P)
     * @param value
     * @param name
     * @param passThrough
     */
    template <typename R, typename P>
    BoundFunction(R(*_func)(P), P value, std::string name = "",
        PassThrough passThrough = ALWAYS_PASSTHROUGH): func(new FunctionWrapper<R, void>(std::function<R()>([ = ]() {return _func(value);}))),
    passThrough(passThrough), name(name) { }
    template <typename R>
    BoundFunction(R(*_func)(std::string), const char* value, std::string name = "",
        PassThrough passThrough = ALWAYS_PASSTHROUGH): BoundFunction(_func, std::string(value), name, passThrough) {}
    /// @}

    /**
     *
     * Note that value is copy constructed
     *
     * @tparam P1
     * @tparam P2
     * @tparam R
     * @param _func function of type R(P1, P2)
     * @param value
     * @param name
     * @param passThrough
     */
    template <typename P1, typename P2, typename R = void>
    BoundFunction(R(*_func)(P1, P2), P2 value, std::string name = "",
        PassThrough passThrough = ALWAYS_PASSTHROUGH): func(new FunctionWrapper<R, P1>(std::function<R(P1)>([ = ](
                            P1 p) {return _func(p, value);}))), passThrough(passThrough), name(name) { }


    /// @return true iff the function is set
    operator bool() const {return func != NULL;}
    /**
     * @return the type of of the underlying function
     */
    BoundFunctionType getType() const {return func->type;};
    /**
     * @return does this function take a string (0) or an int (1) on match
     */
    bool isNumeric() const {return func->type == OTHER_TYPE;};

    /**
     * @return 1 iff this function takes no args
     */
    bool isVoid() const {return func->type == VOID_TYPE;};
    /**
     * Calls the underling function and returns whether the caller should proceed or abort
     *
     * @param arg
     * @return 1 iff the caller should proceed
     */
    bool execute(const BoundFunctionArg& arg)const;
    /**
     * Calls the underlying function and returns the result
     *
     * For functions that return void, 1 is returned
     *
     * @param arg
     *
     * @return
     */
    int call(const BoundFunctionArg& arg)const;
    /// @copydoc call
    int operator()(const BoundFunctionArg& arg={})const {
        return execute(arg);
    }
    /**
     * @param boundFunction
     *
     * @return 1 iff the names match
     */
    bool operator==(const BoundFunction& boundFunction)const;
    /**
     *
     * @return the human readable name of this function
     */
    std::string getName()const {return name;}
    /**
     *
     *
     * @param stream
     * @param boundFunction
     *
     * @return
     */
    friend std::ostream& operator<<(std::ostream& stream, const BoundFunction& boundFunction);
} ;

/**
 * Deletes all normal and batch event rules
 */
void clearAllRules();
/**
 * @param i index of eventRules
 * @return the specified event list
 */
ArrayList<BoundFunction*>& getEventRules(int i);

/**
 * @param i index of batch event rules
 * @return the specified event list
 */
ArrayList<BoundFunction*>& getBatchEventRules(int i);

/**
 * Deletes every eventRule and BatchEvent rule
 */
void deleteAllRules() ;
/**
 * Increment the counter for a batch rules.
 * All batch rules with a non zero counter will be trigged
 * on the next call to incrementBatchEventRuleCounter()
 *
 * @param i the event type
 */
void incrementBatchEventRuleCounter(int i);
/**
 * For every event rule that has been triggered since the last call
 * incrementBatchEventRuleCounter(), trigger the corresponding batch rules
 *
 */
void applyBatchEventRules(void);
/**
 * Returns the number of times an event has been triggered since the last corresponding batch event call;
 *
 * @param type
 *
 * @return
 */
int getNumberOfEventsTriggerSinceLastIdle(int type);

/**
 *
 * Apply the list of rules designated by the type
 *
 * @param type
 * @param arg
 *
 * @return the result
 */
int applyEventRules(int type, const BoundFunctionArg& arg = {});
/**
 * Apply the list of rules designated by the type
 *
 * @param type
 * @param winInfo
 * @param master
 * @return the result
 */
static inline int applyEventRules(int type, WindowInfo* winInfo, Master* master = getActiveMaster()) {
    return applyEventRules(type, {.winInfo = winInfo, .master = master});
}
#endif
