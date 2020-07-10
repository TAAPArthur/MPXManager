/**
 * @file arraylist.h
 * @brief ArrayList implementation
 */
#ifndef ARRAY_LIST_H
#define ARRAY_LIST_H

/// Returns the length of the array
#define LEN(X) (sizeof X / sizeof X[0])

#include <stdint.h>
#include <sys/types.h>
#include <vector>

/**
 * Reversible iterator
 */
template<class T>
struct Iterator {
    /// backing pointer
    const T* p;
    /// iff true the iterator will iterate backwards
    const bool reverse;
    /**
     * Constructs an iterator starting at p
     * @param p backing pointer. Should be the first element in the array
     * @param reverse whether to reverse the iteration or not.
     */
    Iterator(const T* p, bool reverse = 0): p(p), reverse(reverse) {}
    /// Move to the next element
    Iterator operator ++() {
        auto temp = *this;
        reverse ? p-- : p++;
        return temp;
    }
    /// compares backing pointer
    bool operator ==(const Iterator& other)const {return p == other.p;}
    /// compares backing pointer
    bool operator !=(const Iterator& other)const {return !(*this == other);}
    /// returns object backed backing pointer
    const T& operator*() {return *p;}
    /// returns backing pointer
    T* operator->() {return p;}
};

/**
 * Wrapper around std::vector with some Java-like methods
 * This class doesn't not support NULL values
 */
template<class T>
struct ArrayList: std::vector<T> {
    /// Enable if T or *T (if pointer) is convertible to an int
    template<typename U = T, typename V = int>
    using EnableIf = typename std::enable_if_t < std::is_convertible<std::remove_pointer_t<U>, int>::value, V > ;
    /// Enable if T is a pointer
    template<typename U = T, typename V = int>
    using EnableIfPointer = typename std::enable_if_t<std::is_pointer<U>::value, V> ;
    /// Enable if T is not a pointer
    template<typename U = T, typename V = int>
    using EnableIfNotPointer = typename std::enable_if_t < !std::is_pointer<U>::value, V > ;
    /// Enable if *T is convertible to an int
    template<typename U = T, typename V = int>
    using EnableIfPointerAndInt = typename std::enable_if_t < std::is_convertible<std::remove_pointer_t<U>, int>::value &&
        std::is_pointer<U>::value, V > ;

    /// Constructs an empty list
    ArrayList() {}
    /// Constructs a list with the specified elements
    ArrayList(std::initializer_list<T> __l) : std::vector<T>(__l) {}
    /**
     * Copies the members onto the heap and adds them to the list
     */
    template<typename U = T>
    ArrayList(std::initializer_list<std::remove_pointer_t<U>> __l, EnableIfPointer<U>_enable __attribute__((unused)) = 0) {
        for(auto t : __l)
            this->add(t);
    }
    virtual ~ArrayList() = default;

    /**
     * @param __l list of elements to add
     */
    void addAll(const std::vector<T>& __l) {
        for(auto t : __l)
            this->add(t);
    }
    /// Reverse iterator
    virtual Iterator<T> rbegin() const {
        return Iterator(this->data() + this->size() - 1, 1);
    }
    /// Reverse iterator end
    virtual Iterator<T> rend() const {
        return this->data() - 1;
    }
    /// normal iterator end
    virtual Iterator<T> begin() const {
        return Iterator(this->data());
    }
    /// normal iterator end
    virtual Iterator<T> end() const {
        return this->data() + this->size();
    }
    /// Copies value to the heap and adds into according to remove
    template<typename U = T>
    EnableIfPointer<U, bool> add(const std::remove_pointer_t<U>& value, bool remove = 0) {
        return add(new std::remove_pointer_t<U>(value), remove);
    }
    /**
     * Adds value to the end of list
     * @param value the value to append
     */
    virtual void add(T value) {
        this->push_back(value);
    }
    /**
     * Adds/removes value to list according to flag.
     * If value is not added to the list, it is deleted. If an element is removed from the list it is deleted.
     * For ADD_REMOVE, the value will also be deleted
     * @return 1 iff element was not freed or if remove == ADD_REMOVE, if value was not present in list
     */
    template<typename U = T>
    EnableIfPointer<U, bool>add(T value, bool removeValue) {
        if(removeValue) {
            int i = indexOf(value);
            delete value;
            if(i == -1)
                return 1;
            delete remove(i);
            return 0;
        }
        else {
            add(value);
            return 1;
        }
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
        while(size())
            delete pop();
    }
    /**
     * @param value
     * @return the first element whose memory starts with value
     */
    T find(const T value)const {
        int index = indexOf(value);
        return index != -1 ? (*this)[index] : (T)0;
    }

