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

#include "source.h"
#include "source_test.h"
#include "error.h"
#include "utils/log.h"

#include <cassert>
#include <cstring>

#include <stdlib.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>

namespace brie {

static const char* TestPrefix = "test:";
static const size_t TestPrefixLen = 5;


SourcePtr make_source(const char* cfg)
{
    assert(cfg != nullptr);

    if (strstr(cfg, TestPrefix) == cfg) {
        return make_test_source(cfg + TestPrefixLen);
    }

    // Prefix for source config
    constexpr size_t prefixMallocLen = strlen(PrefixMalloc);
    constexpr size_t prefixSysVShMemLen = strlen(PrefixSysVShMem);

    if (strstr(cfg, PrefixMalloc) == cfg) {
        SourcePtr result = std::make_shared<MallocSource>(cfg);
        result->init(cfg + prefixMallocLen);
        return result;
    } else if (strstr(cfg, PrefixSysVShMem) == cfg) {
        SourcePtr result = std::make_shared<SysVShMemSource>(cfg);
        result->init(cfg + prefixSysVShMemLen);
        return result;
    }

    struct stat st;
    int ret = stat(cfg, &st);
    if (ret != 0) {
        SourcePtr result = std::make_shared<PosixShMeSource>(cfg);
        result->init(cfg);
        return result;
    }

    if (S_ISREG(st.st_mode)) {
        SourcePtr result = std::make_shared<FileSource>(cfg);
        result->init(cfg);
        return result;
    }

    return nullptr;
}

/**********************************************************************
 */
static size_t type_length(Type type)
{
    switch (type) {
        case U8:  return sizeof(uint8_t);
        case U16: return sizeof(uint16_t);
        case U32: return sizeof(uint32_t);
        case I16: return sizeof(int16_t);
        case I32: return sizeof(int32_t);
        case I64: return sizeof(int64_t);
        case F32: return sizeof(float);
        case F64: return sizeof(double);
        default: throwex("Invalid type");
    }

    return 0;
}

int64_t Source::read_int(Type type)
{
    size_t len = type_length(type);
    if (m_pos + len > m_size) throwex("Insufficent data in source");

    const char* data = m_ptr + m_pos;

    switch (type) {
        case U8:  {
            uint8_t val;
            memcpy(&val, data, len);
            m_pos += len;
            return static_cast<int64_t>(val);
        }
        case U16:  {
            uint16_t val;
            memcpy(&val, data, len);
            m_pos += len;
            return static_cast<int64_t>(val);
        }
        case U32:  {
            uint32_t val;
            memcpy(&val, data, len);
            m_pos += len;
            return static_cast<int64_t>(val);
        }
        case I16:  {
            int16_t val;
            memcpy(&val, data, len);
            m_pos += len;
            return static_cast<int64_t>(val);
        }
        case I32:  {
            int32_t val;
            memcpy(&val, data, len);
            m_pos += len;
            return static_cast<int64_t>(val);
        }
        case I64:  {
            int64_t val;
            memcpy(&val, data, len);
            m_pos += len;
            return val;
        }

        default: throwex("Invalid type");
    }

    return 0;
}

double  Source::read_float(Type type)
{
    size_t len = type_length(type);
    if (m_pos + len > m_size) throwex("Insufficent data in source");

    const char* data = m_ptr + m_pos;

    switch (type) {
        case F32:  {
            float val;
            memcpy(&val, data, len);
            m_pos += len;
            return val;
        }
        case F64:  {
            double val;
            memcpy(&val, data, len);
            m_pos += len;
            return val;
        }

        default: throwex("Invalid type");
    }

    return 0.0;
}

std::string Source::read_str(size_t len)
{
    bool is_null_terminated = false;

    size_t alterLen = 0;
    if (len == 0) {
        is_null_terminated = true;
    
        for (size_t pos=m_pos; pos < m_size; pos++, len++) {
            if (m_ptr[pos] == '\0') break;
        }
    } else { // if len != 0
        // Cover possible \0 padding included in len
        size_t tempLen = 0;
        for (size_t pos=m_pos; pos < m_pos + len; pos++, tempLen++) {
            if (m_ptr[pos] == '\0') {
                alterLen = tempLen;
                break;
            }
        }
    }

    if (m_pos + len >= m_size) throwex("No available data: string is too long");

    if (alterLen == 0) alterLen = len;
    std::string str(m_ptr + m_pos, alterLen);
    //std::string str(m_ptr + m_pos, len);

    m_pos += len;
    if (is_null_terminated) m_pos += 1;

    return str;
}

std::string Source::read_wstr(size_t len)
{
    bool is_null_terminated = false;

    len *= 2;
    size_t prefixLen = 0;
    size_t alterLen = 0;

    const char* s = m_ptr + m_pos;
    const char FF = 0xFF;
    const char FE = 0xFE;

    if (*s == FF || *s == FE) {
        s += 2;
        prefixLen = 2;
    }

    if (len == 0) {
        is_null_terminated = true;

        for (size_t i = m_pos + prefixLen; i < m_size; i += 2, len += 2) {
            if (m_ptr[i] == '\0') {
                if (i >= m_size - 1) throwex("Invalid format of wstr: no terminator");
                if (m_ptr[i+1] == '\0') break;
            }
        }
    } else { // if len != 0
        // Cover possible \0 padding included in len
        size_t tempLen = 0;
        for (size_t i = m_pos + prefixLen; i < m_pos + len; i += 2, tempLen += 2) {
            if (i >= m_size - 1) break;

            if (m_ptr[i] == '\0' && m_ptr[i+1] == 0) {
                alterLen = tempLen;
                break;
            }
        }
    }

    if (m_pos + len > m_size) throwex("No available data: wstring is too long");

    len -= prefixLen;
    if (alterLen != 0) len = alterLen;
    std::string str(s, len);

    m_pos += len + prefixLen;
    if (is_null_terminated) m_pos += 2;

    return str;
}

void Source::set_pos(size_t value)
{
    if (value > m_size) throwex("Position outside of data space");
    m_pos = value;
}

void Source::set_test_pos(const char* str)
{
    if (strcmp("test:strings", str) == 0) set_pos(testOffsets.strings);
    else if (strcmp("test:u32array", str) == 0) set_pos(testOffsets.u32array);

    else if (strcmp("test:fixedStringsArray", str) == 0) set_pos(testOffsets.fixedStringsArray);
    else if (strcmp("test:zeroTermStringsArray", str) == 0) set_pos(testOffsets.zeroTermStringsArray);

    else if (strcmp("test:fixedWstrArray", str) == 0) set_pos(testOffsets.fixedWstrArray);
    else if (strcmp("test:zeroTermWstrArray", str) == 0) set_pos(testOffsets.zeroTermWstrArray);

    else throwex("Invalid argument");
}

size_t Source::find(const char* str, size_t maxOffset)
{
    if (str == nullptr || *str == '\0') return nopos();

    size_t len = strlen(str);
    if (len > m_size || len > maxOffset) return nopos();

    for (size_t i = m_pos; i < m_size - len && i < maxOffset - len; i++) {
        if (m_ptr[i] == *str) {
            if (0 == memcmp(m_ptr + i, str, len)) return i;
        }
    }

    return nopos();
}

/**********************************************************************
 */
MallocSource::~MallocSource()
{
}

void MallocSource::init(const char* cfg)
{
    int ret = sscanf(cfg, "%p %lu", &m_ptr, &m_size);
    if (ret != 2) throwex("Source: invalid malloc config");
}

/**********************************************************************
 */
MmapBase::MmapBase(const char* name)
  : Source(name), m_fd(-1)
{

}

MmapBase::~MmapBase()
{
    if (m_ptr != nullptr && m_ptr != MAP_FAILED) {
        munmap((void*)m_ptr, m_size);
    }

    if (m_fd != -1) {
        close(m_fd);
    }
}

void MmapBase::map_fd()
{
    struct stat sb;
    int ret = fstat(m_fd, &sb);
    if (ret < 0) {
        close(m_fd);
        m_fd = -1;
        throwex("Source: failed stat mmap");
    }

    m_size = sb.st_size;
    m_ptr = (const char*)mmap(NULL, m_size, PROT_READ, MAP_PRIVATE, m_fd, 0);
    if (m_ptr == MAP_FAILED) {
        close(m_fd);
        m_fd = -1;
        throwex("Source: failed map");
    }
}

/**********************************************************************
 */
void FileSource::init(const char* cfg)
{
    m_fd = open(cfg, O_RDONLY);
    if (m_fd < 0) throwex("Source: failed to open file");

    map_fd();
}


/**********************************************************************
 */
void PosixShMeSource::init(const char* cfg)
{
    m_fd = shm_open(cfg, O_RDONLY, 0);
    std::string err = "Source: failed to open shared memory: ";
    err += cfg;
    if (m_fd < 0) throwex(err);

    map_fd();
}

/**********************************************************************
 */
SysVShMemSource::SysVShMemSource(const char* name)
  : Source(name), m_shmid(-1)
{

}

SysVShMemSource::~SysVShMemSource()
{
    if (m_ptr != MAP_FAILED) {
        shmdt((void*)m_ptr);        
    }
}

void SysVShMemSource::init(const char* cfg)
{
    int ret = sscanf(cfg, "%d", &m_shmid);
    if (ret != 1) throwex("Source: invalid sysvshmem config");

    shmid_ds ds;
    ret = shmctl(m_shmid, IPC_STAT, &ds);
    if (ret < 0) throwex("Source: invalid sysvshmem id");
    m_size = ds.shm_segsz;

    m_ptr = (const char*)shmat(m_shmid, nullptr, SHM_RDONLY);
    if (m_ptr == MAP_FAILED) throwex("Source: failed to attach sysvshmem");
}

} // namespace brie
