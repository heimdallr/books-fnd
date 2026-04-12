// xkeyboard.cpp
// Implementation of a class to get keyboard layout information and change layouts
// Copyright (C) 2008 by Jay Bromley <jbromley@gmail.com>
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// $Id: XKeyboard.cpp 53 2008-07-18 08:38:47Z jay $
//
// 2010-01-02 Kristian Setälä added code to retrieve layout variant information

#include "log.h"

#include "XKeyboard.h"
#include <algorithm>
#include <cstdlib>
#include <cctype>
#include <cstring>
#include <stdexcept>
#include <X11/XKBlib.h>

using namespace HomeCompa::Platform;

namespace{

const std::string EMPTY;

int compareNoCase(const std::string& s1, const std::string& s2)
{
    std::string::const_iterator it1 = s1.begin();
    std::string::const_iterator it2 = s2.begin();

    while (it1 != s1.end() && it2 != s2.end()) {
        if (::toupper(*it1) != ::toupper(*it2)) {
            return (::toupper(*it1) < ::toupper(*it2)) ? -1 : 1;
        }

        ++it1;
        ++it2;
    }

    size_t size1 = s1.size();
    size_t size2 = s2.size();

    if (size1 == size2)  {
        return 0;
    }
    return (size1 < size2) ? -1 : 1;
}

const std::string& Get(const StringVector& data, unsigned int index) noexcept
{
    return index < data.size() ? data[index] : EMPTY;
}

class XkbSymbolParser
{
public:
    typedef StringVector::iterator StringVectorIter;

    XkbSymbolParser()
    {
        _nonSymbols.push_back("group");
        _nonSymbols.push_back("inet");
        _nonSymbols.push_back("pc");
    }

    void parse(const std::string& symbols, StringVector& symbolList, StringVector& variantList)
    {
        bool inSymbol = false;
        std::string curSymbol;
        std::string curVariant;

        //std::cout << symbols << std::endl;
        // A sample line:
        // pc+fi(dvorak)+fi:2+ru:3+inet(evdev)+group(menu_toggle)

        for (size_t i = 0; i < symbols.size(); i++) {
            char ch = symbols[i];
            if (ch == '+' || ch == '_') {
                if (inSymbol) {
                    if (isXkbLayoutSymbol(curSymbol)) {
                        symbolList.push_back(curSymbol);
                        variantList.push_back(curVariant);
                    }
                    curSymbol.clear();
                    curVariant.clear();
                } else {
                    inSymbol = true;
                }
            } else if (inSymbol && (isalpha(static_cast<int>(ch)) || ch == '_')) {
                curSymbol.append(1, ch);
            } else if (inSymbol && ch == '(') {
                while (++i < symbols.size()) {
                    ch = symbols[i];
                    if (ch == ')')
                        break;
                    else
                        curVariant.append(1, ch);
                }
            } else {
                if (inSymbol) {
                    if (isXkbLayoutSymbol(curSymbol)) {
                        symbolList.push_back(curSymbol);
                        variantList.push_back(curVariant);
                    }
                    curSymbol.clear();
                    curVariant.clear();
                    inSymbol = false;
                }
            }
        }

        if (inSymbol && !curSymbol.empty() && isXkbLayoutSymbol(curSymbol)) {
            symbolList.push_back(curSymbol);
            variantList.push_back(curVariant);
        }
    }

private:
    bool isXkbLayoutSymbol(const std::string& symbol) const
    {
        const auto result = find(_nonSymbols.cbegin(), _nonSymbols.cend(), symbol);
        return result == _nonSymbols.cend();
    }

    StringVector _nonSymbols;
};


}

XKeyboard::XKeyboard()
    : _deviceId(XkbUseCoreKbd)
{

    XkbIgnoreExtension(False);

    char* displayName = strdup("");
    int eventCode;
    int errorReturn;
    int major = XkbMajorVersion;
    int minor = XkbMinorVersion;;
    int reasonReturn;
    _display = XkbOpenDisplay(displayName, &eventCode, &errorReturn, &major,
                              &minor, &reasonReturn);
    free(displayName);
    switch (reasonReturn) {
    case XkbOD_BadLibraryVersion:
        throw std::runtime_error("Bad XKB library version.");
    case XkbOD_ConnectionRefused:
        throw std::runtime_error("Connection to X server refused.");
    case XkbOD_BadServerVersion:
        throw std::runtime_error("Bad X11 server version.");
    case XkbOD_NonXkbServer:
        throw std::runtime_error("XKB not present.");
    }
    assert(reasonReturn == XkbOD_Success);

    initializeXkb();

    XkbSelectEventDetails(_display, XkbUseCoreKbd, XkbStateNotify,
                          XkbAllStateComponentsMask, XkbGroupStateMask);

    XkbStateRec xkbState;
    XkbGetState(_display, _deviceId, &xkbState);
    _currentGroupNum = (_currentGroupNum != xkbState.group) ? xkbState.group : _currentGroupNum;
    accomodateGroupXkb();
}