    /**
     * @param value
     * @return the first element whose memory starts with value
     */
    template<typename U = T>
    EnableIf<U, T> find(uint32_t value) const {
        int index = indexOf(value);
        return index != -1 ? (*this)[index] : (T)0;
    }
    /**
     * @param value
     * @return the first element whose memory starts with value
     */
    bool contains(const T value)const {
        return indexOf(value) != -1;
    }
    /**
     * Remove the index-th element form list
     * @param index
     * @return the element that was removed
     */
    T remove(uint32_t index) {
        T value = (*this)[index];
        for(uint32_t i = index; i < this->size() - 1; i++)
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
        int index = indexOf(element);
        return index != -1 ? this->remove(index) : (T) 0;
    }
    /**
     * @copydoc removeElement
     */
    template<typename U = T>
    EnableIf<U, T> removeElement(uint32_t element) {
        int index = indexOf(element);
        return index != -1 ? this->remove(index) : NULL;
    }
    /**
     * Removes the last element from the list and returns it
     * @return the element removed
     */
    T pop() {return remove(this->size() - 1);};
    /**
     * Returns the index of the element equal to value.
     * If size is non zero, we check for equality by comparing the first size bytes of value with every element.
     * If size is zero, we check if the pointers are equal
     * @param value
     * @return the index of the element whose memory address starts with the first size byte of value
     */
    template<typename U = T>
    EnableIfNotPointer<U> indexOf(const T value)const {
        for(int i = this->size() - 1; i >= 0; i--)
            if((*this)[i] == value)
                return i;
        return -1;
    }
    /**
     * Returns the index of the element that points to an element equal to what value points to.
     * @copydoc indexOf
     */
    template<typename U = T>
    auto indexOf(const std::remove_pointer_t<U>* value)const -> decltype((*value) == (*value), int()) {
        for(int i = this->size() - 1; i >= 0; i--)
            if((*(*this)[i]) == *value)
                return i;
        return -1;
    }
    /**
     * Returns the index of the element that points to an element with an int value equal to value
     * @copydoc indexOf
     */
    template<typename U = T>
    EnableIfPointerAndInt<U, int> indexOf(uint32_t value)const {
        for(int i = this->size() - 1; i >= 0; i--)
            if((uint32_t)(*(*this)[i]) == value)
                return i;
        return -1;
    }
    /**
     * Returns the element at index current + delta if the array list where to wrap around
     * For example:
     * getNextIndex(0,-1) returns size-1
     * getNextIndex(size-1,1) returns 0
     * @param current
     * @param delta
     */
    uint32_t getNextIndex(int current, int delta)const {
        return ((current + delta) % (ssize_t)this->size() + this->size()) % this->size();
    }
    /**
     * Returns the value at the next index
     *
     * @param current
     * @param delta
     *
     * @return
     */
    T getNextValue(int current, int delta)const {
        return (*this)[getNextIndex(current, delta)];
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
        T newHead = (*this)[ index];
        for(int i = index; i > 0; i--)
            (*this)[i] = (*this)[i - 1];
        (*this)[0] = newHead;
    }
    /**
     * Sort the members of this list in ascending order using the '<' operator
     */
    void sort() {
        for(auto i = 0U; i < size(); i++)
            for(int n = i; n > 0; n--)
                if(*(*this)[n] < * (*this)[n - 1])
                    swap(n, n - 1);
                else break;
    }
    /**
     * Swaps the element at index1 with index2
     * @param index1
     * @param index2
     */
    void swap(uint32_t index1, uint32_t index2) {
        T temp = (*this)[ index1];
        (*this)[index1] = (*this)[index2];
        (*this)[index2] = temp;
    }

    /**
     * Deep equality check used when T is a pointer
     * @tparam U
     * @param list
     *
     * @return 1 if the elements all point to equivalent structures
     */
    template<typename U = T>
    EnableIfPointer<U, bool>operator ==(const ArrayList<T>& list)const {
        if(size() != list.size())
            return 0;
        for(int i = 0; i < list.size(); i++)
            if(!(*(*this)[i] == *list[i]))
                return 0;
        return 1;
    }


    /**
     * @param list
     *
     * @return true if not equal to list
     */
    bool operator !=(const ArrayList<T>& list)const {
        return !(*this == list);
    }
};
/**
 * Subclass of ArrayList that iterates in the reverse order
 */
template<class T>
struct ReverseArrayList: ArrayList<T> {
    /// Returns an iterator from end to start
    Iterator<T> begin() const override {
        return ArrayList<T>::rbegin();
    }
    /// counterpart of begin
    Iterator<T> end() const override {
        return ArrayList<T>::rend();
    }
    /// Returns start to end
    Iterator<T> rbegin() const override {
        return ArrayList<T>::begin();
    }
    /// counterpart of end
    Iterator<T> rend() const override {
        return ArrayList<T>::end();
    }
};
/**
 * ArrayList of pointers
 * When this object is deconstructed, ArrayList<T>::deleteElements() is called
 *
 * @tparam T
 */
template<typename T>
struct UniqueArrayList: ArrayList<T> {
    static_assert(std::is_pointer<T>::value);
    /// Constructs an empty list
    UniqueArrayList() {}
    /// Constructs a list with the specified elements
    UniqueArrayList(std::initializer_list<T> __l) : ArrayList<T>(__l) {}
    /**
     * Steals data from list
     * @param list
     */
    UniqueArrayList(ArrayList<T> list) : ArrayList<T>(std::move(list)) {}
    /**
     * Copies the members onto the heap and adds them to the list
     */
    UniqueArrayList(std::initializer_list<std::remove_pointer_t<T>> __l): ArrayList<T>(__l) {}
    ~UniqueArrayList() {
        ArrayList<T>::deleteElements();
    }
};

#endif
