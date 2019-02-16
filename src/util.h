/**
 * @file util.h
 * @brief ArrayList and other generic helper methods
 *
 */
#ifndef MYWM_UTIL
#define MYWM_UTIL

///Returns the length of the array
#define LEN(X) (sizeof X / sizeof X[0])
/// Returns the min of a,b
#define MIN(a,b) (a<b?a:b)
/// Returns the max of a,b
#define MAX(a,b) (a>b?a:b)

/**
 * Simple ArrayList struct
 */
typedef struct {
    ///backing arr
    void**arr;
    ///number of elements in list
    int size;
    ///cappacity of the list. When size==maxSize, the arr is doubled
    int maxSize;
}ArrayList;



/**
 * Returns a monotonically increasing number that servers a the time in ms.
 * @return the current time (ms)
 */
unsigned int getTime ();
/**
 * Sleep of mil milliseconds
 * @param mil number of milliseconds to sleep
 */
void msleep(int mil);


/**
 * @brief Iterates over list and runs arbitrary command(s).
 * The value of list will be updated upon every iteration.
 * Note that behavior is undefined if the list is modified while iterating
 * @param var the value represents the ith value of list 
 * @param list - starting point for iteration. Will be set to the NULL upon completion
 * @param code - code to run on every iteration
 */
#define FOR_EACH(var,list,code...) \
    for(int _i=0;_i<getSize(list);_i++){\
        var = getElement(list,_i);\
        code;\
    }

/**
 * @brief Iterates backwards over list and runs arbitrary command(s).
 * The value of var will be updated upon every iteration.
 * Note that behavior is undefined if the list is modified while iterating unless the element is modified at the current posistion or later
 * @param var the value represents the ith value of list 
 * @param list the list to iterate over
 * @param code - code to run on every iteration
 */
#define FOR_EACH_REVERSED(var,list,code...) \
    for(int _i=getSize(list)-1;_i>=0;_i--){\
        var = getElement(list,_i);\
        code;\
    }

/**
 * Iterates over head until EXPR is true
 * var will either be set to the first element of head that satifies EXPR or NULL if no such element exists
 * @param var 
 * @param head the list to iterate over
 * @param EXPR the expression to evaluate
 */
#define UNTIL_FIRST(var,head,EXPR)\
    FOR_EACH(var,head,if(EXPR)break;else var=NULL;);


/**
 * @param list
 * @return true if list is not empty (size!=0)
 */
int isNotEmpty(ArrayList*list);


/**
 * Adds value to the end of list
 * @param list the list to append to 
 * @param value the value to append
 */
void addToList(ArrayList*list,void* value);
/**
 * Adds value to the front of the list
 * @param list the list to append to 
 * @param value the value to append
 */
void prependToList(ArrayList*list,void* value);
/**
 * Removes and returns the last element of the list
 * @param list
 * @return the (former) last element of the list
 */
void* pop(ArrayList*list);

/**
 * Removes the index-th element and re-inserts it at the head of the list
 * @param list
 * @param index
 */
void shiftToHead(ArrayList*list,int index);
/**
 * Adds value to the list iff there exists no element whose value is equal to value (wrt the first size bytes)
 * @param list
 * @param value
 * @param size
 * return 1 iff a new element was inserted
 */
int addUnique(ArrayList*list,void* value,int size);
/**
 * Frees all internal memory of list and resets size
 * The list does not need to be initilized again before it can be used
 * @param list
 */
void clearList(ArrayList*list);
/**
 * This method is safe to call on uninitlized lists and cleared lists
 * @param list
 * @return the number of elements in the list
 */
int getSize(ArrayList*list);
/**
 * Note this method does not perform anys bounds checking
 * @param list
 * @param index
 * @return the element at the index-th position in list
 */
void* getElement(ArrayList*list,int index);
/**
 * Note this method does not perform anys bounds checking
 * @param list
 * @param index
 * @param value the new value of the index-th element
 */
void setElement(ArrayList*list,int index,void *value);
/**
 * Returns the first element in the list
 */
void* getHead(ArrayList*list);
/**
 * Clear the list and free all elements
 */
void deleteList(ArrayList*list);
/**
 * Remove the index-th element form list
 * @param list 
 * @param index
 * @return the element that was removed
 */
void* removeFromList(ArrayList*list,int index);
/**
 * @param list
 * @param value
 * @param size
 * @return the index of the element whose mem address starts with the first size byte of value
 */
int indexOf(ArrayList*list,void*value,int size);
/**
 * @param list
 * @param value
 * @param size
 * @return the element whose mem address starts with the first size byte of value
 */
void*find(ArrayList*list,void*value,int size);
/**
 * @param list
 * @return the last element in list
 */
void* getLast(ArrayList*list);
/**
 * Returns the element at index current + delta if the array list where to wrap around
 * For example:
 *   getNextIndex(list,0,-1) returns getSize(list)-1
 *   getNextIndex(list,getSize(list)-1,1) returns 0
 * @param list
 * @param current must be a valid index of the list
 * @param delta
 */
int getNextIndex(ArrayList*list,int current,int delta);
/**
 * Swaps the element at index1 with index2
 * @param list
 * @param index1
 * @param index2
 */
void swap(ArrayList*list,int index1,int index2);
#endif
