/**
 * @file
 * Contains methods related to the Monitor struct
 */

#ifndef MPX_RECT_H_
#define MPX_RECT_H_
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/**
 * holds top-left coordinates and width/height of the bounding box
 */
typedef struct Rect {
    /// top left coordinate of the bounding box
    int16_t x;
    /// top left coordinate of the bounding box
    int16_t y;
    /// width of the bounding box
    uint16_t width;
    /// height of the bounding box
    uint16_t height;
} Rect;
static inline bool isRectEqual(Rect a, Rect b) {
    return memcmp(&a, &b, sizeof(Rect)) == 0;
}

/**
 * Copies the values of this rect into arr
 *
 * @param arr
 */
static inline void copyTo(const Rect* rect, bool includeBorder, uint32_t* arr) {
    for(int i = 0; i < 4 + includeBorder; i++)
        arr[i] = ((short*)rect)[i];
}
/**
 * Checks to if two lines intersect
 * (either both vertical or both horizontal
 * @param P1 starting point 1
 * @param D1 displacement 1
 * @param P2 staring point 2
 * @param D2 displacement 2
 * @return 1 iff the line intersect
 */
static inline int intersects1D(int P1, int D1, int P2, int D2) {
    return P1 < P2 + D2 && P1 + D1 > P2;
}

/**
 * Tests to see if the area of arg overlaps with this rect
 * @return 1 iff the this intersects arg
 */
static inline bool intersects(Rect rect, Rect arg) {
    return (intersects1D(rect.x, rect.width, arg.x, arg.width) &&
            intersects1D(rect.y, rect.height, arg.y, arg.height));
}
/**
 * Tests to see if the area of arg is completely contained in this rect
 * @param arg
 *
 * @return 1 iff this completely contains arg
 */
static inline bool contains(Rect rect, Rect arg) {
    return (rect.x <= arg.x && arg.x + arg.width <=  rect.x + rect.width &&
            rect.y <= arg.y && arg.y + arg.height <= rect.y + rect.height);
}
/**
 * Tests to see if the area of arg is completely contained in this rect
 * @param rect
 * @param arg
 *
 * @return 1 iff this completely contains arg
 */
static inline bool containsProper(Rect rect, Rect arg) {
    return (rect.x < arg.x && arg.x + arg.width <  rect.x + rect.width &&
            rect.y < arg.y && arg.y + arg.height < rect.y + rect.height);
}

static inline uint32_t getArea(Rect rect) {
    return rect.width * rect.height;
}
#endif