void XKeyboard::initializeXkb()
{
    int major = XkbMajorVersion;
    int minor = XkbMinorVersion;
    int opCode;
    /*auto status =*/ XkbQueryExtension(_display, &opCode, &_baseEventCode, &_baseErrorCode,  &major, &minor);

    XkbDescRec* kbdDescPtr = XkbAllocKeyboard();
    if (!kbdDescPtr) {
        std::runtime_error("Failed to get keyboard description.");
    }

    kbdDescPtr->dpy = _display;
    if (_deviceId != XkbUseCoreKbd) {
        kbdDescPtr->device_spec = _deviceId;
    }

    XkbGetControls(_display, XkbAllControlsMask, kbdDescPtr);
    XkbGetNames(_display, XkbSymbolsNameMask, kbdDescPtr);
    XkbGetNames(_display, XkbGroupNamesMask, kbdDescPtr);

    if (kbdDescPtr->names == NULL) {
        XFree(kbdDescPtr);
        XkbFreeControls(kbdDescPtr, XkbAllControlsMask, true);
        XkbFreeNames(kbdDescPtr, XkbSymbolsNameMask, true);
        XkbFreeNames(kbdDescPtr, XkbGroupNamesMask, true);
        std::runtime_error("Failed to get keyboard description.");
    }

    // Count the number of configured groups.
    const Atom* groupSource = kbdDescPtr->names->groups;
    if (kbdDescPtr->ctrls != NULL) {
        _groupCount = kbdDescPtr->ctrls->num_groups;
    } else {
        _groupCount = 0;
        while (_groupCount < XkbNumKbdGroups &&
               groupSource[_groupCount] != None) {
            _groupCount++;
        }
    }

    // There is always at least one group.
    if (_groupCount == 0) {
        _groupCount = 1;
    }

    // Get the group names.
    const Atom* tmpGroupSource = kbdDescPtr->names->groups;
    Atom curGroupAtom;
    std::string groupName;
    for (int i = 0; i < _groupCount; i++) {
        if ((curGroupAtom = tmpGroupSource[i]) != None) {
            char* groupNameC = XGetAtomName(_display, curGroupAtom);
            if (groupNameC == NULL) {
                _groupNames.push_back("");
            } else {
                groupName = groupNameC;
                // Remove everything after a brace, e.g. "English(US)" -> "English"
                std::string::size_type pos = groupName.find('(', 0);
                if (pos != std::string::npos) {
                    groupName = groupName.substr(0, pos - 1);
                }
                _groupNames.push_back(groupName);
            }
            XFree(groupNameC);
        }
    }

    // Get the symbol name and parse it for layout symbols.
    Atom symNameAtom = kbdDescPtr->names->symbols;
    std::string symName;
    if (symNameAtom != None) {
        char* symNameC = XGetAtomName(_display, symNameAtom);
        symName = symNameC;
        XFree(symNameC);
        if (symName.empty()) {
            XFree(kbdDescPtr);
            XkbFreeControls(kbdDescPtr, XkbAllControlsMask, true);
            XkbFreeNames(kbdDescPtr, XkbSymbolsNameMask, true);
            XkbFreeNames(kbdDescPtr, XkbGroupNamesMask, true);
            throw std::runtime_error("symName is empty");
        }
    } else {
        XFree(kbdDescPtr);
        XkbFreeControls(kbdDescPtr, XkbAllControlsMask, true);
        XkbFreeNames(kbdDescPtr, XkbSymbolsNameMask, true);
        XkbFreeNames(kbdDescPtr, XkbGroupNamesMask, true);
        throw std::runtime_error("symNameAtom is None");
    }

    XkbSymbolParser symParser;

    // Parser is supposed to extract layout symbols (like us, de, ru)
    // and variants (like dvorak) from symName string. But sometimes 
    // it gets nothing, for example on my system with Xwayland symName 
    // is "(unnamed)" and both _symbolNames and _variantNames are empty.
    symParser.parse(symName, _symbolNames, _variantNames);
    int symNameCount = _symbolNames.size();

    if (symNameCount == 1 && _groupNames[0].empty() && _symbolNames[0] == "jp") {
        _groupCount = 2;
        _symbolNames.resize(_groupCount);
        _groupNames.resize(_groupCount);
        _symbolNames[1] = _symbolNames[0];
        _symbolNames[0] = "us";
        _groupNames[0] = "US/ASCII";
        _groupNames[1] = "Japanese";
    } else {
        // If there are less symbol names, than groups, 
        // fill remaining with default name of "en_US".
        if (symNameCount < _groupCount) {
            while (_symbolNames.size() < (size_t)_groupCount) {
                _symbolNames.insert(_symbolNames.begin(), "en_US");
            }
        }
    }

    int groupNameCount = _groupNames.size();
    for (int i = 0; i < groupNameCount; i++) {
        if (_groupNames[i].empty()) {
            std::string name = getSymbolNameByResNum(i);
            if (name.empty()) {
                name = "U/A";
            }
            PLOGW << "Group Name " << i + 1 << " is undefined, set to '" << name << "'!\n";
            _groupNames[i] = name;
        }
    }

    XkbStateRec xkbState;
    XkbGetState(_display, _deviceId, &xkbState);
    _currentGroupNum = xkbState.group;
    XFree(kbdDescPtr);
    XkbFreeControls(kbdDescPtr, XkbAllControlsMask, true);
    XkbFreeNames(kbdDescPtr, XkbSymbolsNameMask, true);
    XkbFreeNames(kbdDescPtr, XkbGroupNamesMask, true);
}

