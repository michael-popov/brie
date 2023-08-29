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
#include "source.h"
#include <string>
#include <string.h>

namespace brie {

/*****************************************************************
 *   Special classes to support tests. They create different types
 *   of sources: in-process memory, files, shared memory segments.
 */

/*****************************************************************
 *   Offsets to support access to different parts of test data.
 */
struct TestOffsets
{
    size_t strings = 0;
    size_t u32array = 0;
    size_t fixedStringsArray = 0;
    size_t zeroTermStringsArray = 0;
    size_t fixedWstrArray = 0;
    size_t zeroTermWstrArray = 0;
};
extern TestOffsets testOffsets;

/*****************************************************************
 *   Test sources.
 *   Method make() returns a "connection" string that can be used
 *   to create a Source object for the prepared test data.
 */

//=======================================================================
class MallocSourceTest
{
public:
    MallocSourceTest();
    ~MallocSourceTest();

    std::string make(size_t size);

    char* ptr() const { return m_ptr; }
    size_t size() const { return m_size; }

private:
    char* m_ptr;
    size_t m_size;
};

//=======================================================================
class FileSourceTest
{
public:
    ~FileSourceTest();

    std::string make(const char* path);

    int fd() const { return m_fd; }

private:
    int m_fd = -1;
    std::string m_path;
};

//=======================================================================
class ShmSourceTest
{
public:
    ~ShmSourceTest();

    std::string make(const char* path, size_t size);

    char* ptr() const { return m_ptr; }
    size_t size() const { return m_size; }

private:
    int m_fd = -1;
    char* m_ptr = nullptr;
    size_t m_size = 0;
    std::string m_path;
};

//=======================================================================
class IpcSourceTest
{
    const int ShMemId = 42;

public:
    ~IpcSourceTest();

    std::string make(const char* path, size_t size);

    char* ptr() const { return m_ptr; }
    size_t size() const { return m_size; }

private:
    int m_key = -1;
    int m_shmid = -1;
    char* m_ptr = nullptr;
    size_t m_size = 0;
    std::string m_path;
};

//=======================================================================
SourcePtr make_test_source(const char* cfg);

template <class T>
void add_number(T num, char* ptr, size_t& pos)
{
    memcpy(ptr+pos, &num, sizeof(T));
    pos += sizeof(T);
}

} // namespace brie
