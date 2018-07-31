#include "../UnitTests.h"
#include "../../mywm-structs.h"
#include "../../util.h"

WindowInfo* createStruct(int i){
    WindowInfo* t=malloc(sizeof(WindowInfo));
    if(i<=0)
        i=(long)t;
    t->id=i;
    return t;
}

int size=10;
Node* createLinkedListAssert(Node*n,int size,int start,int a){
    assert(getSize(n)==0);\
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
        WindowInfo*info=createStruct(i);
        insertHead(n, info);
        insertHead(n, info);
        assert(getIntValue(n)==i);
        if(i>1)
            assert(getIntValue(n->next)==i);
    }
    return n;
}
Node* createLinkedList(int size){
    return createLinkedListAssert(createEmptyHead(),size,1,0);
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
    assertEmptyHead(createEmptyHead());
    assertEmptyHead(createHead(NULL));
    void*p=createStruct(1);
    assert(createHead(p)!=NULL);
    assert(createHead(p)->value==p);
    assert(createHead(p)->next==NULL);
    assert(createHead(p)->prev==NULL);
}END_TEST

START_TEST(test_create_cirular_head){
    Node*head=createCircularHead(NULL);
    assert(getValue(head)==NULL);
    assert(!isNotEmpty(head));
    assert(getSize(head)==0);
    assert(head->next==head);
    assert(head->prev==head);
    head=createCircularHead(createStruct(1));
    assert(getValue(head));
    assert(isNotEmpty(head));
    assert(getSize(head)==1);
    assert(head->next==head);
    assert(head->prev==head);
}END_TEST

START_TEST(test_single_node_insertion){
    Node*n1=createHead(createStruct(1));

    int value=size+1;
    Node* singleNode=createHead(&value);
    int originalHeadValue=getIntValue(n1);

    insertBefore(n1, singleNode);
    //n's value should now be the head of the list;
    assert(getIntValue(n1)==value);
    assert(getIntValue(n1->next)==originalHeadValue);
    assert(getSize(n1)==2);

    int value2=size+2;
    singleNode=createHead(&value2);

    insertAfter(n1, singleNode);
    //head value should not have changed
    assert(getIntValue(n1)==value);
    assert(getIntValue(n1->next)==value2);
    assert(getSize(n1)==3);

}END_TEST

/**Teests get size, and insert head*/
START_TEST(test_create_list){
    createLinkedListAssert(createEmptyHead(),size, 1,1);
}END_TEST


/**
 * Tests FOR_EACH and FOR_EACH_REVERSED
 */
START_TEST(test_for_each){
    Node*head=createLinkedList(size);
    Node*position=head;
    FOR_EACH(head,
        assert(head==position);
        assert(head!=NULL);
        position=position->next;);
    assert(head==NULL);
    assert(NULL==position);
    head=getLast(createLinkedList(size));
    position=head;
    FOR_EACH_REVERSED(head,
            assert(head==position);
            assert(head!=NULL);
            position=position->prev;);
    assert(head==NULL);
    assert(NULL==position);
    //assert should never be reaaced
    head=createEmptyHead();
    FOR_EACH(head,assert(0));
    assert(head==NULL);
    head=createEmptyHead();
    FOR_EACH_REVERSED(head,assert(0));
    assert(head==NULL);
}END_TEST

START_TEST(test_for_at_most){


    Node*head=createEmptyHead();
    FOR_AT_MOST(head,1,assert(0));
    assert(head==NULL);
    head=createLinkedList(size);
    Node*position=head;
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


}END_TEST

START_TEST(test_util_first){
    Node*head=createEmptyHead();
    UNTIL_FIRST(head,1);
    assert(head==NULL);
    head=createEmptyHead(createStruct(1));
    int i=1;
    UNTIL_FIRST(head,i--);
    assert(i==1);

    head=createLinkedList(size);
    int headValue=getIntValue(head);
    UNTIL_FIRST(head,1);
    assert(getIntValue(head)==headValue);
    i=0;
    int nextHeadValue=getIntValue(head->next);
    UNTIL_FIRST(head,i++);
    assert(getIntValue(head)==nextHeadValue);
    UNTIL_FIRST(head,0);
    assert(head==NULL);

}END_TEST
START_TEST(test_iter_circular){
    Node*head=createLinkedListAssert(createCircularHead(NULL), size, 1, 1);
    Node*temp=head;
    UNTIL_FIRST(temp,0)
    assert(temp==NULL);
    temp=head;
    FOR_EACH_CIRCULAR(temp,)
    assert(temp==NULL);
}END_TEST

