#include <stdlib.h>
#include <assert.h>

#include "util.h"

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
    int size=0;
    FOR_EACH_CIRCULAR(head,size++)
    return size;
}


Node* isInList(Node* list,int value){
    assert(list);
    FOR_EACH_CIRCULAR(list,if(value==(*(int*)list->value))return list;);
    return NULL;
}

Node* getNth(Node* list,int n){
    assert(list!=NULL);
    int i=0;
    if(!isNotEmpty(list))
        return NULL;
    UNTIL_FIRST(list,i++==n || n<0 && !list->next)
    return list;
}

Node* getLast(Node* list){
    return getNth(list, -1);
}


int getIntValue(Node*node){
    return node->value?*(int*)node->value:0;
}
void* getValue(Node*node){
    return node?node->value:NULL;
}


void _freeNode(Node* node){
    if(!node)
        return;
    free(node);
    return;
}
void clearList(Node* head){
    destroyList(head->next);
    head->value=NULL;
}
void destroyList(Node* head){
    if(!head)
        return;
    if(head->prev)
        head->prev->next=NULL;
    while(head){
        Node*temp=head->next;
        _freeNode(head);
        head=temp;
    }
}


Node* popNode(Node* node){

    if(!node)
        return NULL;
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
    node->prev->next=node->next;
    if(node->next)
        node->next->prev=node->prev;
    node->next=node->prev=NULL;
    return node;
}
void deleteNode(Node* node){
    void *value=getValue(node);
    Node*popped=popNode(node);

    if(popped){
        if(popped->value)
            free(popped->value);
        _freeNode(popped);
    }
    else
        free(value);

}
void softDeleteNode(Node* node){
    _freeNode(popNode(node));
}

int removeByValue(Node* list,int value){
    Node *nodeWithValue=isInList(list,value);
    if(nodeWithValue)
        softDeleteNode(nodeWithValue);

    return nodeWithValue?1:0;
}

void insertBefore(Node* head,Node* newNode){
    Node*nextNode=newNode->next;
    //disconnect node from eariler values
    if(newNode->prev){
        newNode->prev->next=NULL;
        newNode->prev=NULL;
    }
    //disconnect node from later values
    if(newNode->next){
        newNode->next->prev=NULL;
        newNode->next=NULL;
    }
    //set new Nodes' value to head
    swap(head,newNode);
    //insert head's old value
    insertAfter(head, newNode);
    //insert the reset of the new node list
    if(nextNode)
        insertAfter(head, nextNode);
}
void insertAfter(Node* node,Node* newNode){
    Node*end=getLast(newNode);
    end->next=node->next;
    newNode->prev=node;
    if(node->next)
        node->next->prev=end;
    node->next=newNode;

}
void insertHead(Node* head,void *value){
    assert(head!=NULL);
    assert(value!=NULL);
    if(head->value){
        Node* newNode = calloc(1,sizeof(Node));
        newNode->value=head->value;
        head->value=value;
        insertAfter(head,newNode);
    }
    head->value=value;
}
int shiftToHead(Node* list,Node *node){
    assert(node!=NULL);
    if(list==node || !node || list==NULL)
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
