/**
 * @file monitors.h
 * Contains methods related to the Monitor struct
 */

#ifndef MPX_RECT_H_
#define MPX_RECT_H_
#include <iostream>
#include <assert.h>
/**
 * holds top-left coordinates and width/height of the bounding box
 */
struct Rect {
    /// top left coordinate of the bounding box
    short x = 0;
    /// top left coordinate of the bounding box
    short y = 0;
    /// width of the bounding box
    uint16_t width = 0;
    /// height of the bounding box
    uint16_t height = 0;
    Rect(const short* p): x(p[0]), y(p[1]), width(p[2]), height(p[3]) {}
    Rect(short x, short y, uint16_t w, uint16_t h): x(x), y(y), width(w), height(h) {}
    short& operator[](int i) {
        return (&x)[i];
    }
    operator const short* () const {
        return &x;
    }
    bool operator!=(const Rect& r) const {
        return !(*this == r);
    }
    bool operator==(const Rect& r) const {
        for(int i = 0; i < 4; i++)
            if((*this)[i] != r[i])
                return 0;
        return 1;
    }
    void copyTo(int* arr)const {
        for(int i = 0; i < 4; i++)
            arr[i] = (*this)[i];
    }
    /**
     * Tests to see if the area of arg overlaps with this rect
     * @return 1 iff the this intersects arg
     */
    bool intersects(Rect arg) const;
    /// returns the area of this rect
    int getArea() const {return width * height;}
    /**
     *
     * Tests to see if the this rect's area is larger than arg's
     * @param arg
     * @return 1 if this has a larger area
     */
    bool isLargerThan(Rect arg) const;
    /**
     * Tests to see if the area of arg is completely contained in this rect
     * @param arg
     *
     * @return 1 iff this completely contains arg
     */
    bool contains(Rect arg)const;
    /**
     * Tests to see if the area of arg is completely contained in this rect
     * @param arg
     *
     * @return 1 iff this completely contains arg
     */
    bool containsProper(Rect arg)const;
};
std::ostream& operator<<(std::ostream&, const Rect&);

struct RectWithBorder : Rect {
    short border;
    RectWithBorder(): Rect(0, 0, 0, 0), border(0) {}
    RectWithBorder(const short* p): Rect(p), border(p[4]) {}
    RectWithBorder(short x, short y, uint16_t w, uint16_t  h, uint16_t b = 0): Rect(x, y, w, h), border(b) {}
    bool operator==(const Rect& r) const {
        for(int i = 0; i < 4; i++)
            if((*this)[i] != r[i])
                return 0;
        return 1;
    }
    void copyTo(int* arr)const {
        for(int i = 0; i < 5; i++)
            arr[i] = (*this)[i];
    }
};
#endif
