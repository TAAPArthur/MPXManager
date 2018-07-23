/**
 * @file util.h
 * @brief Linked list implementation and helper methods
 *
 */

#ifndef MYWM_UTIL
#define MYWM_UTIL

/**
 * @struct
 * @brief A Linked list struct
 * The struct can hold any value, but
 * there are convience methods that work best when the
 * value is/starts with an int.
 *
 * Nodes are just holders for values ie the value a node holds can change to
 * another value in the list. This property is mainly set so that the pointer
 * to the head of a list will always point to the head of the list while the list exists.
 * A list is considered to not exist when it is destroyed (via @see destroyList) or it is
 * joined with another list (@see insertBefore @see insertAfter)
 */
typedef struct node_struct{
    /** The value this node has or NULL if node is empty*/
    void * value;
    /** Pointer to next element in the list or NULL*/
    struct node_struct * next;
    /** Pointer to prev element in the list or NULL*/
    struct node_struct * prev;
} Node;



/**
 * Creates a pointer to a head of an empty node list.
 *
 * This function is equivalent to calling createHead(NULL)
 * @see createHead(void*value)
 * @return a pointer to node
 */
Node* createEmptyHead();
/**
 * Creates a pointer to a node with the given value.
 * @param the value of the new node
 * @return a pointer to node
 */
Node* createHead(void*value);
/**
 * Creates the head of a cirular linked list with the given values.
 * A circular linked list is defined to have the next pointer of the last node
 * point to the head of the list and prev pointer of the head to point to the last element.
 * Once created no function shall break the above invariant.
 * @param value the value of the linked list
 * @return a pointer a node
 */
Node* createCircularHead(void*value);


/**
 * Adds a new new to the list such that the
 * head's current value to reflect value and all pointers to head are still valid
 * @param head
 * @param value
 */
void insertHead(Node* head,void *value);

/**
 * Give list A--C--D, insertAfter(A,B) will yield
 * A--B--C--D
 *
 * Insert a new node with value n->value after n,
 * and change n's value to value.
 * @param n
 * @param value
 */
void insertAfter(Node* node,Node* newNode);
/**
 * Given list A--C--D, insertAfter(A,B) will yield
 * B--A--C--D.
 * The pointer to head will always point to the start of the list
 * even if it means shifting values of other nodes in the list
 * @param head the head of the list
 * @param newNode the new node to inser
 */
void insertBefore(Node* head,Node* newNode);



/**
 * @brief Iterates over head and runs arbitrary command(s).
 * The value of head will be updated upon every iteration.
 * Note that behavior is undefined if the list is modified while iterating
 * @param head - starting point for iteration. Will be set to the NULL upon completion
 */
#define FOR_EACH(head,...) \
        _FOR_EACH(next,head,__VA_ARGS__)

/**
 * @brief Iterates over head and runs arbitrary command(s) util the end
 * is reached or until the list wraps around back to head.
 * The value of head will be updated upon every iteration.
 * Note that behavior is undefined if the list is modified while iterating
 * @param head - starting point for iteration. Will be set to the NULL upon completion
 */
#define FOR_EACH_CIRCULAR(head,...) {\
        Node*__start__=head; \
        FOR_EACH(head,__VA_ARGS__;if(__start__==head->next){head=NULL;break;})\
    }
/**
 * Iterates over head backwards and runs arbitary command(s).
 * The value of head will be updated upon every iteration
 * Note that behavior is undefined if the list is modified while iterating
 * @param head - starting point for iteration. Will be set to the NULL upon completion
 */
#define FOR_EACH_REVERSED(head,...) \
        _FOR_EACH(prev,head,__VA_ARGS__)
/**
 * itererates over head in the specified direction(either next or prev) and
 * runs arbitary command(s).
 * The value of head will be updated upon every iteration
 * head will be set to either the last or front element.
 *
 * Note that behavior is undefined if the list is modified while iterating
 *
 * You probably don't want to use this function
 * @param head  - starting point for iteration. Will be set to the NULL upon completion
 * @see FOR_EACH_REVERSED(head,...)
 * @see FOR_EACH(head,...)
 */
