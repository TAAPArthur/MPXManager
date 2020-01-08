#include "../rect.h"
#include "../system.h"
#include "tester.h"

#pragma GCC diagnostic ignored "-Wnarrowing"
static short baseArr[] = {1, 2, 3, 4};
MPX_TEST("print", {
    suppressOutput();
    Rect r = Rect(baseArr);
    std::cout << r << RectWithBorder(r);
});
MPX_TEST("rect_==", {
    Rect rects[] = {Rect(baseArr), Rect(baseArr[0], baseArr[1], baseArr[2], baseArr[3])};
    assertEquals(rects[0], rects[1]);
});
MPX_TEST("rect_[]", {
    Rect r = Rect(baseArr);
    for(int i = 0; i < LEN(baseArr); i++)
        assertEquals(baseArr[i], r[i]);
});
MPX_TEST("rect_copy_to", {
    uint32_t p2[4];
    RectWithBorder r = RectWithBorder(baseArr);
    r.copyTo(p2);
    for(int i = 0; i < LEN(baseArr); i++)
        assertEquals(p2[i], r[i]);
});
MPX_TEST("rect_is_larger", {
    assert(Rect(0, 0, 1, 1).isLargerThan(Rect(0, 0, 0, 0)));
    assert(!Rect(0, 0, 1, 1).isLargerThan(Rect(0, 0, 1, 1)));
    assert(!Rect(0, 0, 1, 1).isLargerThan(Rect(0, 0, 2, 1)));
});
MPX_TEST("rect_contains", {
    Rect large = {0, 0, 20, 20};
    Rect rect = {1, 1, 2, 2};
    assert(!rect.containsProper(rect));
    assert(rect.contains(rect));
    assert(large.containsProper(rect));
    assert(large.contains(rect));
    for(int x = 1; x <= 2; x++)
        for(int y = 1; y <= 2; y++) {
            Rect child = {x, y, 1, 1};
            assert(!rect.containsProper(child));
            assert(rect.contains(child));
            assert(!child.contains(rect));
            assert(!child.containsProper(rect));
        }
});
MPX_TEST("rect_intersection", {
    short dimX = 100;
    short dimY = 200;
    short offsetX = 10;
    short offsetY = 20;
    Rect rect = {offsetX, offsetY, dimX, dimY};
    assert(rect.intersects(rect));
    //easy to see if fail
    assert(!rect.intersects({ 0, 0, offsetX, offsetY}));
    assert(!rect.intersects({ 0, offsetY, offsetX, offsetY}));
    assert(!rect.intersects({ offsetX, 0, offsetX, offsetY}));
    for(short x = 0; x < dimX + offsetX * 2; x++) {
        assert(rect.intersects({ x, 0, offsetX, offsetY}) == 0);
        assert(rect.intersects({ x, offsetY + dimY, offsetX, offsetY}) == 0);
    }
    for(short y = 0; y < dimY + offsetY * 2; y++) {
        assert(rect.intersects({ 0, y, offsetX, offsetY}) == 0);
        assert(rect.intersects({ offsetX + dimX, y, offsetX, offsetY}) == 0);
    }
    assert(rect.intersects({ 0, offsetY, offsetX + 1, offsetY}));
    assert(rect.intersects({ offsetX + 1, offsetY, 0, offsetY}));
    assert(rect.intersects({ offsetX + 1, offsetY, dimX, offsetY}));
    assert(rect.intersects({ offsetX, 0, offsetX, offsetY + 1}));
    assert(rect.intersects({ offsetX, offsetY + 1, offsetX, 0}));
    assert(rect.intersects({ offsetX, offsetY + 1, offsetX, dimY}));
    assert(rect.intersects({ offsetX + dimX / 2, offsetY + dimY / 2, 0, 0}));
});
MPX_TEST("translate", {
    Rect rect = {0, 0, 100, 100};
    rect.translate({0, 0}, {100, 200});
    assertEquals(rect, Rect(100, 200, 100, 100));
});
