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
 * Holds a generic function
 */
struct GenericFunctionWrapper {
    /// @{
    /// Used to distinguished different types of supported functions
    template<typename V>
    using EnableIfWinInfo = typename std::enable_if_t<std::is_same<V, WindowInfo*>::value, int>;
    template<typename V>
    using EnableIfWin = typename std::enable_if_t<std::is_convertible<V, WindowID>::value, int>;
    template<typename V>
    using EnableIfMaster = typename std::enable_if_t<std::is_same<V, Master*>::value, int>;
    template<typename V>
    using EnableIfWorkspace = typename std::enable_if_t<std::is_same<V, Workspace*>::value, int>;
    template<typename V>
    using EnableIfMonitor = typename std::enable_if_t<std::is_same<V, Monitor*>::value, int>;
    /// @}

    /**
     * Calls the stored function
     *
     * @param winInfo the windowInfo to operate on or NULL
     * @param master the master to operate on or NULL
     *
     * @return the result of the function
     */
    virtual int call(WindowInfo* winInfo = NULL, Master* master = NULL)const = 0;
};

/**
 * Wrapper for a function that takes a single parameter and returns void
 *
 *
 * @tparam R
 * @tparam P
 */
template <typename R, typename P>
struct FunctionWrapper: GenericFunctionWrapper {
    static_assert(std::is_convertible<R, int>::value);
    static_assert(std::is_convertible<P, int>::value || std::is_base_of<WMStruct, std::remove_pointer_t<P>>::value);
    /// generic function
    std::function<R(P)>func;
    /**
     * @param func the function to call
     */
    FunctionWrapper(std::function<R(P)>func): func(func) {}
    /**
     * @param func the function to call
     */
    FunctionWrapper(R(*func)(P)): func(func) {}

    int call(WindowInfo* winInfo = NULL, Master* master = NULL)const override {
        return _call(winInfo, master);
    }

