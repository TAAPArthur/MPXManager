#ifndef MPX_BOUND_FUNCTION_H
#define MPX_BOUND_FUNCTION_H

#include <assert.h>
#include <stdint.h>
#include <sys/types.h>
#include <type_traits>
#include "masters.h"
#include "windows.h"
#include "mywm-structs.h"

#define DEFAULT_EVENT(F){F, #F,PASSTHROUGH_IF_TRUE}
#define PASSTHROUGH_EVENT(F,P){F, #F,P}
#define FUNC_NAME (std::string("_")+__FUNCTION__)

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

struct GenericFunctionWrapper {
    template<typename U, typename V >
    using EnableIfWinInfo = typename std::enable_if_t<std::is_same<V, WindowInfo*>::value, int>;
    template<typename U, typename V >
    using EnableIfWin = typename std::enable_if_t<std::is_same<V, WindowID>::value, int>;
    template<typename U, typename V >
    using EnableIfMaster = typename std::enable_if_t<std::is_same<V, Master*>::value, int>;
    template<typename U, typename V >
    using EnableIfWorkspace = typename std::enable_if_t<std::is_same<V, Workspace*>::value, int>;
    template<typename U, typename V >
    using EnableIfMonitor = typename std::enable_if_t<std::is_same<V, Monitor*>::value, int>;
    int call(WindowInfo* winInfo = NULL, Master* master = NULL)const {
        return _call(winInfo, master);
    } ;
    virtual int _call(WindowInfo* winInfo = NULL, Master* master = NULL)const = 0;
};


template <typename R, typename P>
struct FunctionWrapper: GenericFunctionWrapper {
    R(*func)(P);
    FunctionWrapper(R(*func)(P)): func(func) {}

    static_assert(std::is_convertible<R, int>::value);
    int _call(WindowInfo* winInfo = NULL, Master* master = NULL)const override {
        return call(winInfo, master);
    }

    template<typename U = R, typename V = P>
    EnableIfWinInfo<U, V> call(WindowInfo* winInfo = NULL, Master* master __attribute__((unused)) = NULL) const {
        return winInfo ? func(winInfo) : 0;
    }
    template<typename U = R, typename V = P>
    EnableIfMaster<U, V> call(WindowInfo* winInfo __attribute__((unused)) = NULL, Master* master = NULL) const {
        return master ? func(master) : 0;
    }
    template<typename U = R, typename V = P>
    EnableIfWin<U, V> call(WindowInfo* winInfo = NULL, Master* master __attribute__((unused)) = NULL) const {
        return winInfo ? func(winInfo->getID()) : 0;
    }
    template<typename U = R, typename V = P>
    EnableIfWorkspace<U, V> call(WindowInfo* winInfo __attribute__((unused)) = NULL, Master* master = NULL) const {
        return master && master->getWorkspace() ? func(master->getWorkspace()) : 0;
    }
    template<typename U = R, typename V = P>
    EnableIfMonitor <U, V> call(WindowInfo* winInfo __attribute__((unused)) = NULL, Master* master = NULL) const {
        return master && master->getWorkspace() &&
               master->getWorkspace()->getMonitor() ? func(master->getWorkspace()->getMonitor()) : 0;
    }
};
template <typename P>
struct FunctionWrapper<void, P>: GenericFunctionWrapper {
    void (*func)(P);
    FunctionWrapper(void(*func)(P)): func(func) {}
    int _call(WindowInfo* winInfo = NULL, Master* master = NULL)const override {
        return call(winInfo, master);
    }
    template< typename V = P>
    EnableIfWinInfo<void, V> call(WindowInfo* winInfo = NULL, Master* master __attribute__((unused)) = NULL) const {
        if(winInfo)
            func(winInfo);
        return 1;
    }
    template< typename V = P>
    EnableIfMaster<void, V> call(WindowInfo* winInfo __attribute__((unused)) = NULL, Master* master = NULL) const {
        if(master)
            func(master);
        return 1;
    }
    template< typename V = P>
    EnableIfWin<void, V> call(WindowInfo* winInfo = NULL, Master* master __attribute__((unused)) = NULL) const {
        if(winInfo)
            func(winInfo->getID());
        return 1;
    }
    template<typename V = P>
    EnableIfWorkspace<void, V> call(WindowInfo* winInfo __attribute__((unused)) = NULL, Master* master = NULL) const {
        if(master && master->getWorkspace())
            func(master->getWorkspace());
        return 1;
    }
    template<typename V = P>
    EnableIfMonitor <void, V> call(WindowInfo* winInfo __attribute__((unused)) = NULL, Master* master = NULL) const {
        if(master && master->getWorkspace() && master->getWorkspace()->getMonitor())
            func(master->getWorkspace()->getMonitor());
        return 1;
    }
};
template <typename R>
struct FunctionWrapper<R, void>: GenericFunctionWrapper {
    static_assert(std::is_convertible<R, int>::value);
    R(*func)();
    FunctionWrapper(R(*func)()): func(func) {}
    int _call(WindowInfo* winInfo = NULL, Master* master = NULL)const override {
        return call(winInfo, master);
    }
    int call(WindowInfo* winInfo __attribute__((unused)) = NULL, Master* master __attribute__((unused)) = NULL) const {
        return func();
    }
};
template <>
struct FunctionWrapper<void, void>: GenericFunctionWrapper {
    void (*func)();
    FunctionWrapper(void(*func)()): func(func) {}
    int _call(WindowInfo* winInfo = NULL, Master* master = NULL)const override {
        return call(winInfo, master);
    }
    int call(WindowInfo* winInfo __attribute__((unused)) = NULL, Master* master __attribute__((unused)) = NULL) const {
        func();
        return 1;
    }
};

