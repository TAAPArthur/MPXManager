#include "../UnitTests.h"
#include "../../util.h"

int* createStruct(int i){
    int* t = malloc(sizeof(int));
    *t = i;
    return t;
}

static ArrayList list = {0, .offset = 1};
const int size = 10;
void populateList(void){
    for(int i = 0; i < size; i++)
        addToList(&list, createStruct(i));
    assert(getSize(&list) == size);
}
void freeListMem(void){
    deleteList(&list);
    assert(getSize(&list) == 0);
}

START_TEST(test_arraylist){
    ArrayList list = {0};
    int size = 10;
    for(int n = 0; n < 5; n++){
        if(n < 3)
            for(int i = 0; i < size; i++){
                if(n == 2)
                    addUnique(&list, createStruct(i), sizeof(int));
                else addToList(&list, createStruct(i));
                assert(isNotEmpty(&list));
                assert(*(int*)getHead(&list) == 0);
            }
        else
            for(int i = size - 1; i >= 0; i--){
                prependToList(&list, createStruct(i));
                assert(isNotEmpty(&list));
                assert(*(int*)getHead(&list) == i);
            }
        assert(getSize(&list) == size);
        for(int i = 0; i < size; i++){
            assert(!addUnique(&list, &i, sizeof(int)));
            assert(getSize(&list) == size);
        }
        for(int i = 0; i < size; i++)
            assert(*(int*)getElement(&list, i) == i);
        deleteList(&list);
        assert(getSize(&list) == 0);
        assert(!isNotEmpty(&list));
    }
}
END_TEST

START_TEST(test_arraylist_remove){
    for(int i = 0; i < getSize(&list); i++)
        free(removeFromList(&list, i));
    for(int i = 0; i < getSize(&list); i++)
        assert(*(int*)getElement(&list, i) == i * 2 + 1);
}
END_TEST
START_TEST(test_arraylist_remove_value){
    int size = getSize(&list);
    for(int i = 0; i < size; i++){
        void* p = removeElementFromList(&list, &i, sizeof(int));
        assert(p);
        free(p);
        assert(!removeElementFromList(&list, &i, sizeof(int)));
    }
    assert(getSize(&list) == 0);
}
END_TEST
START_TEST(test_arraylist_pop){
    for(int i = size - 1; i >= 0; i--){
        void* p = pop(&list);
        assert(*(int*)p == i);
        free(p);
    }
    assert(!isNotEmpty(&list));
}
END_TEST

START_TEST(test_get_last){
    assert(*(int*)getLast(&list) == size - 1);
}
END_TEST

START_TEST(test_indexof){
    for(int i = 0; i < getSize(&list); i++)
        assert(indexOf(&list, &i, sizeof(int)) == i);
    for(int i = 0; i < getSize(&list); i++)
        assert(*(int*)find(&list, &i, sizeof(int)) == i);
    int outOfRange = size;
    assert(indexOf(&list, &outOfRange, sizeof(int)) == -1);
    assert(find(&list, &outOfRange, sizeof(int)) == NULL);
}
END_TEST
START_TEST(test_indexof_ptr){
    freeListMem();
    for(int i = 0; i < size; i++)
        addToList(&list, (void*)(long)i);
    for(int i = 0; i < getSize(&list); i++)
        assert(indexOf(&list, (void*)(long)i, 0) == i);
    clearList(&list);
}
END_TEST

START_TEST(test_swap){
    int maxIndex = getSize(&list) - 1;
    for(int i = 0; i < getSize(&list); i++){
        swap(&list, i, maxIndex);
        assert(indexOf(&list, &i, sizeof(int)) == maxIndex);
        swap(&list, i, maxIndex);
        assert(indexOf(&list, &i, sizeof(int)) == i);
    }
}
END_TEST

START_TEST(test_get_next_index){
    assert(getNextIndex(&list, 0, -1) == getSize(&list) - 1);
    assert(getNextIndex(&list, 0, getSize(&list) - 1) == getSize(&list) - 1);
    assert(getNextIndex(&list, 0, 0) == 0);
    assert(getNextIndex(&list, 0, getSize(&list)) == 0);
    assert(getNextIndex(&list, 0, -getSize(&list)) == 0);
    for(int i = 0; i < getSize(&list) * 2; i++){
        assert(getNextIndex(&list, 0, i) == i % getSize(&list));
    }
}
END_TEST

