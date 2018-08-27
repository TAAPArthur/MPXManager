/**
 * @file util.c
 */
/// \cond
#include <assert.h>
#include <stdlib.h>
/// \endcond

#include "util.h"

static void _join(Node*prev,Node*next){
    if(prev)
        prev->next=next;
    if(next)
        next->prev=prev;
}

Node* createEmptyHead(){
    return calloc(1, sizeof(Node));
}
Node* createHead(void*value){
    Node*head= createEmptyHead();
    head->value=value;
    return head;
}
Node* createCircularHead(void*value){
    Node*head=createHead(value);
    head->next=head;
    head->prev=head;
    return head;
}

int isNotEmpty(Node*head){
    assert(head);
    return head->value?1:0;
}
int getSize(Node*head){
    assert(head);
    int size=0;
    FOR_EACH_CIRCULAR(head,size++)
    return size;
}


Node* isInList(Node* list,int value){
    assert(list);
    FOR_EACH_CIRCULAR(list,if(value==(*(int*)list->value))return list;);
    return NULL;
}

Node* getLast(Node* list){
    assert(list!=NULL);
    UNTIL_FIRST(list, !list->next)
    return list;
}

int getIntValue(Node*node){
    return node->value?*(int*)node->value:0;
}
void* getValue(Node*node){
    return node?node->value:NULL;
}


static void _freeNode(Node* node){
    if(!node)return;
    free(node);
    return;
}
void partitionLeft(Node*node){
    if(node->prev)
        node->prev->next=NULL;
    node->prev=NULL;
}
void partitionRight(Node*node){
    if(node->next)
        node->next->prev=NULL;
    node->next=NULL;
}
void clearList(Node* head){
    assert(head);
    while(head->next && head->next!=head){
        Node*temp=head->next->next;
        _freeNode(head->next);
        head->next=temp;
    }
    head->prev=head->next;
    head->value=NULL;
}

static void _deleteList(Node* head,int freeValue){
    assert(head);
    partitionLeft(head);
    while(head){
        Node*temp=head->next;
        if(freeValue)
            free(getValue(head));

        _freeNode(head);
        head=temp;
    }
}
void deleteList(Node* head){
    _deleteList(head,1);
}
void destroyList(Node* head){
    _deleteList(head,0);
}

/**
 * Removes the give node from the list.
 * If node is the head of the list,
     * If the list is empty, node->value is set to null
     * else node->value is et to node->next->value and popNode is called on node->next
 * @param node
 * @return a pointer to the node that was removed or NULL
 */
static Node* popNode(Node* node){

    assert(node);
    if(!node->prev){ //head of list
        if(node->next){
            swap(node,node->next);
            node=node->next;
        }
        else{
            node->value=NULL;
            return NULL;
        }
    }
    if(node->prev==node->next && node->next==node){
        node->value=NULL;
        return NULL;
    }

    Node*prev=node->prev;
    Node*next=node->next;
    partitionLeft(node);
    partitionRight(node);
    _join(prev,next);
    return node;
}
void deleteNode(Node* node){
    if(getValue(node))
        free(getValue(node));
    softDeleteNode(node);
}
void softDeleteNode(Node* node){
    _freeNode(popNode(node));
}

void insertBefore(Node* head,Node* newNode){
    assert(isNotEmpty(head));
    assert(isNotEmpty(newNode));


    if(head->prev){
        insertAfter(head->prev, newNode);
        return;
    }
    Node*nextNode=newNode->next;
    //disconnect node from later values
    partitionRight(newNode);
    //set new Nodes' value to head
    swap(head,newNode);
    //insert head's old value
    insertAfter(head, newNode);
    //insert the reset of the new node list between head and newNode
    if(nextNode)
        insertAfter(head, nextNode);
}
void insertAfter(Node* node,Node* newNode){
    assert(isNotEmpty(node));
    assert(isNotEmpty(newNode));
    partitionLeft(newNode);
    Node*end=getLast(newNode);
    _join(end,node->next);
    _join(node,newNode);
}

void insertTail(Node* head,void *value){
    if(isNotEmpty(head))
        insertBefore(head, createHead(value));
    else head->value=value;
}
void insertHead(Node* head,void *value){
    assert(head!=NULL);
    assert(value!=NULL);
    if(head->value){
        Node* newNode = createHead(value);
        if(head->prev)
            insertBefore(head, newNode);
        else{
            swap(newNode,head);
            insertAfter(head,newNode);
        }
    }
    else
        head->value=value;
}
int shiftToHead(Node* list,Node *node){
    assert(node);
    assert(list);
    if(list==node)
        return 0;

    assert(node->prev);
    //TODO handle case where node is not in list?
    Node *singleNode=popNode(node);
    assert(singleNode==node);
    insertBefore(list,singleNode);
    return 0;
}
void swap(Node* n,Node* n2){
    void*temp=n->value;
    n->value=n2->value;
    n2->value=temp;
}
int addUnique(Node* head,void* value){
    assert(head!=NULL);
    Node* node=isInList(head, *(int*)value);

    if(!node)
        insertHead(head, value);
    else
        shiftToHead(head, node);
    return node?0:1;
}
