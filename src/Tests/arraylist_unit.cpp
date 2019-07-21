#include "tester.h"
#include "../arraylist.h"


int* createStruct(int i) {
    return new int(i);
}
static ArrayList<int*> list;
const int size = 32;
void setup(void) {
    assert(list.size() == 0);
    for(int i = 0; i < size; i++)
        list.push_back(createStruct(i));
    assert(list.size() == size);
}
void teardown(void) {
    list.deleteElements();
}
struct Temp {
    int i = 0;
    operator int()const {return i;}
    bool operator ==(Temp& t)const {return this->i == t.i;}
};
struct TempEquals: Temp {
    bool operator ==(TempEquals& t)const {return this->i == t.i;}
};
static_assert(std::is_convertible<Temp, int>::value, "not an int");
static_assert(std::is_convertible<std::remove_pointer<Temp>::type, int>::value);
static_assert(std::is_convertible<std::remove_pointer<Temp*>::type, int>::value);
static_assert(std::is_convertible<std::remove_pointer_t<Temp*>, int>::value);

MPX_TEST("enable_if", {
    ArrayList<int> list;
    list.add(1);
    list.add(2);
    assert(list.find(1));
    assert(list.find(2));
});
MPX_TEST("enable_if_struct", {
    ArrayList<Temp*> list;
    list.add(new Temp{1});
    list.add(new Temp{2});
    assert(list.find(1));
    assert(list.find(2));
    list.deleteElements();
});
MPX_TEST("enable_if_struct", {
    ArrayList<TempEquals*> list;
    list.add(new TempEquals{1});
    list.add(new TempEquals{2});
    assert(list.find(1));
    assert(list.find(2));
    list.deleteElements();
});
MPX_TEST_ITER("init_list", 2, {
    ArrayList<Temp*> list;
    if(_i == 0)
        list = ArrayList<Temp*>({{1}, {2}, {3}});
    else if(_i == 1)
        list = {new Temp{1}, new Temp{2}, new Temp{3}};
    list.deleteElements();
});

SET_ENV(setup, teardown);

MPX_TEST("template_int", {
    static ArrayList<unsigned int>intList;
    auto lambda = [](int i) {intList.add(i);};
    for(int i = 0; i < size; i++)
        lambda(i * 3);
    for(int i = 0; i < size; i++)
        assert(intList[i] == i * 3);
});

MPX_TEST("init", {assert(list.size() == size);})
MPX_TEST("add", {
    for(int n = 0; n < 10; n++) {
        assert(list.size() == size + n);
        list.add(createStruct(n));
        assert(*list[size + n] == n);
    }
});
MPX_TEST("add_flag", {
    list.deleteElements();
    int* p = createStruct(1);
    assert(list.add(*p, ADD_ALWAYS));
    assert(list.add(*p, ADD_ALWAYS));
    assertEquals(list.size(), 2);
    assert(!list.add(*p, ADD_UNIQUE));
    assert(!list.add(*p, ADD_REMOVE));
    assert(!list.add(*p, ADD_REMOVE));
    assertEquals(list.size(), 0);
    assert(list.add(*p, ADD_UNIQUE));
    assert(!list.add(*p, ADD_TOGGLE));
    assert(list.add(*p, ADD_TOGGLE));
    assert(list.size() == 1);
    list.deleteElements();
    delete p;
});
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
