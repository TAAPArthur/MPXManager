/**
 * @file ext.h
 * @brief Used to extend built in structs
 *
 * There are cases where one will want to be able to track state as if the state was a field in the object. This file provides an interface to simulate that. For each type of state, define a Index with the struct you want to store the state in. Calling get with this Index and Monitor/WindowInfo/Master/Workspace pointer will return the saved state related to this tuple.
 */
#ifndef MPX_EXT_H
#define MPX_EXT_H
#include <unordered_map>
#include <stdint.h>

/**
 * Used to place common code across all Indexes
 */
struct AbstractIndex {
private:
    /// global count used to generate a unique id
    static uint32_t count;
    /// the value of count at the time of creation
    const uint32_t value;
public:
    /// Creates a new instance with a unique key
    AbstractIndex(): value(count++) {}
    /// returns the unique id
    operator uint32_t () const {return value;}
    /// Allocate and return a new instance of the class associated with this Index
    virtual void* alloc() = 0;
    /**
     * Deallocate an instance of the class associated with this Index
     * @param p the pointer to deallocate
     */
    virtual void dealloc(void* p) = 0;
};
/**
 * Used to differentiate the many possible extra, independent states added, and how to alloc/dealloc them
 */
template<class T >
struct Index: AbstractIndex {
    void* alloc() override {return new T();}
    void dealloc(void* t)override {delete(T*)t;};
};
/**
 * A map from pointer to builtin struct to a map of Index* to custom struct
 */
extern std::unordered_map<const void*, std::unordered_map<AbstractIndex*, void*>> ext;

/**
 * Returns the last accessed pointer associated with key, id
 * Return value associated with key, id. If not value is currently associated, a new one is allocated
 * When the object backed by id is deallocated, the associated value is freed
 *
 * @param key
 * @param id
 * @param createNew allocate a new instance if one doesn't exists for the key,id pair
 * @param newElement if not null, will be 1 iff a new element was created
 *
 * @return a pointer or NULL
 */
template<class T >
T* get(Index<T>& key, const void* id, bool createNew = 1, bool* newElement = NULL) {
    auto& map = ext[id];
    int count = map.count(&key);
    if(newElement)
        *newElement = createNew && !count;
    if(!createNew && !count)
        return NULL;
    if(createNew && count == 0) {
        map[&key] = key.alloc();
    }
    return (T*)map[&key];
}
template<class T >
/**
 * Removes and frees the entry associated with key,id if it exists
 *
 * @param key
 * @param id
 */
void remove(Index<T>& key, void* id) {
    auto& map = ext[id];
    if(map.count(&key) == 0)
        return;
    key.dealloc(map[&key]);
    map.erase(&key);
}
/**
 * Removes all values associated with id across all indexes
 *
 * @param id
 */
void removeID(void* id);
#endif
