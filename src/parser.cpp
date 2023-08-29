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

#include "parser.h"
#include "error.h"
#include "utils/log.h"
#include <unordered_map>
#include <string>
#include <memory>
#include <string.h>

namespace brie {

using DataItemListPtr = std::shared_ptr<DataItemList>;
using ParserCache = std::unordered_map<std::string, DataItemListPtr>;
static ParserCache parserCache;

void clear_data_item(DataItem& item)
{
    item.type = UNDEFINED_TYPE;
    item.size = 0;
    item.count = 1;
    item.typeName.clear();
}

void clear_field(Field& field)
{
    clear_data_item(field);
    field.name.clear();
}

/**************************************************************************
 */
void ParserBase::on_token(void* context, int num, const char *str)
{
    ParserBase* self = static_cast<ParserBase*>(context);

    if (num < UNDEFINED_TYPE) {
        self->push_symbol(num);
        return;
    } 
    
    if (num < MAX_TYPE) {
        self->push_type(static_cast<Type>(num));
        return;
    } 
    
    if (num < MAX_LITERAL) {
        if (num == IDENTIFIER) {
            self->push_identifier(str);
        } else {
            self->push_value(num, str);
        }
        return;
    }

    throwex("Parser: invalid token type");
}

/**************************************************************************
 */
static size_t extract_num(const char* str)
{
    size_t val;
    int ret = sscanf(str+1, "%lu", &val);
    if (ret != 1) throwex("Parser: invalid fixed length or array size value");
    return val;
}

ReadStringParser::ReadStringParser(DataItemList& dataItems)
  : m_dataItems(dataItems)
{
    clear_data_item(m_current);
}

void ReadStringParser::push_type(Type type)
{
    complete();
    if (m_state != State::Initial) throwex("Parser: invalid state");
    m_current.type = static_cast<Type>(type);
    m_state = State::Type;
}

void ReadStringParser::push_identifier(const char* str)
{
    complete();
    if (m_state != State::Initial) throwex("Parser: invalid state");
    m_current.typeName = str;
    m_state = State::Name;
}

void ReadStringParser::push_value(int valueType, const char* str)
{
    switch (valueType) {
        default: throwex("Parser: invalid token type"); break;

        case FIXED_LENGTH:
            if (m_state != State::Type) throwex("Parser: invalid state");
            if (m_current.type != STRING && m_current.type != WSTRING && m_current.type != VOID) {
                throwex("Parser: invalid state");
            }
            m_current.size = extract_num(str);
            m_state = State::FixedLength;
            break;

        case ARRAY_SIZE:
            if (m_state != State::Type && 
                m_state != State::FixedLength && 
                m_state != State::Name) throwex("Parser: invalid state");
            m_current.count = extract_num(str);
            m_state = State::ArraySize;
            break;

        case FUNC_CONSTANT:
            if (m_state != State::Initial) throwex("Parser: invalid state");
            m_current.typeName = str + 1;
            m_current.type = FUNC;
            m_state = State::Type;
            break;
    }
}

void ReadStringParser::complete()
{
    if (m_state != State::Initial) {
        if (m_current.type == VOID && m_current.size == 0) {
            throwex("Parser: invalid state");
        }
        m_dataItems.push_back(m_current);
        clear_data_item(m_current);
        m_state = State::Initial;
    }
}

void parse_read_str(const char* str, DataItemList& dataItems)
{
    const auto iter = parserCache.find(str);
    if (iter != parserCache.end()) {
        dataItems = *iter->second;
        return;
    }

    char* t = strdup(str);
    const DataCleaner dc(t);

    dataItems.clear();
    ReadStringParser parser(dataItems);
    briebase_callback(&parser, t, ParserBase::on_token);
    parser.complete();

    DataItemListPtr p = std::make_shared<DataItemList>();
    *p = dataItems;
    parserCache[str] = p;
}

/**************************************************************************
 */
void NameParser::push_identifier(const char* str)
{
    m_name = str;
}

std::string parse_name_str(const char* str)
{
    char* t = strdup(str);
    const DataCleaner dc(t);

    NameParser parser;
    briebase_callback(&parser, t, ParserBase::on_token);

    if (strlen(str) != parser.name().length()) {
        throwex("Invalid name");        
    }

    return parser.name();
};

/*************************************************************************
 */
FieldsParser::FieldsParser(FieldList& fields)
  : m_fields(fields)
{
    clear_field(m_current);
}

void FieldsParser::push_type(Type type)
{
    if (m_current.type == VOID && 
       (m_state == State::FixedLength || m_state == State::ArraySize)) {
        complete();
    }

    if (m_state != State::Initial) throwex("Parser: invalid state");
    m_current.type = type;
    m_state = State::Type;
}

void FieldsParser::push_identifier(const char* str)
{
    if (m_current.type == VOID && 
       (m_state == State::FixedLength || m_state == State::ArraySize)) {
        complete();
    }

    if (m_state == State::Initial) {
        m_current.typeName = str;
        m_state = State::Type;
    } else if (m_state == State::Colon) {
        m_current.name = str;
        m_state = State::Name;
        complete();
    } else {
        throwex("Parser: invalid state");
    }
}

void FieldsParser::push_value(int valueType, const char* str)
{
    switch (valueType) {
        default:
            throwex("Parser: invalid state");
            break;

        case FIXED_LENGTH:
            if (m_state != State::Type) throwex("Parser: invalid state");
            if (m_current.type != STRING && m_current.type != WSTRING && m_current.type != VOID) {
                throwex("Parser: invalid state");
            }
            m_current.size = extract_num(str);
            m_state = State::FixedLength;
            break;

        case ARRAY_SIZE:
            if (m_state != State::Type && 
                m_state != State::FixedLength) throwex("Parser: invalid state");
            m_current.count = extract_num(str);
            m_state = State::ArraySize;
            break;

        case FUNC_CONSTANT:
            if (m_state != State::Initial) throwex("Parser: invalid state");
            m_current.typeName = str + 1;
            m_current.type = FUNC;
            m_state = State::Type;
            break;
    }
}

void FieldsParser::push_symbol(int sym)
{
    switch (sym) {
        default: throwex("Parser: invalid state"); break;

        case ':':
            if (m_state != State::Type && 
                m_state != State::FixedLength &&
                m_state != State::ArraySize) throwex("Parser: invalid state");
            m_state = State::Colon;
            break;
    }
}

void FieldsParser::complete()
{
    if (m_state == State::Initial) return;

    if (m_current.type == VOID) {
        if (m_state != State::FixedLength && m_state != State::ArraySize) {
            throwex("Parser: invalid state");
        }
        if (m_current.size == 0) throwex("Parser: invalid state");
    } else {
        if (m_state != State::Name) throwex("Parser: invalid state");
        if (m_current.name.empty()) throwex("Parser: invalid state");
    }
    m_fields.push_back(m_current);
    clear_field(m_current);
    m_state = State::Initial;
}

void parse_fields_str(const char* str, FieldList& fields)
{
    char* t = strdup(str);
    const DataCleaner dc(t);

    FieldsParser parser(fields);
    briebase_callback(&parser, t, ParserBase::on_token);

    parser.complete();
}

} // namespace brie
