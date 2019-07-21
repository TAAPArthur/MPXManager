#include "ext.h"

std::unordered_map<Key, std::unordered_map<IndexKey, void*>> ext;
uint32_t AbstractIndex::count = 0;
void removeID(Key id) {
    if(ext.count(id)) {
        for(std::pair<IndexKey, void*> element : ext[id])
            element.first->dealloc(element.second);
        ext.erase(id);
    }
}
