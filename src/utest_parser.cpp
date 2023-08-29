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

/*****************************************************************
 *   Unit tests to support parser testing.
 */

#include "parser.h"
#include "utils/log.h"
#include "gtest/gtest.h"
#include <string.h>

using namespace brie;

TEST(PARSER, Basic)
{
    DataItemList dataItems;
    const char* str = "u8 u16 u32 i16 i32 i64 f32 f64 str";
    bool ok = true;
    try {
        parse_read_str(str, dataItems);
    } catch (...) {
        ok = false;
    }

    ASSERT_TRUE(ok);

    Type types[] = { U8, U16, U32, I16, I32, I64, F32, F64, STRING };
    ASSERT_EQ(sizeof(types)/sizeof(*types), dataItems.size());


    for (size_t i = 0; i < dataItems.size(); i++) {
        ASSERT_EQ(types[i], dataItems[i].type);
        ASSERT_EQ(0, dataItems[i].size);
        ASSERT_EQ(1, dataItems[i].count);
        ASSERT_TRUE(dataItems[i].typeName.empty());
    }
}

TEST(PARSER, Arrays)
{
    DataItemList dataItems;
    const char* str = "u8*8 u16*16 u32*32 i16*16 i32*32 i64*64 f32*32 f64*64 str*128";
    bool ok = true;
    try {
        parse_read_str(str, dataItems);
    } catch (...) {
        ok = false;
    }

    ASSERT_TRUE(ok);

    Type types[] = { U8, U16, U32, I16, I32, I64, F32, F64, STRING };
    ASSERT_EQ(sizeof(types)/sizeof(*types), dataItems.size());

    size_t lens[] = { 8, 16, 32, 16, 32, 64, 32, 64, 128 };

    for (size_t i = 0; i < dataItems.size(); i++) {
        ASSERT_EQ(types[i], dataItems[i].type);
        ASSERT_EQ(0, dataItems[i].size);
        ASSERT_EQ(lens[i], dataItems[i].count);
        ASSERT_TRUE(dataItems[i].typeName.empty());
    }
}

TEST(PARSER, FixedLength)
{
    DataItemList dataItems;
    const char* str = "str#3 str#32*8";
    bool ok = true;
    try {
        parse_read_str(str, dataItems);
    } catch (...) {
        ok = false;
    }

    ASSERT_TRUE(ok);

    Type types[] = { STRING, STRING };
    ASSERT_EQ(sizeof(types)/sizeof(*types), dataItems.size());

    size_t lens[] = { 1, 8 };
    size_t sizes[] = { 3, 32 };

    for (size_t i = 0; i < dataItems.size(); i++) {
        ASSERT_EQ(types[i], dataItems[i].type);
        ASSERT_EQ(sizes[i], dataItems[i].size);
        ASSERT_EQ(lens[i], dataItems[i].count);
        ASSERT_TRUE(dataItems[i].typeName.empty());
    }
}

static void check_invalid_name(const char* name)
{
    bool ok = true;
    try {
        parse_name_str(name);
    } catch (const Error&) {
        ok = false;
    }
    ASSERT_FALSE(ok);
}

TEST(PARSER, Name)
{
    const std::string s1 = "name";
    const std::string s1c = parse_name_str(s1.c_str());
    ASSERT_EQ(s1, s1c);

    check_invalid_name("name ");
    check_invalid_name("u8");
    check_invalid_name("12name");
    check_invalid_name("12");
    check_invalid_name(",aa");
}

TEST(PARSER, BasicStruct)
{
    FieldList fields;
    parse_fields_str("u8:aaa", fields);
    ASSERT_EQ(1, fields.size());
    const Field& f = fields[0];
    ASSERT_EQ(U8, f.type);
    ASSERT_TRUE(f.typeName.empty());
    ASSERT_EQ(1, f.count);
    ASSERT_EQ(0, f.size);
    ASSERT_EQ(std::string("aaa"), f.name);
}

TEST(PARSER, BasicStructDefault)
{
    FieldList fields;
    parse_fields_str("u8:aaa", fields);
    ASSERT_EQ(1, fields.size());
    const Field& f = fields[0];
    ASSERT_EQ(U8, f.type);
    ASSERT_TRUE(f.typeName.empty());
    ASSERT_EQ(1, f.count);
    ASSERT_EQ(0, f.size);
    ASSERT_EQ(std::string("aaa"), f.name);
}

TEST(PARSER, BasicStructCustomType)
{
    FieldList fields;
    parse_fields_str("xyz:aaa", fields);
    ASSERT_EQ(1, fields.size());
    const Field& f = fields[0];
    ASSERT_EQ(std::string("xyz"), f.typeName);
    ASSERT_EQ(1, f.count);
    ASSERT_EQ(0, f.size);
    ASSERT_EQ(std::string("aaa"), f.name);
}

