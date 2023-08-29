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

#include <errno.h>
#include <string.h>

namespace brie {

TestOffsets testOffsets;

/**********************************************************************
 */
MallocSourceTest::MallocSourceTest()
  : m_ptr(nullptr), m_size(0)
{

}

MallocSourceTest::~MallocSourceTest()
{
    if (m_ptr != nullptr) {
        free(m_ptr);
    }
}

std::string MallocSourceTest::make(size_t size)
{
    assert(size > 0);
    m_size = size;
    m_ptr = (char*)malloc(m_size);

    char buf[128];
    snprintf(buf, sizeof(buf), "%s %p %lu", PrefixMalloc, m_ptr, m_size);

    return buf;
}

/**********************************************************************
 */
FileSourceTest::~FileSourceTest()
{
    if (m_fd > 0) {
        close(m_fd);
    }

    if (!m_path.empty()) {
        unlink(m_path.c_str());
    }
}

std::string FileSourceTest::make(const char* path)
{
    assert(path != nullptr);

    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    m_fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, mode);
    if (m_fd < 0) throwex("SourceTest: failed to create file");

    return path;
}

/**********************************************************************
 */
ShmSourceTest::~ShmSourceTest()
{
    if (m_ptr != nullptr && m_ptr != MAP_FAILED) {
        munmap(m_ptr, m_size);
    }

    if (m_fd > 0) {
        close(m_fd);
    }

    if (!m_path.empty()) {
        shm_unlink(m_path.c_str());
    }
}

std::string ShmSourceTest::make(const char* path, size_t size)
{
    assert(path != nullptr);

    m_size = size;

    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    m_fd = shm_open(path, O_RDWR | O_CREAT, mode);
    if (m_fd < 0) {
        LOG_DEBUG << strerror(errno);
        throwex("SourceTest: failed to create shared mem");
    }

    int ret = ftruncate(m_fd, size);
    if (ret < 0) throwex("SourceTest: failed to size shared mem");

    m_ptr = (char*)mmap(NULL, size, PROT_WRITE | PROT_READ, MAP_SHARED, m_fd, 0);
    if (m_ptr == MAP_FAILED) throwex("SourceTest: failed to map shared mem");

    return path;
}

/**********************************************************************
 */
IpcSourceTest::~IpcSourceTest()
{
    if (m_ptr != nullptr && m_ptr != MAP_FAILED) {
        shmdt(m_ptr);
    }

    if (m_shmid != -1) {
        shmctl(m_shmid, IPC_RMID, nullptr);
    }
}

std::string IpcSourceTest::make(const char* path, size_t size)
{
    assert(path != nullptr);

    m_size = size;

    int fd = open(path, O_CREAT, S_IRUSR | S_IWUSR);
    if (fd < 0) throwex("SourceTest: failed to create sysvshmem mem");
    close(fd);

    m_key = ftok(path, ShMemId);
    if (m_key < 0) throwex("SourceTest: failed to create token for sysvshmem mem");

    unlink(path);

    m_shmid = shmget(m_key, size, IPC_CREAT | S_IRWXU);
    if (m_shmid < 0) throwex("SourceTest: failed to get sysvshmem mem");

    m_ptr = (char*)shmat(m_shmid, nullptr, 0);
    if (m_ptr == MAP_FAILED) throwex("SourceTest: failed to attach sysvshmem mem");

    bzero(m_ptr, m_size);

    char buf[64];
    snprintf(buf, sizeof(buf), "%s%d", PrefixSysVShMem, m_shmid);

    return buf;
}

/**********************************************************************
 */
size_t populate_data(char* ptr)
{
    size_t  pos = 0;
    uint8_t  ku8  = 1;    add_number(ku8,  ptr, pos);
    uint16_t ku16 = 2;    add_number(ku16, ptr, pos);
    uint32_t ku32 = 3;    add_number(ku32, ptr, pos);
    int16_t  ki16 = -5;   add_number(ki16, ptr, pos);
    int32_t  ki32 = -6;   add_number(ki32, ptr, pos);
    int64_t  ki64 = -7;   add_number(ki64, ptr, pos);
    float    kf32 = -8.1; add_number(kf32, ptr, pos);
    double   kf64 = 9.2;  add_number(kf64, ptr, pos);

    testOffsets.strings = pos;
    const char* strs[] = { "one", "two", "bbc", "fox" };

    memcpy(ptr + pos, strs[0], strlen(strs[0]));     pos += strlen(strs[0]);
    memcpy(ptr + pos, strs[1], strlen(strs[1]) + 1); pos += strlen(strs[1]) + 1;
    memcpy(ptr + pos, strs[2], strlen(strs[2]) + 1); pos += strlen(strs[2]) + 1;
    memcpy(ptr + pos, strs[3], strlen(strs[3]));     pos += strlen(strs[3]);

    testOffsets.u32array = pos;
    for (int i = 0; i < 8; i++) {
        uint32_t ku32 = 1000 + i;
        add_number(ku32, ptr, pos);
    }

    testOffsets.fixedStringsArray = pos;
    const char* xs[] = { "x1", "x2", "x3" };
    for (size_t i = 0; i < sizeof(xs)/sizeof(*xs); i++) {
        int len = strlen(xs[i]);
        memcpy(ptr + pos, xs[i], len);
        pos += len;
    }

    testOffsets.zeroTermStringsArray = pos;
    const char* ys[] = { "y1", "y2", "y3" };
    for (size_t i = 0; i < sizeof(ys)/sizeof(*ys); i++) {
        int len = strlen(ys[i]) + 1;
        memcpy(ptr + pos, ys[i], len);
        pos += len;
    }

    testOffsets.fixedWstrArray = pos;
    uint8_t zs[] = {
        208, 160, 208, 176, 208, 183,
        208, 148, 208, 178, 208, 176,
        208, 147, 208, 190, 208, 191,
        208, 163, 208, 191, 209, 129
    };
    for (size_t i = 0; i < sizeof(zs)/sizeof(*zs); i++) {
        add_number(zs[i], ptr, pos);
    }

    testOffsets.zeroTermWstrArray = pos;
    uint8_t qs[] = {
        208, 148, 208, 182, 208, 176, 208, 183, 0, 0,
        208, 160, 208, 190, 208, 186, 0, 0,
        208, 154, 208, 187, 208, 176, 209, 129, 209, 129, 208, 184, 208, 186, 208, 176, 0, 0
    };
    for (size_t i = 0; i < sizeof(qs)/sizeof(*qs); i++) {
        add_number(qs[i], ptr, pos);
    }

    return pos;
}

