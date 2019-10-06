/**
 * @file mywm-structs.h
 * @brief contains global struct definitions
 */
#ifndef MYWM_STRUCTS_H
#define MYWM_STRUCTS_H

#include "arraylist.h"
#include <iostream>
#include <unordered_map>

/// typeof WindowInfo::mask
typedef unsigned int WindowMask;
/// typeof WindowInfo::id
typedef unsigned int WindowID;
/// typeof Master::id
typedef unsigned int MasterID;
typedef MasterID SlaveID;
/// typeof Workspace::id
typedef unsigned int WorkspaceID;
/// typeof Monitor::id
typedef unsigned int MonitorID;
typedef unsigned long TimeStamp;

struct Monitor;
struct Layout;
struct Master;
struct Slave;
struct WindowInfo;
struct Workspace;

struct WMStruct {
    const uint32_t id;
    WMStruct(uint32_t id): id(id) {};
    virtual ~WMStruct() = default;
    uint32_t getID()const {return id;}
    operator int()const {return getID();}
    bool operator==(const WMStruct& s)const {return id == s.id;}
    bool operator==(const uint32_t& id)const {return this->id == id;}
};

std::ostream& operator<<(std::ostream&, const Monitor&);
std::ostream& operator<<(std::ostream&, const Layout&);
std::ostream& operator<<(std::ostream&, const Master&);
std::ostream& operator<<(std::ostream&, const Slave&);
std::ostream& operator<<(std::ostream&, const WindowInfo&);
std::ostream& operator<<(std::ostream&, const Workspace&);
template<class T>
std::ostream& operator<<(std::ostream& stream, const ArrayList<T>& list) {
    stream << "{ ";
    for(int i = 0; i < list.size(); i++)
        stream  << (i ? ", " : "") << list[i];
    stream << " }";
    return stream;
}
template<class T>
std::ostream& operator<<(std::ostream& stream, const ArrayList<T*>& list) {
    stream << "{ ";
    for(int i = 0; i < list.size(); i++)
        stream  << (i ? ", " : "") << *list[i];
    stream << " }";
    return stream;
}
template<class T>
std::enable_if_t < std::is_convertible<T, int>::value, std::ostream& >
operator>>(std::ostream& stream, const ArrayList<T*>& list) {
    stream << "{ ";
    for(int i = 0; i < list.size(); i++)
        stream  << (i ? ", " : "") << (int)*list[i];
    stream << " }";
    return stream;
}
template<class T>
std::enable_if_t < std::is_convertible<T, std::string>::value, std::ostream& >
operator>>(std::ostream& stream, const ArrayList<T*>& list) {
    stream << "{ ";
    for(int i = 0; i < list.size(); i++)
        stream  << (i ? ", " : "") << (std::string)*list[i];
    stream << " }";
    return stream;
}
#endif
