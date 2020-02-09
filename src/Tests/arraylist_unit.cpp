#include "../arraylist.h"
#include "tester.h"

MPX_TEST("enable_if", {
    ArrayList<int> list;
    list.add(1);
    list.add(2);
    assert(list.find(1));
    assert(list.find(2));
    assert(!list.find(3));
});
MPX_TEST_ITER("init_list", 2, {
    ArrayList<int*> list;
    if(_i == 0)
        list = ArrayList<int*>({1, 2, 3});
    else if(_i == 1)
        list = {new int{1}, new int{2}, new int{3}};
    list.deleteElements();
});
MPX_TEST("_just_for_code_coverage", {
#define _TOUCH(T){ T L;L.begin();L.end();L.rbegin();L.rend();}
    _TOUCH(ArrayList<int>);
    _TOUCH(ArrayList<int*>);
});
MPX_TEST("sort", {
    ArrayList<int*> list;
    list.add(new int(-1));
    for(int i = 10; i >= 0; i--)
        list.add(new int(i));
    list.sort();
    for(int i = -1; i < list.size(); i++)
        assertEquals(*list[i], i);
    list.deleteElements();
});
MPX_TEST("template_int", {
    ArrayList<int>intList;
    for(int i = 0; i < 3; i++)
        intList.add(i * 3);
    int i = 0;
    for(auto n : intList)
        assertEquals(n, i++ * 3);
});
MPX_TEST("equality_pointer", {
    ArrayList<int*> list1;
    ArrayList<int*> list2;
    list1.add(new int(1));
    assert(list1 != list2);
    list2.add(new int(1));
    assertEquals(list1, list2);
    list1.deleteElements();
    list2.deleteElements();
});
MPX_TEST("equality_nonpointer", {
    ArrayList<int> list1;
    ArrayList<int> list2;
    list1.add(1);
    assert(list1 != list2);
    list2.add(1);
    assertEquals(list1, list2);
});

int* createStruct(int i) {
    return new int(i);
}
ArrayList<int*> list;
const int size = 32;
void setup(void) {
    assertEquals(list.size(), 0);
    for(int i = 0; i < size; i++)
        list.push_back(createStruct(i));
    assertEquals(list.size(), size);
}
void teardown(void) {
    list.deleteElements();
}
SET_ENV(setup, teardown);

MPX_TEST("equals_ptr", {
    ArrayList<int*> list2(list);
    assertEquals(list2, list);
    list2.add(new int(1));
    assert(list2 != list);
    assertAndDelete(list2.pop());
    assertEquals(list2, list);
    list2.pop();
    assert(list2 != list);
    list2.add(new int(-11));
    assert(list2 != list);
    assertAndDelete(list2.pop());
})

