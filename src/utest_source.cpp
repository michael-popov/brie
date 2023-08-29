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
 *   Unit tests to support testing Source and SourceTest integration.
 */

#include "source_test.h"
#include "utils/log.h"
#include "gtest/gtest.h"

using namespace brie;

TEST(SOURCE, Malloc)
{
    //TempLogLevel tll(LL_DEBUG);

    const size_t Size = 1024;

    MallocSourceTest mst;
    const std::string cfg = mst.make(Size);

    ASSERT_EQ(Size, mst.size());
    ASSERT_NE(nullptr, mst.ptr());

    char* ptr = mst.ptr();
    for (size_t i = 0; i < mst.size(); i++) {
        ptr[i] = i % 127;
    }

    SourcePtr src = make_source(cfg.c_str());

    ASSERT_EQ(Size, src->size());
    ASSERT_EQ(ptr, src->ptr()) << "\n\n  cfg=[" << cfg << "]\n\n";
    ASSERT_EQ(0, memcmp(src->ptr(), ptr, Size));
}

TEST(SOURCE, File)
{
    //TempLogLevel tll(LL_DEBUG);

    const size_t Size = 1024;
    char* ptr = (char*)malloc(Size);
    for (size_t i = 0; i < Size; i++) {
        ptr[i] = i % 127;
    }

    const char* path = "/tmp/file_utest.bin";
    FileSourceTest fst;
    const std::string cfg = fst.make(path);

    size_t ret = write(fst.fd(), ptr, Size);
    ASSERT_EQ(Size, ret);

    SourcePtr src = make_source(cfg.c_str());

    ASSERT_EQ(Size, src->size());
    ASSERT_EQ(0, memcmp(src->ptr(), ptr, Size));
}

TEST(SOURCE, ShMem)
{
    TempLogLevel tll(LL_DEBUG);

    const size_t Size = 1024;

    const char* path = "/shm_utest.bin";
    ShmSourceTest fst;
    const std::string cfg = fst.make(path, Size);

    ASSERT_EQ(Size, fst.size());

    char* ptr = fst.ptr();
    for (size_t i = 0; i < Size; i++) {
        ptr[i] = i % 127;
    }

    SourcePtr src = make_source(cfg.c_str());

    ASSERT_TRUE(src);

    ASSERT_EQ(Size, src->size());
    ASSERT_EQ(0, memcmp(src->ptr(), fst.ptr(), Size));
}

TEST(SOURCE, SysVShMem)
{
    TempLogLevel tll(LL_DEBUG);

    const size_t Size = 1024;

    const char* path = "/tmp/sysVshm_utest.bin";
    IpcSourceTest fst;
    const std::string cfg = fst.make(path, Size);

    ASSERT_EQ(Size, fst.size());

    char* ptr = fst.ptr();
    for (size_t i = 0; i < Size; i++) {
        ptr[i] = i % 127;
    }

    SourcePtr src = make_source(cfg.c_str());

    ASSERT_TRUE(src);

    ASSERT_EQ(Size, src->size());
    ASSERT_EQ(0, memcmp(src->ptr(), fst.ptr(), Size));
}