#define _FOR_EACH(dir,head,...) \
    if(head->value)while(head){__VA_ARGS__;head=head->dir;}else head=NULL;

/**
 * Like for each but stops after N iterations
 * @see FOR_EACH(head,...)
 * @param head - starting point for iteration. Will be set to the last element visited
 * @param N - max number of iterations
 */
#define FOR_AT_MOST(head,N,...) \
    {int __temp_size__=0; FOR_EACH(head,if(__temp_size__++==N)break; __VA_ARGS__;)}
/**
 * Iterates over head and set head to the node in the last | EXPR is maximized
 * In the case of ties, the element closet to the head of the list is chosen
 */
#define GET_MAX(head,CONDITION,EXPR) \
    {\
        unsigned int __maxValue__=0;Node*__maxNode__=NULL;\
        FOR_EACH(head,if(CONDITION && (!__maxNode__||EXPR>__maxValue__)){__maxValue__=EXPR;__maxNode__=head;})\
        head=__maxNode__;\
    }

/**
 * Iterates over head until EXPR is true
 * head will be set to the first value that makes EXPR true or
 * NULL if it is never true
 */
#define UNTIL_FIRST(head,EXPR)\
    FOR_EACH_CIRCULAR(head,if(EXPR)break;);




/**
 *
 * @param list
 * @param value
 * @return the node whose value is value or NULL
 */
Node* isInList(Node* list,int value);



/**
 * Returns the number of nodes in this (sub)section of the list
 * Returns the number of elements from head to the end of the list.
 * If this is an cirular list, then it will returns the
 * number of elements between head and itself.
 *
 * Note that if this contains a circular list and head does not start it,
 * this method will hang forever
 * @param head the starting node
 * @return the number of element to the end of the last starting from head
 */
int getSize(Node*head);
/**
 * Checks it the node has a value
 * @param head the node to check
 * @return true if head has a non-Null value
 */
int isNotEmpty(Node*head);

/**
 * Swaps the value of n and n2
 * @param n
 * @param n2
 */
void swap(Node* n,Node* n2);



/**
 * Returns Nth node in list or NULL if n>= the length of the list
 * @param list list to check
 * @param n
 * @return
 */
Node* getNth(Node* list,int n);

/**
 * Returns last node in list
 * @param list the last to check
 * @return pointer to the last value in the list
 */
Node* getLast(Node* list);


/**
 * Frees all the nodes in the linked list. The values themselves are not freed
 * @param head
 */
void destroyList(Node* head);

/**
 * Destroys the list but preserves just the refrence to the head
 * @param head
 */
void clearList(Node* head);


/**
 * Removes the give node from the list.
 * If node is the head of the list,
     * If the list is empty, node->value is set to null
     * else node->value is et to node->next->value and popNode is called on node->next
 * @param node
 * @return a pointer to the node that was removed or NULL
 */
Node* popNode(Node* node);
/**
 * Pops node from list and re-inserts it at the head
 * @param list
 * @param node a node in list
 * @return
 */
int shiftToHead(Node* list,Node *node);

/**
 * Pops the node from its list and frees the node and its value.
 * This is the only method that frees the nodes value
 * @param node
 */
void deleteNode(Node* node);

/**
 * Pops the node from its list but does not free the node and its value.
 * @param node
 */
void softDeleteNode(Node* node);

/**
 * Finds the first node with value, pops it.
 * The pointer to the head of the list is garunteed point to a valid node
 * @param list
 * @param value
 * @return 1 iff a value was found
 */
int removeByValue(Node* list,int value);

/**
 * Gets an integer representation of the return value
 * @param node
 * @return
 */
int getIntValue(Node*node);
/**
 * Returns the value of the given node
 * @param node
 * @return
 */
void* getValue(Node*node);


/**
 * Adds a new element if it is not currently in the list.
 * If it is in the list, the first matching element is shifted to the head
 * @param head
 * @param value
 * @return 0 if the value is already present in head 1 o.w.
 */
int addUnique(Node* head,void* value);

#endif
