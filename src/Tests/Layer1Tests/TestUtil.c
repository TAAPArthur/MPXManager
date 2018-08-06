#include "../UnitTests.h"
#include "../../util.h"

int* createStruct(int i){
    int* t=malloc(sizeof(int));
    if(i<=0)
        i=(long)t;
    *t=i;
    return t;
}

int size=10;
Node* createLinkedListAssert(Node*n,int size,int start,int a){
    if(a)
        assert(getSize(n)==0);
    int cirular=n->prev==n->next&&n->prev==n;
    for(int i=start;i<start+size;i++){
        insertHead(n, createStruct(i));
        if(a){
            assert(isNotEmpty(n));
            assert(getIntValue(n)==i);
            assert(getSize(n)==i-start+1);
            if(i>1){
                assert(getIntValue(n->next)==i-1);
                assert(getIntValue(n->next->prev)==i);
            }
            if(!cirular)
                assert(n->prev==NULL);
        }
    }
    return n;
}

Node* createRedundentLinkedList(int size){
    Node* n=createEmptyHead();
    for(int i=1;i<=size;i++){
        insertHead(n, createStruct(i));
        insertHead(n, createStruct(i));
        assert(getIntValue(n)==i);
        if(i>1)
            assert(getIntValue(n->next)==i);
    }
    return n;
}
Node* createLinkedList(int size){
    return createLinkedListAssert(createEmptyHead(),size,1,0);
}
Node* fillListLinkedList(Node*n,int size){
    return createLinkedListAssert(n,size,1,0);
}

int nodeTypes=7;
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
Node* getNodeType(int i){
    switch(i){
        default : return createEmptyHead();
        case 1: return createCircularHead(NULL);
        case 2: return createLinkedList(size=2);
        case 3: return createLinkedList(size);
        case 4: return fillListLinkedList(createCircularHead(NULL), size=2);
        case 5: return fillListLinkedList(createCircularHead(NULL), size);
        case 6: return createRedundentLinkedList(size);
    }
}
Node*createCopy(Node*n){
    Node*copy=createEmptyHead();
    FOR_EACH_CIRCULAR(n,insertHead(copy, getValue(n)));
    return copy;
}
int isCircular(Node*n){
    int size=getSize(n);
    if(size){
        FOR_AT_MOST(n,size+1);
        return n?1:0;
    }
    else
        return n->next==n->prev && n->prev==n;
}



int getCount(Node*node,int value){
    int count=0;
    FOR_EACH_CIRCULAR(node,count+=(getIntValue(node)==value))
    return count;
}

void assertEmptyHead(Node*head){
    assert(NULL!=head);
    assert(NULL==head->next);
    assert(NULL==head->prev);
    assert(NULL==head->value);
    assert(isNotEmpty(head)==0);
    assert(getSize(head)==0);
}

/**
 * Ensures node's values are initialized to NULL
 * @param START_TEST(test_create_head)
 */
START_TEST(test_create_head){
    Node*head=createEmptyHead();
    assertEmptyHead(head);
    deleteList(head);
    head=createHead(NULL);
    assertEmptyHead(head);

    deleteList(head);
    void*p=createStruct(1);
    head=createHead(p);
    assert(head!=NULL);
    assert(head->value==p);
    assert(head->next==NULL);
    assert(head->prev==NULL);
    deleteList(head);
}END_TEST

START_TEST(test_create_cirular_head){
    Node*head=createCircularHead(NULL);
    assert(getValue(head)==NULL);
    assert(!isNotEmpty(head));
    assert(getSize(head)==0);
    assert(head->next==head);
    assert(head->prev==head);
    deleteList(head);
    head=createCircularHead(createStruct(1));
    assert(getValue(head));
    assert(isNotEmpty(head));
    assert(getSize(head)==1);
    assert(head->next==head);
    assert(head->prev==head);
    deleteList(head);
}END_TEST
START_TEST(test_insert_unique){
    int*v1=createStruct(1);
    int*v1Clone=createStruct(1);
    int*v2=createStruct(2);
    Node*n1=createHead(v1);
    assert(!addUnique(n1, v1));
    assert(!addUnique(n1, v1Clone));
    assert(addUnique(n1, v2));
    free(v1Clone);
    deleteList(n1);
}END_TEST
START_TEST(test_single_node_insertion){
    Node*n1=createHead(createStruct(1));

    int value=size+1;
    Node* singleNode=createHead(createStruct(value));
    int originalHeadValue=getIntValue(n1);

    insertBefore(n1, singleNode);
    //n's value should now be the head of the list;
    assert(getIntValue(n1)==value);
    assert(getIntValue(n1->next)==originalHeadValue);
    assert(getSize(n1)==2);

    int value2=size+2;
    singleNode=createHead(createStruct(value2));

    insertAfter(n1, singleNode);
    //head value should not have changed
    assert(getIntValue(n1)==value);
    assert(getIntValue(n1->next)==value2);
    assert(getSize(n1)==3);
    deleteList(n1);

}END_TEST

