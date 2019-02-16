/**
 * @file util.c
 */
/// \cond
#include <assert.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>
/// \endcond

#include "util.h"

///init number of elemenets an arraylist can hold
#ifndef INITIAL_ARRAY_LIST_CAP
    #define INITIAL_ARRAY_LIST_CAP 8
#endif

void msleep(int mil){
    int sec=mil/1000;
    mil=mil%1000;
    nanosleep((const struct timespec[]){{sec, mil*1e6}}, NULL);
}

unsigned int getTime () {
    struct timeval start;
    gettimeofday(&start, NULL);
    return start.tv_sec*1000+start.tv_usec*1e-3;
}

static void initArrayList(ArrayList*list){
    list->size=0;
    list->maxSize=INITIAL_ARRAY_LIST_CAP;
    list->arr=malloc(sizeof(void*)*list->maxSize);
}
void* getElement(ArrayList*list,int index){
    return list->arr[index];
}
void setElement(ArrayList*list,int index,void *value){
    list->arr[index]=value;
}

int isNotEmpty(ArrayList*list){
    return getSize(list)?1:0;
}
int indexOf (ArrayList*list,void*value,int size){
    for(int n=0;n<getSize(list);n++)
        if(memcmp(getElement(list,n),value,size)==0)
            return n;
    return -1;
}
void*find(ArrayList*list,void*value,int size){
    int index=indexOf(list,value,size);
    return index==-1?NULL:getElement(list,index);
}
void*getHead(ArrayList*list){
    return getElement(list,0);
}
void* pop(ArrayList*list){
    return removeFromList(list,getSize(list)-1);
}
void* removeFromList(ArrayList*list,int index){
    void*value=getElement(list,index);
    assert(index>=0 && index<getSize(list));
    for(int i=index;i<getSize(list)-1;i++)
        setElement(list,i,getElement(list,i+1));
    list->size--;
    return value;
}
void shiftToHead(ArrayList*list,int index){
    void*newHead=getElement(list,index);
    for(int i=index;i>0;i--)
        setElement(list,i,getElement(list,i-1));
    setElement(list,0,newHead);
}
int getSize(ArrayList*list){
    return list->size;
}
int getNextIndex(ArrayList*list,int current,int delta){
    assert(isNotEmpty(list));
    return (current+delta%getSize(list)+getSize(list))%getSize(list);
}
static inline void autoResize(ArrayList*list){
    if(list->size==list->maxSize){
        list->maxSize*=2;
        list->arr=realloc(list->arr,list->maxSize*sizeof(void*));
    }
}
void* getLast(ArrayList*list){
    return getElement(list,getSize(list)-1);
}
void prependToList(ArrayList*list,void* value){
    addToList(list,value);
    shiftToHead(list,getSize(list)-1);
}

int addUnique(ArrayList*list,void* value,int size){
    if(!find(list,value,size))
        addToList(list,value);
    else return 0;
    return 1;
};
void addToList(ArrayList*list,void* value){
    if(list->arr==NULL)
        initArrayList(list);
    autoResize(list);
    setElement(list,list->size++,value);
}
void clearList(ArrayList*list){
    if(list->arr)
        free(list->arr);
    list->size=0;
    list->maxSize=INITIAL_ARRAY_LIST_CAP;
    list->arr=NULL;
}
void deleteList(ArrayList*list){
    for(int i=0;i<getSize(list);i++)
        free(getElement(list,i));
    clearList(list);
}
void swap(ArrayList*list,int index1,int index2){
    void*temp=getElement(list,index1);
    list->arr[index1]=getElement(list,index2);
    list->arr[index2]=temp;
}
