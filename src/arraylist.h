/**
 * @file util.h
 * @brief ArrayList and other generic helper methods
 *
 */
#ifndef MYWM_UTIL
#define MYWM_UTIL

///Returns the length of the array
#define LEN(X) (sizeof X / sizeof X[0])

#include <assert.h>
#include <stdint.h>
#include <sys/types.h>
#include <type_traits>
#include <vector>


enum AddFlag {
    ADD_ALWAYS,
    ADD_UNIQUE,
    ADD_REMOVE,
    /// Adds value to the list iff the list does not already contain it else it removes the element
    ADD_TOGGLE,
    PREPEND_ALWAYS,
    PREPEND_UNIQUE,
    /// Adds value to the list iff the list does not already contain it else it removes the element
    PREPEND_TOGGLE,
};

template<class T>
struct Iterator {
    const T* p;
    const bool reverse;
    Iterator(const T* p, bool reverse = 0): p(p), reverse(reverse) {}
    Iterator operator ++() {
        auto temp = *this;
        reverse ? p-- : p++;
        return temp;
    }
    bool operator ==(const Iterator& other)const {return p == other.p;}
    bool operator !=(const Iterator& other)const {return !(*this == other);}
    const T& operator*() {return *p;}
    T* operator->() {return p;}
};

template<class T>
struct ArrayList: std::vector<T> {
    template<typename U = T, typename V = int>
    using EnableIf = typename std::enable_if_t < std::is_convertible<std::remove_pointer_t<U>, int>::value, V >  ;
    template<typename U = T, typename V = int>
    using EnableIfPointer = typename std::enable_if_t<std::is_pointer<U>::value, V>  ;
    template<typename U = T, typename V = int>
    using EnableIfNotPointer = typename std::enable_if_t < !std::is_pointer<U>::value, V >  ;
    template<typename U = T, typename V = int>
    using EnableIfPointerAndInt = typename std::enable_if_t < std::is_convertible<std::remove_pointer_t<U>, int>::value &&
                                  std::is_pointer<U>::value, V >  ;
    template<typename U = T, typename V = int>
    ArrayList() {}
    ArrayList(std::initializer_list<T> __l) {
        for(T t : __l)
            this->add(t);
    }
    template<typename U = T>
    ArrayList(std::initializer_list<std::remove_pointer_t<U>> __l, EnableIfPointer<U>_enable __attribute__((unused)) = 0) {
        for(auto t : __l)
            this->add(t);
    }
    template<typename U = T>
    ArrayList(T t, int num, EnableIfPointer<U>_enable __attribute__((unused)) = 0) {
        for(int i = 0; i < num; i++)
            this->add(t + i);
    }
    virtual ~ArrayList() = default;
    virtual Iterator<T> rbegin() const {
        return Iterator(this->data() + this->size() - 1, 1);
    }
    virtual Iterator<T> rend() const {
        return this->data() - 1;
    }
    virtual Iterator<T> begin() const {
        return Iterator(this->data());
    }
    virtual Iterator<T> end() const {
        return this->data() + this->size();
    }
    template<typename U = T>
    EnableIfPointer<U, bool> add(const std::remove_pointer_t<U>& value, AddFlag flag = ADD_ALWAYS) {
        return add(new std::remove_pointer_t<U>(value), flag);
    }
    /**
     * Adds value to the end of list
     * @param value the value to append
     */
    void add(T value) {
        this->push_back(value);
    }
    /**
     * Adds/removes value to list according to flag.
     * If value is not added to the list, it is deleted. If an element is removed from the list it is deleted.
     * @return 1 if element was not freed
     */
    template<typename U = T>
    EnableIfPointer<U, bool>add(T value, AddFlag flag) {
        switch(flag) {
            case PREPEND_ALWAYS:
                add(value);
                shiftToHead(size() - 1);
                return 1;
            case ADD_ALWAYS:
                add(value);
                return 1;
            case PREPEND_UNIQUE:
                if(addUnique(value)) {
                    shiftToHead(size() - 1);
                    return 1;
                }
                break;
            case ADD_UNIQUE:
                if(addUnique(value))
                    return 1;
                break;
            case ADD_REMOVE: {
                int i = indexOf(value);
                delete value;
                if(i == -1)
                    return 1;
                delete remove(i);
                return 0;
            }
            case PREPEND_TOGGLE:
            case ADD_TOGGLE: {
                int index = indexOf(value);
                if(index == -1) {
                    this->add(value);
                    if(flag == PREPEND_TOGGLE)
                        shiftToHead(size() - 1);
                    return 1;
                }
                else
                    delete this->remove(index);
            }
        }
        delete value;
        return 0;
    }
    /**
     * Adds value to the list iff the list does not already contain it
     * @param value new value to add
     * @return 1 iff a new element was inserted
     */
    bool addUnique(T value) {
        if(!find(value))
            this->add(value);
        else return 0;
        return 1;
    };
    /**
     * Clear the list and free all elements
     */
    template<typename U = T>
    EnableIfPointer<U, void> deleteElements() {
        for(T t : *this)
            delete t;
        this->clear();
    }
    /**
     * @param value
     * @return the first element whose memory starts with value
     */
    T find(const T  value)const {
        int index = indexOf(value);
        return index != -1 ? (*this)[index] : (T)0;
    }

