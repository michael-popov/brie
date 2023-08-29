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
 *   Unit tests to support testing data access by Source-based
 *   classes.
 */

#include "source.h"
#include "source_test.h"
#include "utils/log.h"
#include "gtest/gtest.h"
#include <string.h>

using namespace brie;

TEST(DATA, Basic)
{
    //TempLogLevel tll(LL_DEBUG);
    const size_t Size = 1204;
    MallocSourceTest mst;
    const std::string cfg = mst.make(Size);
    SourcePtr src = make_source(cfg.c_str());
}

TEST(DATA, RetrieveNumbers)
{
    //TempLogLevel tll(LL_DEBUG);

    const size_t Size = 1204;
    MallocSourceTest mst;
    const std::string cfg = mst.make(Size);

    size_t  check = 0;
    size_t  pos = 0;
    uint8_t  ku8  = 1;    add_number(ku8,  mst.ptr(), pos);  check += sizeof(ku8);
    uint16_t ku16 = 2;    add_number(ku16, mst.ptr(), pos);  check += sizeof(ku16);
    uint32_t ku32 = 3;    add_number(ku32, mst.ptr(), pos);  check += sizeof(ku32);
    int16_t  ki16 = -5;   add_number(ki16, mst.ptr(), pos);  check += sizeof(ki16);
    int32_t  ki32 = -6;   add_number(ki32, mst.ptr(), pos);  check += sizeof(ki32);
    int64_t  ki64 = -7;   add_number(ki64, mst.ptr(), pos);  check += sizeof(ki64);
    float    kf32 = -8.1; add_number(kf32, mst.ptr(), pos);  check += sizeof(kf32);
    double   kf64 = 9.2;  add_number(kf64, mst.ptr(), pos);  check += sizeof(kf64);
    ASSERT_EQ(pos, check);

    SourcePtr src = make_source(cfg.c_str());

    check = 0; ASSERT_EQ(check, src->pos());
    uint8_t  tu8  =  src->read_int(U8);     check += sizeof(ku8);  ASSERT_EQ(check, src->pos());  ASSERT_EQ(ku8, tu8);
    uint16_t tu16  = src->read_int(U16);    check += sizeof(ku16); ASSERT_EQ(check, src->pos());  ASSERT_EQ(ku16, tu16);
    uint32_t tu32  = src->read_int(U32);    check += sizeof(ku32); ASSERT_EQ(check, src->pos());  ASSERT_EQ(ku32, tu32);
    int16_t  ti16  = src->read_int(I16);    check += sizeof(ki16); ASSERT_EQ(check, src->pos());  ASSERT_EQ(ki16, ti16);
    int32_t  ti32  = src->read_int(I32);    check += sizeof(ki32); ASSERT_EQ(check, src->pos());  ASSERT_EQ(ki32, ti32);
    int64_t  ti64  = src->read_int(I64);    check += sizeof(ki64); ASSERT_EQ(check, src->pos());  ASSERT_EQ(ki64, ti64);
    float    tf32  = src->read_float(F32);  check += sizeof(kf32); ASSERT_EQ(check, src->pos());  ASSERT_EQ(kf32, tf32);
    double   tf64  = src->read_float(F64);  check += sizeof(kf64); ASSERT_EQ(check, src->pos());  ASSERT_EQ(kf64, tf64);
}

TEST(DATA, RetrieveString)
{
    //TempLogLevel tll(LL_DEBUG);

    const size_t Size = 1204;
    MallocSourceTest mst;
    const std::string cfg = mst.make(Size);

    size_t checks[] = { 3, 4, 4, 3};
    size_t lens[sizeof(checks)];
    size_t len = 0;
    for (size_t i=0; i < sizeof(checks)/sizeof(*checks); i++) {
        len += checks[i];
        lens[i] = len;
    }
    const char* strs[] = { "one", "two", "bbc", "fox" };

    size_t pos = 0;
    memcpy(mst.ptr() + pos, strs[0], strlen(strs[0]));     pos += strlen(strs[0]);     ASSERT_EQ(lens[0], pos);
    memcpy(mst.ptr() + pos, strs[1], strlen(strs[1]) + 1); pos += strlen(strs[1]) + 1; ASSERT_EQ(lens[1], pos);
    memcpy(mst.ptr() + pos, strs[2], strlen(strs[2]) + 1); pos += strlen(strs[2]) + 1; ASSERT_EQ(lens[2], pos);
    memcpy(mst.ptr() + pos, strs[3], strlen(strs[3]));     pos += strlen(strs[3]);     ASSERT_EQ(lens[3], pos);
    ASSERT_EQ(len, pos);

    // for (size_t i = 0; i < len; i++) { printf("%d ", (int)(mst.ptr()[i])); } printf("\n\n");

    SourcePtr src = make_source(cfg.c_str());
    ASSERT_EQ(0, src->pos());

    std::string s;
    s = src->read_str(strlen(strs[0])); ASSERT_EQ(std::string(strs[0]), s); ASSERT_EQ(lens[0], src->pos());
    s = src->read_str();                ASSERT_EQ(std::string(strs[1]), s); ASSERT_EQ(lens[1], src->pos());
    s = src->read_str();                ASSERT_EQ(std::string(strs[2]), s); ASSERT_EQ(lens[2], src->pos());
    s = src->read_str(strlen(strs[3])); ASSERT_EQ(std::string(strs[3]), s); ASSERT_EQ(lens[3], src->pos());
    ASSERT_EQ(len, src->pos());
}