TEST(PARSER, Struct)
{
    FieldList fields;
    const char* s = "u8:a1 u16:a2 u32:a3  i16*8:a4 i32:a5 i64:a6 f32:a7 f64:a8";
    parse_fields_str(s, fields);
    ASSERT_EQ(8, fields.size());

    const Field& f = fields[3];
    ASSERT_EQ(I16, f.type);
    ASSERT_TRUE(f.typeName.empty());
    ASSERT_EQ(8, f.count);
    ASSERT_EQ(0, f.size);
    ASSERT_EQ(std::string("a4"), f.name);
}

TEST(PARSER, Struct2)
{
    FieldList fields;
    const char* s = "str:a1 str#1234:a2 str#16*4:a3 str*8:a4 str#3:a5";
    parse_fields_str(s, fields);
    ASSERT_EQ(5, fields.size());

    const Field& f = fields[2];
    ASSERT_EQ(STRING, f.type);
    ASSERT_TRUE(f.typeName.empty());
    ASSERT_EQ(4, f.count);
    ASSERT_EQ(16, f.size);
    ASSERT_EQ(std::string("a3"), f.name);
}

static void check_struct(const char* str, bool expected)
{
    bool ok = true;
    try {
        FieldList fields;
        parse_fields_str(str, fields);
    } catch (const Error&) {
        ok = false;
    }
    ASSERT_EQ(expected, ok);
}

TEST(PARSER, InvalidStruct)
{
    check_struct("a1", false);
    check_struct("u8:", false);
    check_struct("u8#3:a", false);
    check_struct("u8:123", false);
    check_struct("void#1:aaa", false);
    check_struct("void*3:aaa", false);
    check_struct("@f2#2:aaa", false);
}

TEST(PARSER, ValidStruct)
{
    check_struct("void#1 void#2*3", true);
    check_struct("f32:a1", true);
    check_struct("u8:a1 u16:a2", true);
    check_struct("@f2:aaa @f2:bbb @f3*4:ccc", true);
}

TEST(PARSER, ComplexStructs)
{
    init_structs();

    StructPtr sp1 = std::make_shared<FieldList>();
    parse_fields_str("u8:aaa", *sp1);
    add_struct("one", sp1);

    StructPtr sp2 = std::make_shared<FieldList>();
    parse_fields_str("u8:bbb", *sp2);
    add_struct("two", sp2);

    StructPtr sp3 = std::make_shared<FieldList>();
    parse_fields_str("one:xxx two:yyy", *sp3);
    add_struct("three", sp3);
}

TEST(PARSER, ComplexStructsInvalid1)
{
    init_structs();

    bool ok = true;
    try {
        StructPtr sp1 = std::make_shared<FieldList>();
        parse_fields_str("one:xxx", *sp1);
        add_struct("one", sp1);
    } catch (const Error&) {
        ok = false;
    }
    ASSERT_FALSE(ok);
}

TEST(PARSER, ComplexStructsInvalid2)
{
    TempLogLevel tll(LL_DEBUG);

    init_structs();

    bool ok = true;
    try {
        StructPtr sp1 = std::make_shared<FieldList>();
        parse_fields_str("two:xxx", *sp1);
        add_struct("one", sp1);
    } catch (const Error&) {
        ok = false;
    }
    ASSERT_FALSE(ok);
}

TEST(PARSER, BasicVoid)
{
    DataItemList dataItems;
    const char* str = "void#3 void#32*8";
    bool ok = true;
    try {
        parse_read_str(str, dataItems);
    } catch (...) {
        ok = false;
    }

    ASSERT_TRUE(ok);

    Type types[] = { VOID, VOID };
    ASSERT_EQ(sizeof(types)/sizeof(*types), dataItems.size());

    size_t lens[] = { 1, 8 };
    size_t sizes[] = { 3, 32 };

    for (size_t i = 0; i < dataItems.size(); i++) {
        ASSERT_EQ(types[i], dataItems[i].type);
        ASSERT_EQ(sizes[i], dataItems[i].size);
        ASSERT_EQ(lens[i], dataItems[i].count);
        ASSERT_TRUE(dataItems[i].typeName.empty());
    }
}

TEST(PARSER, StructVoid)
{
    FieldList fields;
    parse_fields_str("void#4 void#4*8", fields);
    ASSERT_EQ(2, fields.size());
    {
        const Field& f = fields[0];
        ASSERT_EQ(VOID, f.type);
        ASSERT_TRUE(f.typeName.empty());
        ASSERT_EQ(1, f.count);
        ASSERT_EQ(4, f.size);
        ASSERT_TRUE(f.name.empty());
    }
    {
        const Field& f = fields[1];
        ASSERT_EQ(VOID, f.type);
        ASSERT_TRUE(f.typeName.empty());
        ASSERT_EQ(8, f.count);
        ASSERT_EQ(4, f.size);
        ASSERT_TRUE(f.name.empty());
    }
}