    /**
     * @param value
     * @return the first element whose memory starts with value
     */
    template<typename U = T>
    EnableIf<U, T> find(uint32_t  value) const {
        int index = indexOf(value);
        return index != -1 ? (*this)[index] : NULL;
    }
    /**
     * @param value
     * @return the first element whose memory starts with value
     */
    bool contains(const T  value)const {
        return indexOf(value) != -1;
    }

    /**
     * @param value
     * @return the first element whose memory starts with value
     */
    template<typename U = T>
    EnableIf<U, bool> contains(uint32_t  value) const {
        return indexOf(value) != -1;
    }
    /**
     * Remove the index-th element form list
     * @param index
     * @return the element that was removed
     */
    T remove(uint32_t index) {
        assert(index < this->size());
        T value = (*this)[index];
        for(int i = index; i < this->size() - 1; i++)
            (*this)[i] = (*this)[i + 1];
        this->pop_back();
        return value;
    }
    /**
     * Removes the element from the list
     *
     * @param element the value of the element
     *
     * @see indexOf() remove()
     * @return the element that has been removed or NULL
     */
    T removeElement(const T element) {
        uint32_t index = indexOf(element);
        return index != -1 ? this->remove(index) : (T) 0;
    }
    template<typename U = T>
    EnableIf<U, T> removeElement(uint32_t  element) {
        uint32_t index = indexOf(element);
        return index != -1 ? this->remove(index) : NULL;
    }
    T pop() {return remove(this->size() - 1);};
    /**
     * Returns the index of the element with value.
     * If size is non zero, we check for equality by comparing the first size bytes of value with every element.
     * If size is zero, we check if the pointers are equal
     * @param value
     * @return the index of the element whose memory address starts with the first size byte of value
     */
    template<typename U = T>
    EnableIfNotPointer<U> indexOf(const T t)const {
        for(int i = this->size() - 1; i >= 0; i--)
            if((*this)[i] == t)
                return i;
        return -1;
    }
    template<typename U = T>
    auto indexOf(const std::remove_pointer_t<U>* t)const -> decltype((*t) == (*t), int()) {
        for(int i = this->size() - 1; i >= 0; i--)
            if((*(*this)[i]) == *t)
                return i;
        return -1;
    }
    template<typename U = T>
    EnableIfPointerAndInt<U, int> indexOf(uint32_t id)const  {
        for(int i = this->size() - 1; i >= 0; i--)
            if((uint32_t)(*(*this)[i]) == id)
                return i;
        return -1;
    }
    /**
     * Returns the element at index current + delta if the array list where to wrap around
     * For example:
     *   getNextIndex(0,-1) returns size-1
     *   getNextIndex(size-1,1) returns 0
     * @param current
     * @param delta
     */
    uint32_t getNextIndex(int current, int delta)const {
        return ((current + delta) % (ssize_t)this->size() + this->size()) % this->size();
    }
    /// returns the number of elements in the list
    uint32_t size()const {return std::vector<T>::size();};
    /**
     * Removes the index-th element and re-inserts it at the end of the list
     * @param index
     */
    void shiftToEnd(uint32_t index) {
        add(remove(index));
    }
    /**
     * Removes the index-th element and re-inserts it at the head of the list
     * @param index
     */
    void shiftToHead(uint32_t index) {
        assert(index < this->size());
        T newHead = (*this)[ index];
        for(int i = index; i > 0; i--)
            (*this)[i] = (*this)[i - 1];
        (*this)[0] = newHead;
    }
    /**
     * Swaps the element at index1 with index2
     * @param index1
     * @param index2
     */
    void swap(uint32_t index1, uint32_t index2) {
        assert(index1 < this->size() && index2 < this->size());
        T temp = (*this)[ index1];
        (*this)[index1] = (*this)[index2];
        (*this)[index2] = temp;
    }
};
template<class T>
struct ReverseArrayList: ArrayList<T> {
    virtual Iterator<T> begin() const override {
        return Iterator(this->data() + this->size() - 1, 1);
    }
    virtual Iterator<T> end() const override {
        return this->data() - 1;
    }
    virtual Iterator<T> rbegin() const override {
        return Iterator(this->data());
    }
    virtual Iterator<T> rend() const override {
        return this->data() + this->size();
    }
};
template<typename T>
struct UniqueArrayList: ArrayList<T*> {
    ~UniqueArrayList() {
        ArrayList<T*>::deleteElements();
    }
};

#endif
