/* 
 * This file is part of the BRIE distribution (https://github.com/michael-popov/brie).
 * Copyright (c) 2023 Michael Popov.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
#include "structs.h"
#include "briebase.tab.h"
#include "error.h"
#include <string>
#include <vector>

namespace brie {

/********************************************************
 *   Base class for parsing read and decl strings.
 *   Technically it is not quite parser but lexer.
 */
class ParserBase
{
public:
    static void on_token(void* context, int num, const char *str);

    virtual void push_type(Type) { throwex("ParserBase::push_type not impleneted"); }
    virtual void push_identifier(const char*) { throwex("ParserBase::push_identifier not impleneted"); }
    virtual void push_value(int /*valueType*/, const char*) { throwex("ParserBase::push_value not impleneted"); }
    virtual void push_symbol(int) { throwex("ParserBase::push_symbol not impleneted"); }
};

/********************************************************
 *   Class for parsing read definition string.
 */
class ReadStringParser : public ParserBase
{
public:
    ReadStringParser(DataItemList& dataItems);

    virtual void push_type(Type) override;
    virtual void push_identifier(const char*) override;
    virtual void push_value(int /*valueType*/, const char*) override;

    void complete();

private:
    enum class State { Initial, Type, Name, FixedLength, ArraySize, };
private:
    DataItemList& m_dataItems;
    DataItem m_current;
    State m_state = State::Initial;
};

/********************************************************
 *   Class for parsing identifiers.
 */
class NameParser : public ParserBase
{
public:
    virtual void push_identifier(const char*) override;
    const std::string& name() const { return m_name; }
private:
    std::string m_name;
};

/********************************************************
 *   Class for parsing field definitions for declaring
 *   a struct.
 */
class FieldsParser : public ParserBase
{
public:
    FieldsParser(FieldList& fields);

    virtual void push_type(Type) override;
    virtual void push_identifier(const char*) override;
    virtual void push_value(int /*valueType*/, const char*) override;
    virtual void push_symbol(int) override;

    void complete();

private:
    enum class State { Initial, Type, FixedLength, ArraySize, Colon, Name, };
private:
    FieldList& m_fields;
    Field m_current;
    State m_state = State::Initial;
};

// Parse a string provided in read() call
void parse_read_str(const char* str, DataItemList& dataItems);

// Parse a name provided in decl() call
std::string parse_name_str(const char* str);

// Parse a structure declaration provided in decl() call
void parse_fields_str(const char* str, FieldList& fields);

} // namespace brie