MPX_TEST("add", {
    for(int n = 0; n < 10; n++) {
        assertEquals(list.size(), size + n);
        list.add(createStruct(n));
        assertEquals(*list[size + n], n);
    }
});
MPX_TEST("add_flag", {
    list.deleteElements();
    int* p = createStruct(1);
    assert(list.add(*p, ADD_ALWAYS));
    assert(list.add(*p, ADD_ALWAYS));
    assert(list.add(*p, PREPEND_ALWAYS));
    assertEquals(list.size(), 3);
    assert(!list.add(*p, ADD_TOGGLE));
    assertEquals(list.size(), 2);
    assert(!list.add(*p, ADD_UNIQUE));
    assert(!list.add(*p, PREPEND_UNIQUE));
    assertEquals(list.size(), 2);
    assert(!list.add(*p, ADD_REMOVE));
    assertEquals(list.size(), 1);
    assert(!list.add(*p, ADD_REMOVE));
    assertEquals(list.size(), 0);
    assert(list.add(*p, ADD_UNIQUE));
    assert(!list.add(*p, ADD_TOGGLE));
    assert(list.add(*p, ADD_TOGGLE));
    assert(list.size() == 1);
    assert(!list.add(*p, ADD_TOGGLE));
    assert(list.size() == 0);
    assert(list.add(*p, PREPEND_UNIQUE));
    list.deleteElements();
    delete p;
});
MPX_TEST("add_flag2", {
    list.deleteElements();
    assert(list.add(2, ADD_ALWAYS));
    assert(list.add(1, PREPEND_UNIQUE));
    assert(list.add(3, ADD_UNIQUE));

    assert(list.add(1, ADD_ALWAYS));
    assert(!list.add(1, ADD_REMOVE));

    assert(list.add(4, PREPEND_ALWAYS));
    assert(!list.add(4, PREPEND_TOGGLE));
    assert(list.add(4, ADD_TOGGLE));

    assert(list.add(0, PREPEND_TOGGLE));
    assert(!list.add(0, PREPEND_TOGGLE));
    assert(list.add(0, ADD_REMOVE));
    for(int i = 0; i < list.size(); i++)
        assertEquals(*list[i], i + 1);
    list.deleteElements();
})
MPX_TEST("add_unique", {
    list.deleteElements();
    int* p = createStruct(1);
    int* p2 = createStruct(1);
    assert(list.addUnique(p));
    assert(!list.addUnique(p));
    assert(!list.addUnique(p2));
    assert(list.size() == 1);
    list.deleteElements();
    delete p2;
});
MPX_TEST("delete_elements", {
    list.deleteElements();
    list.clear();
    assert(list.size() == 0);
});
MPX_TEST_ITER("find", 2, {
    for(int i = 0; i < list.size(); i++)
        assert((_i ? *list.find(i) : *list.find(&i)) == i);

    assert(list.find(list.size()) == NULL);
});
MPX_TEST("get_next_index", {
    assert(list.getNextIndex(0, -1) == list.size() - 1);
    assert(list.getNextIndex(0, list.size() - 1) == list.size() - 1);
    assert(list.getNextIndex(0, 0) == 0);
    assert(list.getNextIndex(0, list.size()) == 0);
    assert(list.getNextIndex(0, -list.size()) == 0);
    for(int i = 0; i < list.size() * 2; i++) {
        assert(list.getNextIndex(0, i) == i % list.size());
    }
});
MPX_TEST_ITER("indexof", 2, {
    for(int i = 0; i < list.size(); i++)
        assert((_i ? list.indexOf(i) : list.indexOf(&i)) == i);
    assert(list.indexOf(list.size()) == -1);
});
MPX_TEST("search_empty", {
    ArrayList<int*>empty;
    int i = 0;
    assert(empty.find(i) == NULL);
    assert(empty.find(&i) == NULL);
    assert(empty.indexOf(i) == -1);
    assert(empty.indexOf(&i) == -1);
});

MPX_TEST("pop", {
    for(int i = size - 1; i >= 0; i--) {
        int* p = list.pop();
        assert(*(int*)p == i);
        delete p;
    }
    assert(list.empty());
});
MPX_TEST("remove_from_list_alt", {
    while(list.size()) {
        int* p = list.remove(list.size() % 2 ? 0 : list.size() - 1);
        assert(p);
        delete p;
    }
});
MPX_TEST("remove_from_list_middle", {
    int index = list.size() / 2;
    int* p2 = list[index + 1];
    int* p = list.remove(index);
    assert(p);
    delete p;
    assert(p2 == list[index]);
});
MPX_TEST_ITER("remove_from_list", 2, {
    if(_i) {
        for(int i = list.size() - 1; i >= 0; i--) {
            int* p = list.remove(i);
            assert(p);
            assert(i == list.size());
            assert(list.find(i) == NULL);
            delete p;
        }
    }
    else
        for(int i = 0; i < size; i++) {
            int* p = list.remove(0);
            assert(p);
            assert(size - i - 1 == list.size());
            assert(list.find(0U) == NULL);
            delete p;
        }
});

MPX_TEST_ITER("remove_element_from_list", 4, {
    for(int i = 0; i < size; i++) {
        int index = _i % 2 ? i : size - i - 1;
        int* p = _i < 2 ? list.removeElement(&index): list.removeElement(index);
        assert(p);
        assert(list.indexOf(&index) == -1);
        assert(list.find(&index) == 0);
        assert(!list.removeElement(&index));
        delete p;
    }
    assert(list.size() == 0);
});
MPX_TEST("shift_to_head", {
    for(int i = 0; i < list.size(); i++)
        list.shiftToHead(i);
    for(int i = 0; i < size; i++)
        assert(*list[ i] == size - i - 1);
});

MPX_TEST("swap", {
    int maxIndex = list.size() - 1;
    for(int i = 0; i < list.size(); i++) {
        list.swap(i, maxIndex);
        assert(list.indexOf(&i) == maxIndex);
        list.swap(i, maxIndex);
        assert(list.indexOf(&i) == i);
    }
});
MPX_TEST("iter", {
    int i = *list[0];
    ReverseArrayList<int*> rList;
    for(auto p : list) {
        assertEquals(*p, i++);
        rList.add(p);
    }
    for(auto p : rList)
        assertEquals(*p, --i);
    for(auto iter = rList.rbegin(); iter != rList.rend(); ++iter)
        assertEquals(**iter, i++);
});
MPX_TEST("add_flags", {
});
