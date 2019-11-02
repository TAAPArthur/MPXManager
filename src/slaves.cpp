#include "globals.h"
#include "masters.h"
#include "mywm-structs.h"
#include "slaves.h"

static ArrayList<Slave*>slaveList;
ArrayList<Slave*>& getAllSlaves(void) {
    return slaveList;
}
std::ostream& operator<<(std::ostream& strm, const Slave& m) {
    return strm << "{ id:" << m.getID() << ", name:" << m.getName() << ", Master:" << m.getMasterID() << " }";
}
Master* Slave::getMaster()const {
    return getMasterByID(getMasterID(), isKeyboardDevice());
}
Slave::~Slave() {
    setMasterID(0);
}
void Slave::setMasterID(MasterID id) {
    if(attachment) {
        Master* m = getMaster();
        if(m)
            m->slaves.removeElement(this);
    }
    attachment = id;
    if(attachment) {
        Master* m = getMaster();
        if(m)
            m->slaves.add(this);
    }
}
static int endsWith(const std::string& s, const char* suffix) {
    const char* str = s.c_str();
    size_t lenstr = strlen(str);
    size_t lensuffix = strlen(suffix);
    if(lensuffix > lenstr)
        return 0;
    return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}
bool Slave::isTestDevice(std::string name) {
    return endsWith(name, "XTEST pointer") || endsWith(name, "XTEST keyboard");
}
