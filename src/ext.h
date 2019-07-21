#ifndef MPX_EXT_H
#define MPX_EXT_H
#include <unordered_map>

struct AbstractIndex {
private:
    static uint32_t count;
    const uint32_t value;
public:
    AbstractIndex(): value(count++) {}
    bool operator <(AbstractIndex& index) {
        return value < index.value;
    }
    bool operator ==(AbstractIndex& index) {
        return value == index.value;
    }
    operator uint32_t () const {return value;}
    virtual void* alloc() = 0;
    virtual void dealloc(void*) = 0;
};
template<class T >
struct Index: AbstractIndex {
    Index() {}
    void* alloc() override {return new T();}
    void dealloc(void* t)override {delete(T*)t;};
};
typedef void* Key;
typedef AbstractIndex* IndexKey;
extern std::unordered_map<Key, std::unordered_map<IndexKey, void*>> ext;

template<class T >
T* get(Index<T>& key, Key id, bool createNew = 1, bool* newElement = NULL) {
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
void remove(Index<T>& key, Key id) {
    auto& map = ext[id];
    if(map.count(&key) == 0)
        return;
    key.dealloc(map[&key]);
    map.erase(&key);
}
void removeID(Key id);
#endif
