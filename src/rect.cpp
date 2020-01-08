#include "rect.h"

/**
 * Checks to if two lines intersect
 * (either both vertical or both horizontal
 * @param P1 starting point 1
 * @param D1 displacement 1
 * @param P2 staring point 2
 * @param D2 displacement 2
 * @return 1 iff the line intersect
 */
static int intersects1D(int P1, int D1, int P2, int D2) {
    return P1 < P2 + D2 && P1 + D1 > P2;
}

bool Rect::intersects(Rect arg)const {
    return (intersects1D(x, width, arg.x, arg.width) &&
            intersects1D(y, height, arg.y, arg.height));
}
bool Rect::isLargerThan(Rect arg)const {
    return getArea() > arg.getArea();
}
bool Rect::contains(Rect arg)const {
    return (x <= arg.x && arg.x + arg.width <= x + width &&
            y <= arg.y && arg.y + arg.height <= y + height);
}
bool Rect::containsProper(Rect arg)const {
    return (x < arg.x && arg.x + arg.width < x + width &&
            y < arg.y && arg.y + arg.height < y + height);
}
void Rect::translate(const Point currentOrigin, const Point newOrigin) {
    x = x - currentOrigin.x + newOrigin.x;
    y = y - currentOrigin.y + newOrigin.y;
}
std::ostream& operator<<(std::ostream& strm, const Rect& rect) {
    return strm << "{" << rect.x << ", " << rect.y << ", " << rect.width << ", " << rect.height << ", "<< "}";
}
std::ostream& operator<<(std::ostream& strm, const RectWithBorder& rect) {
    return strm << "{" << rect.x << ", " << rect.y << ", " << rect.width << ", " << rect.height << ", "<< rect.border<< "}";
}
