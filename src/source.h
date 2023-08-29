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
#include "briebase.tab.h"
#include <cstddef>
#include <memory>

namespace brie {

// Prefix for the config string for the source attached to in-process memory.
constexpr const char* PrefixMalloc = "malloc:";

// Prefix for the config string for the source attached to a SysV shared
// memory segment.
constexpr const char* PrefixSysVShMem = "sysvshmem:";

//=======================================================================
class Source
{
public:
    Source(const char* name) : m_ptr(nullptr), m_size(0), m_pos(0), m_name(name) {}
    virtual ~Source() {}

    virtual void init(const char* cfg) = 0;

    const char* ptr() const { return m_ptr; }
    size_t size() const { return m_size; }
    size_t pos() const { return m_pos; }
    const std::string& name() const { return m_name; }

    int64_t read_int(Type type);
    double  read_float(Type type);
    std::string read_str(size_t len = 0);
    std::string read_wstr(size_t len = 0);

    void set_size(size_t value) { m_size = value; }
    void set_pos(size_t value);
    void set_test_pos(const char* str);
    void set_name(const char* str) { m_name = str; }

    static size_t nopos() { return UINT64_MAX; }
    size_t find(const char* str, size_t maxOffset);

protected:
    const char* m_ptr;
    size_t m_size;
    size_t m_pos;
    std::string m_name;
};

using SourcePtr = std::shared_ptr<Source>;

//=======================================================================
class MallocSource : public Source
{
public:
    MallocSource(const char* name) : Source(name) {}
    virtual ~MallocSource();
    virtual void init(const char* cfg) override;
};

//=======================================================================
class MmapBase : public Source
{
public:
    MmapBase(const char* name);
    virtual ~MmapBase();

protected:
    int m_fd;

protected:
    void map_fd();
};

//=======================================================================
class FileSource : public MmapBase
{
public:
    FileSource(const char* name) : MmapBase(name) {}
    virtual void init(const char* cfg) override;
};

//=======================================================================
class PosixShMeSource : public MmapBase
{
public:
    PosixShMeSource(const char* name) : MmapBase(name) {}
    virtual void init(const char* cfg) override;
};

//=======================================================================
class SysVShMemSource : public Source
{
public:
    SysVShMemSource(const char* name);
    virtual ~SysVShMemSource();
    virtual void init(const char* cfg) override;
private:
    int m_shmid;
};

/*****************************************************************
 *   Create a source object based on the "cfg" string content.
 */
SourcePtr make_source(const char* cfg);

} // namespace brie