START_TEST(test_cirular_insert_in_place){
    Node*head=fillListLinkedList(createCircularHead(NULL), size);
    int value=getIntValue(head);

    assert(getSize(head)==size);
    for(int i=size+1;i<=size*2;i++){
        insertBefore(head, createHead(createStruct(i)));
        assert(getSize(head)==i);
        assert(getIntValue(head->prev)==i);
    }
    assert(value==getIntValue(head));
    assert(head->prev);
    deleteList(head);
}END_TEST


/**Teests get size, and insert head*/
START_TEST(test_create_list){
    deleteList(createLinkedListAssert(createEmptyHead(),size, 1,1));
}END_TEST


/**
 * Tests FOR_EACH and FOR_EACH_REVERSED
 */
START_TEST(test_for_each){

    int reversed=_i;
    Node*head=createLinkedList(size);
    Node*temp=head;
    if(reversed)
        head=getLast(head);
    Node*position=head;
    if(!reversed){
        FOR_EACH(head,
            assert(head==position);
            assert(head!=NULL);
            position=position->next;);
    }
    else{
        FOR_EACH_REVERSED(head,
            assert(head==position);
            assert(head!=NULL);
            position=position->prev;);
    }
    assert(head==NULL);
    assert(position==NULL);
    deleteList(temp);

    //assert should never be reaaced
    head=createEmptyHead();
    temp=head;
    if(reversed){
        FOR_EACH(head,assert(0));
    }
    else{
        FOR_EACH_REVERSED(head,assert(0));
    }
    assert(head==NULL);
    deleteList(temp);
}END_TEST

START_TEST(test_for_at_most){
    Node*head=createEmptyHead();
    Node*temp=head;
    FOR_AT_MOST(head,1,assert(0));
    assert(head==NULL);
    deleteList(temp);
    head=createLinkedList(size);
    temp=head;
    FOR_AT_MOST(head,0,assert(0));

    //no iteration should occur when n==0
    assert(head->prev==NULL);
    //check to see if iters exactly n times
    int i=0;
    FOR_AT_MOST(head,2,i++);
    assert(i==2);
    FOR_AT_MOST(head,size+1,i++);
    assert(i==size);
    assert(head==NULL);
    deleteList(temp);
}END_TEST

START_TEST(test_util_first){
    Node*head=createEmptyHead();
    Node*temp=head;
    UNTIL_FIRST(head,1);
    assert(head==NULL);
    deleteList(temp);
    head=createHead(createStruct(1));
    temp=head;
    int i=1;
    UNTIL_FIRST(head,i--);
    assert(i==0);
    deleteList(temp);

    head=createLinkedList(size);
    temp=head;
    int headValue=getIntValue(head);
    UNTIL_FIRST(head,1);
    assert(getIntValue(head)==headValue);
    i=0;
    int nextHeadValue=getIntValue(head->next);
    UNTIL_FIRST(head,i++);
    assert(getIntValue(head)==nextHeadValue);
    UNTIL_FIRST(head,0);
    assert(head==NULL);
    deleteList(temp);

}END_TEST
START_TEST(test_iter_circular){
    Node*head=fillListLinkedList(createCircularHead(NULL), size);
    Node*temp=head;
    UNTIL_FIRST(head,0)
    assert(head==NULL);
    head=temp;
    FOR_EACH_CIRCULAR(head,)
    assert(head==NULL);
    deleteList(temp);
}END_TEST

START_TEST(test_in_list){
    Node* n=createEmptyHead();
    for(int i=0;i<size;i++)
        assert(!isInList(n, i));
    deleteList(n);
    n=createLinkedList(size);
    assert(!isInList(n, 0));
    for(int i=0;i<size;i++)
        assert(isInList(n, i+1));
    deleteList(n);
}END_TEST
int assert_not_reached(){
    assert(0);
    return 0;
}
START_TEST(test_get_max){
    Node*head=createEmptyHead();
    Node*temp=head;
    GET_MAX(head,1,assert_not_reached());
    assert(head==NULL);
    deleteList(temp);
    head=createLinkedList(size);
    temp=head;
    for(int i=1;i<-size;i++){
        head=temp;
        GET_MAX(head,getIntValue(head)<=i,getIntValue(head));
        assert(getIntValue(head)==i);
    }
    deleteList(temp);
}END_TEST
/**
 * Checks getLast
 * @param START_TEST(test_get_last)
 */