#include <memory>
/**
 * Used to bind a function with arguments to a Rule or Binding
 * such that when the latter is triggered the bound function
 * is called with arguments
 */
struct BoundFunction {
    const std::shared_ptr<GenericFunctionWrapper> func = NULL;
    const PassThrough passThrough =  ALWAYS_PASSTHROUGH;
    const std::string name;
    BoundFunction(PassThrough passThrough = ALWAYS_PASSTHROUGH): passThrough(passThrough) {}
    template <typename R, typename P>
    BoundFunction(R(*func)(P), std::string name = "",
                  PassThrough passThrough = ALWAYS_PASSTHROUGH): func(new FunctionWrapper(func)), passThrough(passThrough), name(name) {}
    template <typename R>
    BoundFunction(R(*func)(), std::string name = "",
                  PassThrough passThrough = ALWAYS_PASSTHROUGH): func(new FunctionWrapper<R, void>(func)), passThrough(passThrough),
        name(name) {
    }
    template <typename P>
    BoundFunction(void(*func)(P), std::string name = "",
                  PassThrough passThrough = ALWAYS_PASSTHROUGH): func(new FunctionWrapper<void, P>(func)), passThrough(passThrough),
        name(name) {
    }
    BoundFunction(void(*func)(), std::string name = "",
                  PassThrough passThrough = ALWAYS_PASSTHROUGH): func(new FunctionWrapper<void, void>(func)), passThrough(passThrough),
        name(name) {
    }


    int call(WindowInfo* w = NULL, Master* m = NULL)const;
    int execute(WindowInfo* w = NULL, Master* m = NULL)const;
    int operator()(WindowInfo* w = NULL, Master* m = NULL)const;
    bool operator==(const BoundFunction& boundFunction)const;
    std::string getName()const {return name;}
    friend std::ostream& operator<<(std::ostream& stream, const BoundFunction& boundFunction);
} ;

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
int getNumberOfEventsTriggerSinceLastIdle(int type);
/**
 * Apply the list of rules designated by the type
 *
 * @param type
 * @param winInfo
 * @return the result
 */
int applyEventRules(int type, WindowInfo* winInfo);
#endif
