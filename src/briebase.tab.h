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

/**********************************************
 * Parser definitions for brie DSL
 **********************************************/
#pragma once
#include <string>

// Utility class to safely remove strdup-ed string
class DataCleaner
{
public:
    DataCleaner(const void* t) : m_data(t) {}
    ~DataCleaner() { if (m_data) free((void*)m_data); }
private:
    const void* m_data;
};

// Supported data types for reading binary data
enum Type {
    UNDEFINED_TYPE = 3000,
    U8, U16, U32, I16, I32, I64, F32, F64, STRING,
    WSTRING, VOID, FUNC,
    MAX_TYPE
};

// Supported literals for parsing data definition
enum Literal {
    UNDEFINED_LITERAL = 4000,
    IDENTIFIER, I_CONSTANT, F_CONSTANT, STRING_LITERAL,
    FIXED_LENGTH, ARRAY_SIZE, FUNC_CONSTANT,
    MAX_LITERAL
};

// Callback that executes FLEX generated lexer
int briebase_callback(void* context, char *s, void (*f)(void*, int num, const char *));
