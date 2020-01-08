/**
 * @file
 * Contains methods related to the Monitor struct
 */

#ifndef MPX_RECT_H_
#define MPX_RECT_H_
#include <iostream>
#include <assert.h>
/**
 * Helper class for Rect
 */
struct Point {
    /// x coordinate
    const int32_t x;
    /// y coordinate
    const int32_t y;
    /**
     * @param p
     * @return a new Point with the sum of this and p
     */
    Point operator+(const Point p) {
        return {x + p.x, y + p.y};
    }
    /**
     * @param p
     * @return a new Point with the delta between this and p
     */
    Point operator-(const Point p) {
        return {x - p.x, y - p.y};
    }
};
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
    /**
     * Reads the first 4 values and uses them to construct the rect
     * @param p
     */
    Rect(const short* p): x(p[0]), y(p[1]), width(p[2]), height(p[3]) {}
    /**
     * Reads the first 4 values and uses them to construct the rect
     * @param p
     */
    Rect(const uint32_t* p): x(p[0]), y(p[1]), width(p[2]), height(p[3]) {}
    /**
     * @param x
     * @param y
     * @param w
     * @param h
     */
    Rect(short x, short y, uint16_t w, uint16_t h): x(x), y(y), width(w), height(h) {}

    /**
     * @return returns the top left point of bounded by this rect
     */
    const Point getTopLeftCorner() const {
        return {x, y};
    }

    /// @return getTopLeftCorner()
    operator const Point() const {
        return getTopLeftCorner();
    }

    /**
     * Allows access to this struct as if it was a short*
     *
     * @param i
     *
     * @return
     */
    short& operator[](int i) {
        return (&x)[i];
    }
    /**
     * Returns an array of x, y, width and height;
     *
     * @return
     */
    operator const short* () const {
        return &x;
    }
    /**
     * @param r
     * @return
     */
    bool operator!=(const Rect& r) const {
        return !(*this == r);
    }
    /**
     * Checks to see if the size and position is the same
     *
     * @param r
     *
     * @return
     */
    bool operator==(const Rect& r) const {
        for(int i = 0; i < 4; i++)
            if((*this)[i] != r[i])
                return 0;
        return 1;
    }
    /**
     * Copies the values of this rect into arr
     *
     * @param arr
     */
    void copyTo(uint32_t* arr)const {
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

    /**
     * @return  1 iff x and y are 0
     */
    bool isAtOrigin() const {
        return x == 0 && y == 0;
    }
    /**
     * Computes the delta between our position and currentRef and adds it to newRef
     * and sets this as our new position
     *
     * @param currentRef
     * @param newRef
     */
    void translate(const Point currentRef, const Point newRef);
};

/**
 * holds top-left coordinates and width/height of the bounding box along with a border
 * This struct mirrors XWindow geometry
 */
struct RectWithBorder : Rect {
    /// The border of this rect
    short border;
    /**
     * Creates an empty rect
     */
    RectWithBorder(): Rect(0, 0, 0, 0), border(0) {}
    /**
     * Copy-construct r with a border of size 0
     */
    RectWithBorder(const Rect& r): Rect(r), border(0) {}
    /**
     *
     *
     * @param p
     */
    RectWithBorder(const short* p): Rect(p), border(p[4]) {}
    /**
     *
     *
     * @param x
     * @param y
     * @param w
     * @param h
     * @param b
     */
    RectWithBorder(short x, short y, uint16_t w, uint16_t h, uint16_t b = 0): Rect(x, y, w, h), border(b) {}
    /**
     * Copies the values of this rect into arr
     *
     * @param arr
     */
    void copyTo(uint32_t* arr)const {
        for(int i = 0; i < 5; i++)
            arr[i] = (*this)[i];
    }
};
/**
 * Prints Rect
 * @return
 */
std::ostream& operator<<(std::ostream&, const RectWithBorder& );
/**
 * Prints Rect
 * @return
 */
std::ostream& operator<<(std::ostream&, const Rect&);
#endif