    template<typename V = P>
    EnableIfWinInfo<V> _call(WindowInfo* winInfo = NULL, Master* master __attribute__((unused)) = NULL) const {
        return winInfo ? func(winInfo) : 0;
    }
    template<typename V = P>
    EnableIfWin<V> _call(WindowInfo* winInfo = NULL, Master* master __attribute__((unused)) = NULL) const {
        return winInfo ? func(winInfo->getID()) : 0;
    }
    template<typename V = P>
    EnableIfMaster<V> _call(WindowInfo* winInfo __attribute__((unused)) = NULL, Master* master = NULL) const {
        return master ? func(master) : 0;
    }
    template<typename V = P>
    EnableIfWorkspace<V> _call(WindowInfo* winInfo __attribute__((unused)) = NULL, Master* master = NULL) const {
        return master && master->getWorkspace() ? func(master->getWorkspace()) : 0;
    }
    template<typename V = P>
    EnableIfMonitor <V> _call(WindowInfo* winInfo __attribute__((unused)) = NULL, Master* master = NULL) const {
        return master && master->getWorkspace() &&
            master->getWorkspace()->getMonitor() ? func(master->getWorkspace()->getMonitor()) : 0;
    }
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
    FunctionWrapper(std::function<void(P)>func): func(func) {}
    int call(WindowInfo* winInfo = NULL, Master* master = NULL)const override {
        return _call(winInfo, master);
    }
    template<typename V = P>
    EnableIfWinInfo<V> _call(WindowInfo* winInfo = NULL, Master* master __attribute__((unused)) = NULL) const {
        if(winInfo)
            func(winInfo);
        return winInfo ? 1 : 0;
    }
    template<typename V = P>
    EnableIfWin<V> _call(WindowInfo* winInfo = NULL, Master* master __attribute__((unused)) = NULL) const {
        if(winInfo)
            func(winInfo->getID());
        return winInfo ? 1 : 0;
    }
    template<typename V = P>
    EnableIfMaster<V> _call(WindowInfo* winInfo __attribute__((unused)) = NULL, Master* master = NULL) const {
        if(master)
            func(master);
        return master ? 1 : 0;
    }
    template<typename V = P>
    EnableIfWorkspace<V> _call(WindowInfo* winInfo __attribute__((unused)) = NULL, Master* master = NULL) const {
        if(master && master->getWorkspace())
            func(master->getWorkspace());
        return master ? 1 : 0;
    }
    template<typename V = P>
    EnableIfMonitor <V> _call(WindowInfo* winInfo __attribute__((unused)) = NULL, Master* master = NULL) const {
        if(master && master->getWorkspace() && master->getWorkspace()->getMonitor())
            func(master->getWorkspace()->getMonitor());
        return master ? 1 : 0;
    }
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
    int call(WindowInfo* winInfo __attribute__((unused)) = NULL,
        Master* master __attribute__((unused)) = NULL) const override {
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
    int call(WindowInfo* winInfo __attribute__((unused)) = NULL,
        Master* master __attribute__((unused)) = NULL) const override {
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

    /// @{
    /**
     *
     * @tparam R the return parameter or void
     * @tparam P the single argument or void
     * @param _func the function to call that is of type R(P)
     * @param name
     * @param passThrough
     */
    template <typename R, typename P>
    BoundFunction(R(*_func)(P), std::string name = "",
        PassThrough passThrough = PASSTHROUGH_IF_TRUE): func(new FunctionWrapper(_func)), passThrough(passThrough),
        name(name) {}
    template <typename R>
    BoundFunction(R(*_func)(), std::string name = "",
        PassThrough passThrough = PASSTHROUGH_IF_TRUE): func(new FunctionWrapper<R, void>(std::function<R()>(_func))),
        passThrough(passThrough),
        name(name) {}
    template <typename P>
    BoundFunction(void(*_func)(P), std::string name = "",
        PassThrough passThrough = PASSTHROUGH_IF_TRUE): func(new FunctionWrapper<void, P>(std::function<void(P)>(_func))),
        passThrough(passThrough),
        name(name) {}
    BoundFunction(void(*_func)(), std::string name = "",
        PassThrough passThrough = PASSTHROUGH_IF_TRUE): func(new FunctionWrapper<void, void>(_func)), passThrough(passThrough),
        name(name) {}
    /// @}



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
    template <typename P>
    BoundFunction(void(*_func)(P), P value, std::string name = "",
        PassThrough passThrough = ALWAYS_PASSTHROUGH): func(new FunctionWrapper<void, void>(std::function<void()>([ = ]() {_func(value);}))),
    passThrough(passThrough), name(name) { }
    /// @}

    /// @{
    /**
     *
     * Note that value is copy constructed
     *
     * @tparam R
     * @tparam P1
     * @tparam P2
     * @param _func function of type R(P1, P2)
     * @param value
     * @param name
     * @param passThrough
     */
    template <typename R, typename P1, typename P2>
    BoundFunction(R(*_func)(P1, P2), P2 value, std::string name = "",
        PassThrough passThrough = ALWAYS_PASSTHROUGH): func(new FunctionWrapper<R, P1>(std::function<R(P1)>([ = ](
                            P1 p) {return _func(p, value);}))), passThrough(passThrough), name(name) { }
    template <typename P1, typename P2>
    BoundFunction(void(*_func)(P1, P2), P2 value, std::string name = "",
        PassThrough passThrough = ALWAYS_PASSTHROUGH): func(new FunctionWrapper<void, P1>(std::function<void(P1)>([ = ](
                            P1 p) {_func(p, value);}))), passThrough(passThrough), name(name) { }
    /// @}

    /**
     * Calls the underling function and returns whether the caller should proceed or abort
     *
     * @param w
     * @param m
     *
     * @return 1 iff the caller should proceed
     */
    bool execute(WindowInfo* w = NULL, Master* m = NULL)const;
    /**
     * Calls the underlying function and returns the result
     *
     * For functions that return void, 1 is returned
     *
     * @param winInfo
     * @param master
     *
     * @return
     */
    int call(WindowInfo* winInfo = NULL, Master* master = NULL)const;
    /// @copydoc call
    int operator()(WindowInfo* winInfo = NULL, Master* master = NULL)const {
        return execute(winInfo, master);
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
 * Apply the list of rules designated by the type
 *
 * @param type
 * @param winInfo
 * @param m
 * @return the result
 */
int applyEventRules(int type, WindowInfo* winInfo = NULL, Master* m = getActiveMaster());
#endif
