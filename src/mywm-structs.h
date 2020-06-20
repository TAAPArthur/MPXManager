/**
 * @file mywm-structs.h
 * @brief contains global struct definitions
 */
#ifndef MYWM_STRUCTS_H
#define MYWM_STRUCTS_H

#include "arraylist.h"
#include <iostream>

/// typeof WindowInfo::id
typedef unsigned int WindowID;
/// typeof Master::id
typedef unsigned int MasterID;
/// typeof Slave::id
typedef MasterID SlaveID;
/// typeof Workspace::id
typedef unsigned int WorkspaceID;
/// typeof Monitor::id
typedef unsigned int MonitorID;
/// holds current time in mills
typedef unsigned long TimeStamp;

struct Monitor;
struct Layout;
struct Master;
struct Slave;
struct WindowInfo;
struct Workspace;

/**
 * Superclass of all structs with an X counterpart
 */
struct WMStruct {
protected:
    /// unique identifier
    const uint32_t id;
public:
    /**
     *
     *
     * @param id the unique id
     */
    WMStruct(uint32_t id): id(id) {};
    virtual ~WMStruct() = default;
    /// @return the unique identifier
    uint32_t getID()const {return id;}
    /// @return the unique identifier
    operator uint32_t ()const {return getID();}
    /// @return 1 iff the unique identifiers match
    bool operator==(const WMStruct& s)const {return id == s.id;}
    /// @param id
    /// @return 1 iff the unique identifier matches id
    bool operator==(const uint32_t& id)const {return this->id == id;}
};

/// @{
/**
 * Prints object
 * @return
 */
std::ostream& operator<<(std::ostream&, const Monitor&);
std::ostream& operator>>(std::ostream&, const Monitor&);
std::ostream& operator<<(std::ostream&, const Layout&);
std::ostream& operator<<(std::ostream&, const Master&);
std::ostream& operator>>(std::ostream&, const Master&);
std::ostream& operator<<(std::ostream&, const Slave&);
std::ostream& operator<<(std::ostream&, const WindowInfo&);
std::ostream& operator>>(std::ostream&, const WindowInfo&);
std::ostream& operator<<(std::ostream&, const Workspace&);
std::ostream& operator>>(std::ostream&, const Workspace&);
/// @}

/**
 * For each member in list, deference member and print
 *
 * @tparam T
 * @param stream
 * @param list
 *
 * @return
 */
template<class T>
std::ostream& operator<<(std::ostream& stream, const ArrayList<T>& list) {
    stream << "{ ";
    for(uint32_t i = 0; i < list.size(); i++)
        stream << (i ? ", " : "") << "{" << list[i] << "}";
    stream << " }";
    return stream;
}
/**
 * For each member in list, deference member and print
 *
 * @tparam T
 * @param stream
 * @param list
 *
 * @return
 */
template<class T>
std::ostream& operator<<(std::ostream& stream, const ArrayList<T*>& list) {
    stream << "{ ";
    for(uint32_t i = 0; i < list.size(); i++)
        stream << (i ? ", " : "") << *list[i];
    stream << " }";
    return stream;
}
/**
 * For each member in list, deference member and print int representation
 *
 * @tparam T
 * @param stream
 * @param list
 *
 * @return
 */
template<class T>
std::enable_if_t < std::is_convertible<T, int>::value, std::ostream& >
operator>>(std::ostream& stream, const ArrayList<T*>& list) {
    stream << "{ ";
    for(uint32_t i = 0; i < list.size(); i++)
        stream << (i ? ", " : "") << (int)*list[i];
    stream << " }";
    return stream;
}
/**
 * For each member in list, deference member and print string representation
 *
 * @tparam T
 * @param stream
 * @param list
 *
 * @return
 */
template<class T>
std::enable_if_t < std::is_convertible<T, std::string>::value, std::ostream& >
operator>>(std::ostream& stream, const ArrayList<T*>& list) {
    stream << "{ ";
    for(uint32_t i = 0; i < list.size(); i++)
        stream << (i ? ", " : "") << (std::string)*list[i];
    stream << " }";
    return stream;
}
#endif