START_TEST(test_in_list){

    Node* n=createEmptyHead();
    for(int i=0;i<size;i++)
        assert(!isInList(n, i));

    n=createLinkedList(size);
    assert(!isInList(n, 0));
    for(int i=0;i<size;i++)
        assert(isInList(n, i+1));
}END_TEST
int assert_not_reached(){
    assert(0);
    return 0;
}
START_TEST(test_get_max){
    Node*head=createLinkedList(size);
    Node*node;
    node=createEmptyHead();
    GET_MAX(node,1,assert_not_reached());
    assert(node==NULL);
    for(int i=1;i<-size;i++){
        node=head;
        GET_MAX(node,getIntValue(node)<=i,getIntValue(node));
        assert(getIntValue(node)==i);
    }
}END_TEST
/**
 * Checks getLast
 * @param START_TEST(test_get_last)
 */
START_TEST(test_get_nth){
    Node* n=createEmptyHead();
    assert(getNth(n,0)==NULL);
    assert(getNth(n,1)==NULL);
    assert(getLast(n)==NULL);

    n=createHead(createStruct(1));
    assert(n==getNth(n,0));
    assert(NULL==getNth(n,1));
    assert(n==getLast(n));
    n=createLinkedList(size);
    Node*node=n;
    for(int i=0;i<size;i++){
        assert(getNth(n, i)==node);
        node=node->next;
    }
    //testing get last
    n=createLinkedList(size);
    Node*last=getLast(n);
    assert(last==getLast(n));
    assert(last->next==NULL);
    assert(last->prev!=NULL);
} END_TEST

/**
 * Test insertBefore and insertAfter
 */
START_TEST(test_list_join){
    for(int i=0;i<6;i++){
        Node*n1=createLinkedList(size);
        Node*n2=createLinkedListAssert(createEmptyHead(),size,size+1,0);
        assert(getSize(n1)==size);
        assert(getSize(n2)==size);

        int totalSize = size*2 - i/2;
        if(i==2 || i>=4)
            n2=n2->next;
        if(i==3 || i>=4)
            n1=n1->next;
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
    }
}END_TEST

START_TEST(test_cirular_insert_in_place){
    Node*head=createLinkedList(size);
    Node*fakeHead=head->next;
    assert(getSize(fakeHead)==size-1);
    for(int i=size+1;i<=size*2;i++){
        insertBefore(fakeHead, createHead(createStruct(i)));
        assert(getSize(fakeHead)==size-1);
        assert(getSize(head)==i);
    }
    assert(getIntValue(head->next)==size+1);
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
}END_TEST


START_TEST(test_delete_node){

    //when the list is empty don't crash
    deleteNode(createEmptyHead());

    Node*n=createLinkedList(size);
    for(int i=1;i<=size;i++){
        int v=*(int*)n->value;
        deleteNode(n);//remove head of list
        assert(isInList(n, v)==NULL);
    }
    //cannot delete head
    assert(n->value==NULL);
}END_TEST
START_TEST(test_destroy_list){

    //when the list is empty don't crash
    destroyList(createEmptyHead());
    Node*n=createRedundentLinkedList(size);
    //fill error if removes values
    destroyList(n->next);
    //head should be untouched

    assert(getValue(n)!=NULL);
}END_TEST

START_TEST(test_clear_list){

    //when the list is empty don't crash
    clearList(createEmptyHead());
    Node*n=createRedundentLinkedList(size);
    //fill error if removes values
    clearList(n->next);
    assert(getValue(n->next)==NULL);
    assert(getValue(n)!=NULL);
}END_TEST

START_TEST(test_remove_by_value){

    Node*n=createLinkedList(size);
    for(int i=1;i<=size;i++){
        int v=*(int*)n->value;
        assert(removeByValue(n, v)==1);//remove head of list
        assert(isInList(n, v)==0);
    }
    //cannot delete head
    assert(n->value==NULL);
}END_TEST