START_TEST(test_shift_to_head){
    for(int i = 0; i < getSize(&list); i++)
        shiftToHead(&list, i);
    for(int i = 0; i < size; i++)
        assert(*(int*)getElement(&list, i) == size - i - 1);
}
END_TEST

START_TEST(test_get_time){
    unsigned int time = getTime();
    int delay = 11;
    for(int i = 0; i < 100; i++){
        msleep(delay);
        assert(time + delay <= getTime());
        time = getTime();
    }
}
END_TEST
START_TEST(test_min_max){
    int big = 10, small = 1;
    assert(MAX(big, small) == big);
    assert(MIN(big, small) == small);
}
END_TEST
START_TEST(test_wrap){
    for(int i = 0; i < 2; i++){
        int len = 7;
        int offset = len * i;
        assert(WRAP(8 + offset, len) == 1);
        assert(WRAP(len + offset, len) == 0);
        assert(WRAP(0, len) == 0);
        assert(WRAP(-1 + offset, len) == 6);
        assert(WRAP(-len + offset, len) == 0);
    }
}
END_TEST

START_TEST(test_for_each){
    int i = 0;
    FOR_EACH(int*, value, &list){
        assert(value == getElement(&list, i++));
    }
    i = 0;
    FOR_EACH(int*, value, &list){
        assert(value == getElement(&list, i++));
    }
}
END_TEST
START_TEST(test_for_each_nested){
    int i = 0;
    FOR_EACH(int*, value, &list){
        assert(value == getElement(&list, i++));
        int i = 0;
        FOR_EACH(int*, value, &list){
            assert(value == getElement(&list, i++));
        }
    }
}
END_TEST
START_TEST(test_for_each_reversed){
    int i = getSize(&list) - 1;
    FOR_EACH_REVERSED(int*, value, &list){
        assert(value == getElement(&list, i--));
    }
}
END_TEST

START_TEST(test_offset){
    freeListMem();
    setOffset(&list, 100);
    populateList();
    test_for_each(0);
    for(int i = 1; i < getOffset(&list); i++){
        setElement(&list, -i, (void*)(long)i);
        assert(getElement(&list, -i) == (void*)(long)i);
    }
    freeListMem();
}
END_TEST

START_TEST(test_add_many_to_list){
    ArrayList temp = {0};
    int data[size];
    for(int i = 0; i < LEN(data); i++)
        data[i] = (i);
    addManyToList(&temp, data, LEN(data), sizeof(data[0]));
    for(int i = 0; i < LEN(data); i++)
        assert(*(int*)getElement(&temp, i) == data[i]);
    addManyToList(&temp, &list.arr[getOffset(&list)], getSize(&list), sizeof(void*));
    assert(getSize(&temp) == 2 * getSize(&list));
    clearList(&temp);
}
END_TEST

Suite* utilSuite(){
    Suite* s = suite_create("Util");
    TCase* tc_core;
    tc_core = tcase_create("ArrayList");
    tcase_add_checked_fixture(tc_core, populateList, freeListMem);
    tcase_add_test(tc_core, test_arraylist);
    tcase_add_test(tc_core, test_indexof);
    tcase_add_test(tc_core, test_indexof_ptr);
    tcase_add_test(tc_core, test_swap);
    tcase_add_test(tc_core, test_get_next_index);
    tcase_add_test(tc_core, test_arraylist_remove);
    tcase_add_test(tc_core, test_arraylist_remove_value);
    tcase_add_test(tc_core, test_arraylist_pop);
    tcase_add_test(tc_core, test_get_last);
    tcase_add_test(tc_core, test_shift_to_head);
    tcase_add_test(tc_core, test_add_many_to_list);
    suite_add_tcase(s, tc_core);
    tc_core = tcase_create("Iter");
    tcase_add_checked_fixture(tc_core, populateList, freeListMem);
    tcase_add_test(tc_core, test_for_each);
    tcase_add_test(tc_core, test_for_each_nested);
    tcase_add_test(tc_core, test_for_each_reversed);
    suite_add_tcase(s, tc_core);
    tc_core = tcase_create("Offset");
    tcase_add_checked_fixture(tc_core, NULL, freeListMem);
    tcase_add_test(tc_core, test_offset);
    suite_add_tcase(s, tc_core);
    tc_core = tcase_create("MISC");
    tcase_add_test(tc_core, test_get_time);
    tcase_add_test(tc_core, test_min_max);
    tcase_add_test(tc_core, test_wrap);
    suite_add_tcase(s, tc_core);
    return s;
}