size_t write_data(int fd)
{
    size_t pos = 0;
    uint8_t  ku8  = 1;    pos += write(fd, &ku8, sizeof(ku8));
    uint16_t ku16 = 2;    pos += write(fd, &ku16, sizeof(ku16));
    uint32_t ku32 = 3;    pos += write(fd, &ku32, sizeof(ku32));
    int16_t  ki16 = -5;   pos += write(fd, &ki16, sizeof(ki16));
    int32_t  ki32 = -6;   pos += write(fd, &ki32, sizeof(ki32));
    int64_t  ki64 = -7;   pos += write(fd, &ki64, sizeof(ki64));
    float    kf32 = -8.1; pos += write(fd, &kf32, sizeof(kf32));
    double   kf64 = 9.2;  pos += write(fd, &kf64, sizeof(kf64));

    const char* strs[] = { "one", "two", "bbc", "fox" };
    pos += write(fd, strs[0], strlen(strs[0]));
    pos += write(fd, strs[1], strlen(strs[1]) + 1);
    pos += write(fd, strs[2], strlen(strs[2]) + 1);
    pos += write(fd, strs[3], strlen(strs[3]));

    for (int i = 0; i < 8; i++) {
        uint32_t ku32 = 1000 + i;
        pos += write(fd, &ku32, sizeof(ku32));
    }

    const char* xs[] = { "x1", "x2", "x3" };
    for (size_t i = 0; i < sizeof(xs)/sizeof(*xs); i++) {
        int len = strlen(xs[i]);
        int n = write(fd, xs[i], len);
        (void)n;
        pos += len;
    }

    const char* ys[] = { "y1", "y2", "y3" };
    for (size_t i = 0; i < sizeof(ys)/sizeof(*ys); i++) {
        int len = strlen(ys[i]) + 1;
        int n = write(fd, ys[i], len);
        (void)n;
        pos += len;
    }

    testOffsets.fixedWstrArray = pos;
    uint8_t zs[] = {
        208, 160, 208, 176, 208, 183,
        208, 148, 208, 178, 208, 176,
        208, 147, 208, 190, 208, 191,
        208, 163, 208, 191, 209, 129
    };
    for (size_t i = 0; i < sizeof(zs)/sizeof(*zs); i++) {
        pos += write(fd, &zs[i], sizeof(zs[i]));
    }

    testOffsets.zeroTermWstrArray = pos;
    uint8_t qs[] = {
        208, 148, 208, 182, 208, 176, 208, 183, 0, 0,
        208, 160, 208, 190, 208, 186, 0, 0,
        208, 154, 208, 187, 208, 176, 209, 129, 209, 129, 208, 184, 208, 186, 208, 176, 0, 0
    };
    for (size_t i = 0; i < sizeof(qs)/sizeof(*qs); i++) {
        pos += write(fd, &qs[i], sizeof(qs[i]));
    }

    return pos;
}

// This function creates a memory leak. It is acceptable here for test purposes.

static std::unique_ptr<MallocSourceTest> malloc_source_test;
static std::unique_ptr<FileSourceTest> file_source_test;
static std::unique_ptr<ShmSourceTest> shmem_source_test;
static std::unique_ptr<IpcSourceTest> sysvshmem_source_test;

SourcePtr make_test_source(const char* cfg)
{
    const size_t Size = 1024;
    const char* FilePath = "/tmp/brie.test";
    const char* ShMemPath = "/brie.test";
    SourcePtr source;

    if (strcmp(cfg, "malloc") == 0) {
        malloc_source_test = std::make_unique<MallocSourceTest>();
        MallocSourceTest* test = malloc_source_test.get();
        const std::string s = test->make(Size);
        size_t dataSize = populate_data(test->ptr());
        source = make_source(s.c_str());
        source->set_size(dataSize);
    } else if (strcmp(cfg, "file") == 0) {
        file_source_test = std::make_unique<FileSourceTest>();
        FileSourceTest* test = file_source_test.get();
        const std::string s = test->make(FilePath);
        write_data(test->fd());
        source = make_source(s.c_str());
    } else if (strcmp(cfg, "shmem") == 0) {
        shmem_source_test = std::make_unique<ShmSourceTest>();
        ShmSourceTest* test = shmem_source_test.get();
        const std::string s = test->make(ShMemPath, Size);
        size_t dataSize = populate_data(test->ptr());
        source = make_source(s.c_str());
        source->set_size(dataSize);
    } else if (strcmp(cfg, "sysvshmem") == 0) {
        sysvshmem_source_test = std::make_unique<IpcSourceTest>();
        IpcSourceTest* test = sysvshmem_source_test.get();
        const std::string s = test->make(FilePath, Size);
        size_t dataSize = populate_data(test->ptr());
        source = make_source(s.c_str());
        source->set_size(dataSize);
    }

    return source;
}

} // namespace brie