START_TEST(test_get_last){
    Node* n=createEmptyHead();
    assert(getLast(n)==NULL);
    deleteList(n);
    n=createHead(createStruct(1));
    assert(n==getLast(n));
    deleteList(n);

    n=createLinkedList(size);
    Node*last=getLast(n);
    assert(last==getLast(n));
    assert(last->next==NULL);
    assert(last->prev!=NULL);
    deleteList(n);
} END_TEST

/**
 * Test insertBefore and insertAfter
 */
START_TEST(test_list_join){
    for(int i=0;i<6;i++){
        Node*n1=createLinkedList(size);
        Node*n2=createLinkedListAssert(createEmptyHead(),size,size+1,0);

        Node*temp1=NULL,*temp2=NULL;
        assert(getSize(n1)==size);
        assert(getSize(n2)==size);

        int totalSize = size*2 - i/2;
        if(i==2 || i>=4){
            temp2=n2;
            n2=n2->next;
        }
        if(i==3 || i>=4){
            temp1=n1;
            n1=n1->next;
        }
        int v1=getIntValue(i%2?n2:n1);
        int v2=getIntValue(i%2?n1:n2);

        Node*head=n1;
        if(i%2){
            int v3=getIntValue(n2->next);
            int insertingNotAtHead=n1->prev!=NULL;
            insertBefore(n1, n2);
            if(insertingNotAtHead){
                assert(getIntValue(n1)==v2);
                assert(getIntValue(n2)==v1);
                head=n2;
            }
            else
                assert(getIntValue(n1->next)==v3);

        }
        else{
            insertAfter(n1, n2);
            assert(getIntValue(n1->next)==v2);
        }
        assert(getIntValue(head)==v1);
        assert(getSize(head)==totalSize);
        //check to make sure every value is present
        int hits=0;
        for(int i=1;i<=size*2;i++)
            hits+=isInList(head, i)?1:0;
        assert(hits==totalSize);
        deleteList(head);
        if(temp1){
            assert(getSize(temp1)==1);
            deleteList(temp1);
        }
        if(temp2){
            assert(getSize(temp2)==1);
            deleteList(temp2);
        }
    }
}END_TEST

START_TEST(test_cirular_list_insert){
    Node*head=createCircularHead(createStruct(1));
    for(int i=2;i<=size;i++){
        if(i%2)
            insertAfter(head, createHead(createStruct(i)));
        else
            insertBefore(head, createHead(createStruct(i)));
    }
    int i=0;
    assert(size==getSize(head));
    Node*n=head;
    FOR_EACH_CIRCULAR(n,assert(i++<size));
    assert(n==NULL);
    n=head;
    FOR_AT_MOST(n,size*10);
    assert(n);
    deleteList(head);
}END_TEST

START_TEST(test_swap){
    Node*n=createEmptyHead();
    insertHead(n, createStruct(1));
    insertHead(n, createStruct(2));

    assert(getIntValue(n)==2);
    assert(getIntValue(n->next)==1);
    swap(n->next,n);
    assert(getIntValue(n)==1);
    assert(getIntValue(n->next)==2);
    swap(n,n->next);
    assert(getIntValue(n)==2);
    assert(getIntValue(n->next)==1);
    deleteList(n);
}END_TEST




START_TEST(test_partition_right){
    Node*node=getNodeType(_i);
    int size=getSize(node);

    partitionLeft(node);
    assert(size==getSize(node));

    Node*next=node->next;
    partitionRight(node);
    assert(node->next==NULL);

    if(size){
        assert(getSize(next)==size-1);
        assert(getSize(node)==1);
    }
    if(next){
        assert(next->prev==NULL);
        assert(size-1==getSize(next));
    }
    deleteList(node);
    if(next)
        deleteList(next);
}END_TEST
START_TEST(test_partition_left){
    Node*node=getNodeType(_i);
    int size=getSize(node);
    partitionLeft(node);
    assert(size==getSize(node));
    Node*notFirst=node->next;
    if(notFirst){
        partitionLeft(notFirst);
        assert(notFirst->prev==NULL);
        assert(getSize(node)==1);
        assert(size-1==getSize(notFirst));
        deleteList(notFirst);
    }
    deleteList(node);
}END_TEST
START_TEST(test_delete_list){
    deleteList(getNodeType(_i));
}END_TEST
START_TEST(test_delete_node){
    Node*node=getNodeType(_i);
    int cirulcar=isCircular(node);
    int size=getSize(node);
    if(size){
        if(cirulcar){
            Node*next=node->next;
            Node*nextNext=next->next;
            deleteNode(node);
            deleteNode(next);
            if(size==2)
                node=next;
            else node=nextNext;
        }
        else{
            Node*pointer=node;
            deleteNode(node);
            deleteNode(node);
            assert(pointer==node);
        }
        assert(getSize(node)==size-2);
    }
    deleteList(node);

}END_TEST