std::string XKeyboard::getSymbolNameByResNum(int groupResNum)
{
    return _symbolNames.at(groupNumResToXkb(groupResNum));
}

std::string XKeyboard::getGroupNameByResNum(int groupResNum)
{
    return _groupNames.at(groupNumResToXkb(groupResNum));
}

int XKeyboard::groupNumResToXkb(int groupResNum)
{
    return groupLookup(groupResNum, _groupNames, _symbolNames, _groupCount);
}

int XKeyboard::groupLookup(int srcValue, StringVector fromText, StringVector toText, int count)
{
    const std::string srcText = fromText.at(srcValue);

    if (!srcText.empty()) {
        std::string targetText;

        for (int i = 0; i < count; i++) {
            targetText = toText.at(i);
            if (compareNoCase(srcText, targetText) == 0) {
                srcValue = i;
                break;
            }
        }
    }

    return srcValue;
}

void XKeyboard::accomodateGroupXkb()
{
    XkbStateRec state;
    XkbGetState(_display, _deviceId, &state);
    _currentGroupNum = state.group;
}


XKeyboard::~XKeyboard()
{
    XCloseDisplay(_display);
    _display = NULL;
}

int XKeyboard::groupCount() const
{
    return _groupCount;
}

const StringVector& XKeyboard::groupNames() const noexcept
{
    return _groupNames;
}

const StringVector& XKeyboard::groupSymbols() const noexcept
{
    return _symbolNames;
}

const StringVector& XKeyboard::groupVariants() const noexcept
{
    return _variantNames;
}

unsigned int XKeyboard::currentGroupNum() const
{
    XkbStateRec xkbState;
    XkbGetState(_display, _deviceId, &xkbState);
    return static_cast<unsigned int>(xkbState.group);
}

const std::string& XKeyboard::currentGroupName() const
{
    return Get(_groupNames, currentGroupNum());
}

const std::string& XKeyboard::currentGroupSymbol() const
{
    return Get(_symbolNames, currentGroupNum());
}

const std::string& XKeyboard::currentGroupVariant() const
{
    return Get(_variantNames, currentGroupNum());
}

bool XKeyboard::setGroupByNum(unsigned int groupNum)
{
    if (_groupCount <= 1) {
        return false;
    }

    const auto result = XkbLockGroup(_display, _deviceId, groupNum);
    if (result == False) {
        return false;
    }
    accomodateGroupXkb();
    return true;
}

bool XKeyboard::changeGroup(int increment)
{
    const auto result = XkbLockGroup(_display, _deviceId, (_currentGroupNum + increment) % _groupCount);
    if (result == False) {
        return false;
    }
    accomodateGroupXkb();
    return true;
}
