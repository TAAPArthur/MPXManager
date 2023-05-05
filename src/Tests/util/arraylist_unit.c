#include "../../util/arraylist.h"
#include "../tester.h"
#include <assert.h>
#include <stdlib.h>

static int N = 100;
static ArrayList list;
static int* newInt(int i) {
    int* n = malloc(sizeof(int));
    *n = i;
    return n;
}
static void tearDown() {
    FOR_EACH_R(void*, p, &list) {
        free(p);
    }
}
SCUTEST_SET_ENV(NULL, tearDown);
SCUTEST(test_add_remove) {
    for(int i = 0; i < N; i++) {
        assert(list.size == i);
        int* n = malloc(sizeof(int));
        addElement(&list, n);
        assert(getData(&list)[i] == n);
    }
    assert(list.size == N);
    for(int i = 0; i < N; i++) {
        assert(list.size == N - i);
        free(removeIndex(&list, 0));
    }
}

SCUTEST(test_iter_empty) {
    FOR_EACH(int*, n, &list) {
        assert(0 && !n);
    }
    FOR_EACH_R(int*, n, &list) {
        assert(0 && !n);
    }
}
SCUTEST(test_iter) {
    for(int i = 0; i < N; i++) {
        assert(list.size == i);
        addElement(&list, newInt(i));
    }
    int i = 0;
    FOR_EACH(int*, n, &list) {
        assert(i++ == getIndex(&list, n, sizeof(int)));
    }
}

SCUTEST(test_get_element) {
    for(int i = 0; i < N; i++) {
        int* n = newInt(i);
        addElement(&list, n);
        assert(n == getElement(&list, i));
        assert(i == getIndex(&list, n, sizeof(int)));
        assert(n == findElement(&list, n, sizeof(int)));
        for(int j = 0; j < i; j++) {
            assertEquals(j, *(int*)getElement(&list, j));
        }
    }
}
SCUTEST(test_shift_head) {
    for(int i = 0; i < N; i++) {
        addElement(&list, newInt(i));
        shiftToHead(&list, i);
        assert(i == *(int*)getElement(&list, 0));
    }
}
SCUTEST(test_add_after) {
    for(int i = 0; i < N; i++) {
        addElementAt(&list, newInt(i), 0);
        assert(i == *(int*)getElement(&list, 0));
    }
}

SCUTEST(test_index) {
    for(int i = 0; i < N; i++) {
        assert(-1 == getIndex(&list, &i, sizeof(i)));
        addElement(&list, newInt(i));
        assert(i == getIndex(&list, &i, sizeof(i)));
    }
    for(int i = N - 1; i >= 0; i--)
        removeElement(&list, &i, sizeof(i));
}

SCUTEST(test_swap) {
    ArrayList other = {0};
    addElement(&other, newInt(0));
    addElement(&list, newInt(1));
    addElement(&list, newInt(2));
    swapElements(&other, 0, &list, 1);
    assert(*(int*)getElement(&other, 0) == 2);
    assert(*(int*)getElement(&list, 0) == 1);
    assert(*(int*)getElement(&list, 1) == 0);
    free(getElement(&other, 0));
    clearArray(&other);
}
SCUTEST(test_clear) {
    for(int i = 0; i < N; i++) {
        addElement(&list, &i);
    }
    clearArray(&list);
    assert(list.size == 0);
}