START_TEST(test_pop_node){
    Node*node=createLinkedList(size);
    assert(popNode(createEmptyHead())==NULL);
    popNode(node->next);
    assert(getSize(node)==size-1);
    assert(node->next->prev==node);

}END_TEST
/**
 * Tests GET_MAX and shiftToHead
 * @param START_TEST(test_swap_head)
 */
START_TEST(test_shift_to_head){

    //should not crash
    Node* n=createEmptyHead();
    shiftToHead(n, n);
    assert(getSize(n)==0);
    assert(n->next==NULL);
    assert(n->prev==NULL);

    for(int i=1;i<=size;i++)
        insertHead(n, createStruct(i));

    Node*head=n;
    for(int i=1;i<=size;i++){
        Node*max=head;
        GET_MAX(max,1,getIntValue(max))

        assert(max!=NULL);

        int headValue=getIntValue(head);
        int nonheadValue=getIntValue(max);
        assert(headValue<=nonheadValue);
        shiftToHead(head, max);
        assert(getSize(n)==size);
        if(head->next)
            assert(head->next->prev==head);
        if(headValue!=nonheadValue){
            assert(getIntValue(head->next)==headValue);
            assert(getIntValue(head)==nonheadValue);
        }
        head=head->next;
    }

}END_TEST


START_TEST(test_preserve_value){
    assert(size>1);
    WindowInfo* arr[size];
    int count=0;
    int func(Node *node,int i){
        arr[getIntValue(node)-1]=(WindowInfo*)node->value;
        count++;
        return 0;
    }

    for(int n=0;n<3;n++){
        count=0;
        Node*node=createLinkedList(size);
        Node*pointer=node;
        FOR_EACH(pointer,
                arr[getIntValue(pointer)-1]=getValue(pointer);count++;
            );
        assert(count==size);
        if(n==0)
            destroyList(node->next);
        else if(n==1){
            clearList(node);
            assert(getSize(node)==0);
            assert(NULL==getValue(node));
            assert(NULL==node->next);
        }
        else for(int i=0;i<size;i++){
            Node*popped=popNode(node);
            assert(getSize(node)==size-i-1);
            if(popped){
                assert(popped->next==NULL);
                assert(popped->prev==NULL);
            }
        }
        for(int i=0;i<size;i++)
            assert((int)arr[i]->id==i+1);
    }

} END_TEST


Suite *utilSuite() {
    Suite*s = suite_create("Util");

    TCase*tc_core = tcase_create("Basic Node");
    tcase_add_test(tc_core, test_create_head);
    tcase_add_test(tc_core, test_create_cirular_head);
    tcase_add_test(tc_core, test_single_node_insertion);
    tcase_add_test(tc_core,test_cirular_insert_in_place);
    tcase_add_test(tc_core, test_create_list);
    suite_add_tcase(s, tc_core);

    tc_core = tcase_create("Iteration");
    tcase_add_test(tc_core, test_for_each);
    tcase_add_test(tc_core, test_for_at_most);
    tcase_add_test(tc_core, test_util_first);
    tcase_add_test(tc_core, test_iter_circular);
    tcase_add_test(tc_core, test_in_list);
    tcase_add_test(tc_core, test_get_nth);
    tcase_add_test(tc_core, test_get_max);
    suite_add_tcase(s, tc_core);


    tc_core = tcase_create("MISC");
    tcase_add_test(tc_core, test_list_join);
    tcase_add_test(tc_core, test_cirular_list_insert);

    tcase_add_test(tc_core, test_swap);
    tcase_add_test(tc_core, test_shift_to_head);
    suite_add_tcase(s, tc_core);

    tc_core = tcase_create("Removal");
    tcase_add_test(tc_core, test_destroy_list);
    tcase_add_test(tc_core, test_clear_list);
    tcase_add_test(tc_core, test_pop_node);
    tcase_add_test(tc_core, test_delete_node);
    tcase_add_test(tc_core, test_remove_by_value);
    tcase_add_test(tc_core, test_preserve_value);
    suite_add_tcase(s, tc_core);

    return s;
}