START_TEST(test_destroy_list){
    Node*node=getNodeType(_i);
    Node*copy=createCopy(node);
    destroyList(node);
    deleteList(copy);
}END_TEST

#define SOFT_REMOVAL_SETUP Node*node=getNodeType(_i);int circular=isCircular(node);Node*copy=createCopy(node);
#define SOFT_TEAR_DOWN assert(circular==isCircular(node));deleteList(node);deleteList(copy);
START_TEST(test_clear_list){
    SOFT_REMOVAL_SETUP
    clearList(node);
    assert(getSize(node)==0);

    SOFT_TEAR_DOWN
}END_TEST
START_TEST(test_soft_delete){
    SOFT_REMOVAL_SETUP
    int size=getSize(node);
    if(size){
        if(circular){
            Node*next=node->next;
            Node*nextNext=next->next;
            softDeleteNode(node);
            softDeleteNode(next);
            if(size==2)
                node=next;
            else node=nextNext;
        }
        else{
            softDeleteNode(node);
            softDeleteNode(node);
        }
        assert(getSize(node)==size-2);
    }
    else
        softDeleteNode(node);

    clearList(node);
    SOFT_TEAR_DOWN
}END_TEST

START_TEST(test_shift_to_head){

    //should not crash
    Node* n=createEmptyHead();
    swap(n, n);
    assert(getSize(n)==0);
    assert(n->next==NULL);
    assert(n->prev==NULL);

    fillListLinkedList(n, size);
    shiftToHead(n, isInList(n, 2));
    assert(getIntValue(n)==2);
    shiftToHead(n, isInList(n, 2));
    assert(getIntValue(n)==2);

    deleteList(n);

}END_TEST


Suite *utilSuite() {
    Suite*s = suite_create("Util");

    TCase*tc_core = tcase_create("Basic Node");
    tcase_add_test(tc_core, test_create_head);
    tcase_add_test(tc_core, test_create_cirular_head);
    tcase_add_test(tc_core, test_single_node_insertion);
    tcase_add_test(tc_core, test_insert_unique);
    tcase_add_test(tc_core,test_cirular_insert_in_place);
    tcase_add_test(tc_core, test_create_list);
    suite_add_tcase(s, tc_core);


    tc_core = tcase_create("Removal");
    tcase_add_loop_test(tc_core, test_partition_right,0,nodeTypes);
    tcase_add_loop_test(tc_core, test_partition_left,0,nodeTypes);
    tcase_add_loop_test(tc_core, test_delete_list,0,nodeTypes);
    tcase_add_loop_test(tc_core, test_destroy_list,0,nodeTypes);
    tcase_add_loop_test(tc_core, test_delete_node,0,nodeTypes);
    suite_add_tcase(s, tc_core);

    tc_core = tcase_create("Soft Removal");
    tcase_add_loop_test(tc_core, test_clear_list,0,nodeTypes);
    tcase_add_loop_test(tc_core, test_soft_delete,0,nodeTypes);
    suite_add_tcase(s, tc_core);

    tc_core = tcase_create("Iteration");
    tcase_add_loop_test(tc_core, test_for_each,0,2);
    tcase_add_test(tc_core, test_for_at_most);
    tcase_add_test(tc_core, test_util_first);
    tcase_add_test(tc_core, test_iter_circular);
    tcase_add_test(tc_core, test_in_list);
    tcase_add_test(tc_core, test_get_last);
    tcase_add_test(tc_core, test_get_max);
    suite_add_tcase(s, tc_core);


    tc_core = tcase_create("MISC");
    tcase_add_test(tc_core, test_list_join);
    tcase_add_test(tc_core, test_cirular_list_insert);
    tcase_add_test(tc_core, test_swap);
    tcase_add_test(tc_core, test_shift_to_head);
    suite_add_tcase(s, tc_core);

    return s;
}
