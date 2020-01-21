#include "../ringbuffer.h"
#include "tester.h"

const int size = 12;
MPX_TEST("ring_buffer_push_null", {
    RingBuffer<int*, size> buffer;
    assert(!buffer.push(NULL));
    assert(!buffer.getSize());
});
MPX_TEST("ring_single_write", {
    RingBuffer<int*, size> buffer;
    int i = i;
    buffer.push(&i);
    assertEquals(buffer.peekEnd(), &i);
    assertEquals(buffer.peek(), buffer.peekEnd());
    auto p = buffer.pop();
    assert(p);
    assertEquals(p, &i);
})
MPX_TEST_ITER("ring_buffer", 2, {
    int dummy = -1;
    RingBuffer<int*, size> buffer;
    assertEquals(0, buffer.getSize());
    assertEquals(size, buffer.getMaxSize());
    for(int i = 0; i < (_i + 1 * 10); i++) {
        for(int i = 0; i < buffer.getMaxSize() - 1; i++)
            assert(buffer.push(new int(i)));
        assert(!buffer.isFull());
        assert(!buffer.push(new int(size)));
        assert(buffer.isFull());
        assertEquals(size, buffer.getSize());
        assert(!buffer.push(&dummy));
        assert(buffer.isFull());
        assert(buffer.peekEnd() != &dummy);
        assertEquals(buffer.getMaxSize(), buffer.getSize());
        for(int i = 0; i < buffer.getMaxSize(); i++)
            assertAndDelete(buffer.pop());
        assert(buffer.isEmpty());
        assert(!buffer.pop());
    }
});
MPX_TEST("ring_buffer_half_full", {
    const int minSize = size / 4;
    const int maxSize = size * 3 / 4;
    RingBuffer<int*, size> buffer;
    assertEquals(size, buffer.getMaxSize());
    for(int i = 0; i < 10; i++) {
        while(buffer.getSize() < maxSize)
            assert(buffer.push(new int(i)));
        while(buffer.getSize() > minSize)
            assertAndDelete(buffer.pop());
    }
    assert(buffer.peekEnd());
});
