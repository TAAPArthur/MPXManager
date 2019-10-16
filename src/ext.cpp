#include "ext.h"

std::unordered_map<void*, std::unordered_map<AbstractIndex*, void*>> ext;
uint32_t AbstractIndex::count = 0;
void removeID(void* id) {
    if(ext.count(id)) {
        for(std::pair<AbstractIndex*, void*> element : ext[id])
            element.first->dealloc(element.second);
        ext.erase(id);
    }
}
