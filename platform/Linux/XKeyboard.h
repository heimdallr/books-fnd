// xkeyboard.h
// Interface for a class to get keyboard layout information and change layouts
// Copyright (C) 2008 by Jay Bromley <jbromley@gmail.com>
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// $Id: XKeyboard.h 29 2008-04-09 21:37:44Z jay $
//
// 2010-01-02 Kristian Setälä added code to retrieve layout variant information

#pragma once

#include <vector>
#include <string>
#include <X11/Xlib.h>


namespace HomeCompa::Platform
{

typedef std::vector<std::string> StringVector;


// XKeyboard -----------------------------------------------------------

class XKeyboard
{
public:
    XKeyboard();
    ~XKeyboard();
    int groupCount() const;
    const StringVector& groupNames() const noexcept;
    const StringVector& groupSymbols() const noexcept;
    const StringVector& groupVariants() const noexcept;
    unsigned int currentGroupNum() const;
    const std::string& currentGroupName() const;
    const std::string& currentGroupSymbol() const;
    const std::string& currentGroupVariant() const;
    bool setGroupByNum(unsigned int groupNum);
    bool changeGroup(int increment);

    //friend std::ostream& operator<<(std::ostream& os, const XKeyboard& xkb);

private:
    void initializeXkb();
    std::string getSymbolNameByResNum(int groupResNum);
    int groupNumResToXkb(int groupNumRes);
    std::string getGroupNameByResNum(int groupResNum);
    int groupLookup(int srcValue, StringVector fromText, StringVector toText, int count);
    void accomodateGroupXkb();

    Display* _display{nullptr};
    int _groupCount{0};
    StringVector _groupNames;
    StringVector _symbolNames;
    StringVector _variantNames;
    int _currentGroupNum{0};

    int _deviceId;
    int _baseEventCode;
    int _baseErrorCode;
};


// XkbSymbolParser -----------------------------------------------------


}
