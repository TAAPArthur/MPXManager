#include <assert.h>
#include <fcntl.h>
#include <libinput.h>
#include <linux/input.h>
#include <string.h>
#include <sys/eventfd.h>
#include <sys/poll.h>
#include "unistd.h"

#include "../logger.h"
#include "../monitors.h"
#include "../threads.h"
#include "gestures.h"
/**
 * Opens a path with given flags. Path probably references an input device and likely starts with  /dev/input/
 * If user_data is not NULL and points to a non-zero value, then the device corresponding to path will be grabbed (@see EVIOCGRAB)
 *
 * @param path path to open
 * @param flags flags to open path with
 * @param user_data if true the device will be grabbed
 *
 * @return the file descriptor corresponding to the opened file
 */
static int open_restricted(const char* path, int flags, void* user_data) {
    bool* grab = (bool*)user_data;
    int fd = open(path, flags);
    if(fd >= 0 && grab && *grab && ioctl(fd, EVIOCGRAB, &grab) == -1)
        LOG(LOG_LEVEL_ERROR, "Grab requested, but failed for %s (%s)\n",
            path, strerror(errno));
    return fd < 0 ? -errno : fd;
}
/**
 *
 * @param fd the file descriptor to close
 * @param user_data unused
 */
static void close_restricted(int fd, void* user_data __attribute__((unused))) {
    close(fd);
}

static struct libinput_interface interface = {
    .open_restricted = open_restricted,
    .close_restricted = close_restricted,
};
struct libinput* createUdevInterface(bool grab) {
    struct udev* udev = udev_new();
    struct libinput* li = libinput_udev_create_context(&interface, &grab, udev);
    libinput_udev_assign_seat(li, "seat0");
    return li;
}
struct libinput* createPathInterface(const char** paths, int num, bool grab) {
    struct libinput* li = libinput_path_create_context(&interface, &grab);
    for(int i = 0; i < num; i++)
        libinput_path_add_device(li, paths[i]);
    return li;
}
static inline double getX(struct libinput_event_touch* event) {
    return libinput_event_touch_get_x(event);
}
static inline double getY(struct libinput_event_touch* event) {
    return libinput_event_touch_get_y(event);
}
static inline double getXInPixels(struct libinput_event_touch* event) {
    return libinput_event_touch_get_x_transformed(event, getRootWidth());
}
static inline double getYInPixels(struct libinput_event_touch* event) {
    return libinput_event_touch_get_y_transformed(event, getRootHeight());
}
void processTouchEvent(libinput_event_touch* event, enum libinput_event_type type) {
    Point point;
    Point pointPixel;
    int32_t seat;
    uint32_t id;
    struct libinput_device* inputDevice;
    switch(type) {
        case LIBINPUT_EVENT_TOUCH_MOTION:
        case LIBINPUT_EVENT_TOUCH_DOWN:
            pointPixel = {(int)getXInPixels(event), (int)getYInPixels(event)};
            point = { (int)getX(event), (int)getY(event)};
            [[fallthrough]];
        case LIBINPUT_EVENT_TOUCH_CANCEL:
        case LIBINPUT_EVENT_TOUCH_UP:
            inputDevice = libinput_event_get_device((libinput_event*)event);
            id = libinput_device_get_id_product(inputDevice);
            seat = libinput_event_touch_get_seat_slot(event);
            break;
        default:
            break;
    }
    switch(type) {
        case LIBINPUT_EVENT_TOUCH_DOWN:
            startGesture(id, seat, point, pointPixel,  libinput_device_get_sysname(inputDevice),
                libinput_device_get_name(inputDevice));
            break;
        case LIBINPUT_EVENT_TOUCH_MOTION:
            continueGesture(id, seat, point, pointPixel);
            break;
        case LIBINPUT_EVENT_TOUCH_UP:
            endGesture(id, seat);
            break;
        case LIBINPUT_EVENT_TOUCH_CANCEL:
            cancelGesture(id, seat);
        default:
            break;
    }
}
static int dummyFD;
static void wakeupFunction() {
    long num = 100;
    write(dummyFD, &num, sizeof(num));
}
static void listenForGestures(struct libinput* li) {
    dummyFD = eventfd(0, EFD_CLOEXEC);
    registerWakeupFunction(wakeupFunction);
    libinput_event* event;
    auto fd = libinput_get_fd(li);
    struct pollfd fds[2] = {{fd, POLLIN}, {(short)dummyFD, POLLIN}};
    while(!isShuttingDown()) {
        poll(fds, LEN(fds), -1);
        if(fds[0].revents & POLLIN == 0)
            continue;
        libinput_dispatch(li);
#ifndef NDEBUG
        lock();
#endif
        while(!isShuttingDown() && (event = libinput_get_event(li))) {
            enum libinput_event_type type = libinput_event_get_type(event);
            processTouchEvent((libinput_event_touch*)event, type);
            libinput_event_destroy(event);
            libinput_dispatch(li);
        }
#ifndef NDEBUG
        unlock();
#endif
    }
    libinput_unref(li);
}
void enableGestures(const char** paths, int num, bool grab) {
    struct libinput* li = paths && num ? createPathInterface(paths, num, grab) : createUdevInterface(0);
    spawnThread([ = ] {listenForGestures(li);}, "LibInputGesturesReader");
    spawnThread(gestureEventLoop, "GesturesEventProcessor");
}
