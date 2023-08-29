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
 *   Unit tests to support full-cycle tests running brie as
 *   a standalone executable.
 */

#include <stdlib.h>
#include <stdio.h>
#include "utils/log.h"
#include "gtest/gtest.h"


// Dummy finish. Real finish is in main.cpp
int finish()
{
    return 0;
}

class TestFileCloser
{
public:
    TestFileCloser(FILE* f) : m_file(f) {}
    ~TestFileCloser() { if (m_file != nullptr) fclose(m_file); }
private:
    FILE* m_file;
};

static void prepare_file(const char* path, const char** lines, size_t count)
{
    FILE* f = fopen(path, "w");
    ASSERT_NE(nullptr, f);
    const TestFileCloser tfc(f);

    for (size_t i=0; i < count; i++) {
        fputs(lines[i], f);
        fputc('\n', f);
    }
}

static int compare_files(const char* leftPath, const char* rightPath)
{
    FILE* left = fopen(leftPath, "r");
    if (nullptr == left) return 1;
    const TestFileCloser tfcLeft(left);

    FILE* right = fopen(rightPath, "r");
    if (nullptr == right) return 1;
    const TestFileCloser tfcRight(right);

    while (true) {
        char leftBuf[256];
        char rightBuf[256];

        char* sLeft = fgets(leftBuf, sizeof(leftBuf), left);
        char* sRight = fgets(rightBuf, sizeof(rightBuf), right);

        bool leftFinished = (sLeft == nullptr);
        bool rightFinished = (sRight == nullptr);

        if (leftFinished && rightFinished) return 0;
        if (leftFinished && !rightFinished) return 1;
        if (!leftFinished && rightFinished) return 1;

        //printf("Left : %s\n", sLeft);
        //printf("Right: %s\n", sRight);
        if (0 != strcmp(sLeft, sRight)) return 1;
    }

    return 0;
}

static void run_script(const char* testIndex,
                       const char** script, size_t scriptSize,
                       const char** output, size_t outputSize,
                       const char** err, size_t errSize,
                       const char* inputList = "")
{
    const std::string scriptPathStr = std::string("/tmp/utest_console_script_") + testIndex;
    const std::string stdoutPathStr = std::string("/tmp/utest_console_stdout_") + testIndex;
    const std::string stderrPathStr = std::string("/tmp/utest_console_stderr_") + testIndex;
    const std::string expectedPathStr = std::string("/tmp/utest_console_expected_") + testIndex;
    const std::string expectedErrStr = std::string("/tmp/utest_console_err_") + testIndex;

    const char* scriptPath = scriptPathStr.c_str();
    const char* stdoutPath = stdoutPathStr.c_str();
    const char* stderrPath = stderrPathStr.c_str();
    const char* expectedPath = expectedPathStr.c_str();
    const char* expectedErr = expectedErrStr.c_str();

    prepare_file(scriptPath, script, scriptSize);
    prepare_file(expectedPath, output, outputSize);
    prepare_file(expectedErr, err, errSize);

    char commandLine[256];
    snprintf(commandLine, sizeof(commandLine), "./brie %s %s >%s 2>%s", 
        scriptPath, inputList, stdoutPath, stderrPath);

    int n = system(commandLine);
    (void)n;

    int ret = compare_files(stdoutPath, expectedPath);
    ASSERT_EQ(0, ret);

    ret = compare_files(stderrPath, expectedErr);
    ASSERT_EQ(0, ret);

    unlink(scriptPath);
    unlink(stdoutPath);
    unlink(stderrPath);
    unlink(expectedPath);
}


TEST(CONSOLE, Basic)
{
    const char* script[] = { "print(\"Hello, world!\")" };
    const char* expectedOutput[] = { "Hello, world!" };
    const char* expectedErrOutput[] = {};

    run_script("1",
               script, sizeof(script)/sizeof(*script),
               expectedOutput, sizeof(expectedOutput)/sizeof(*expectedOutput),
               expectedErrOutput, sizeof(expectedErrOutput)/sizeof(*expectedErrOutput));
}

TEST(CONSOLE, BasicPrefix)
{
    const char* script[] = {
        "print('prefix')",
        "%%",
        "print('body1')",
        "print('body2')",
    };
    const char* expectedOutput[] = {
        "prefix",
        "body1",
        "body2",
    };
    const char* expectedErrOutput[] = {};

    run_script("1",
               script, sizeof(script)/sizeof(*script),
               expectedOutput, sizeof(expectedOutput)/sizeof(*expectedOutput),
               expectedErrOutput, sizeof(expectedErrOutput)/sizeof(*expectedErrOutput));
}

TEST(CONSOLE, BasicPrefixPostfix)
{
    const char* script[] = {
        "print('prefix')",
        "%%",
        "print('body1')",
        "print('body2')",
        "%%",
        "print('postfix')",
    };
    const char* expectedOutput[] = {
        "prefix",
        "body1",
        "body2",
        "postfix",
    };
    const char* expectedErrOutput[] = {};

    run_script("1",
               script, sizeof(script)/sizeof(*script),
               expectedOutput, sizeof(expectedOutput)/sizeof(*expectedOutput),
               expectedErrOutput, sizeof(expectedErrOutput)/sizeof(*expectedErrOutput));
}

TEST(CONSOLE, MultyPrefix)
{
    const char* script[] = {
        "print('prefix')",
        "%%",
        "print('body1')",
        "print('body2')",
    };
    const char* expectedOutput[] = {
        "prefix",
        "body1",
        "body2",
        "body1",
        "body2",
    };
    const char* expectedErrOutput[] = {};

    const char* inputList = "test:malloc test:malloc";

    run_script("1",
               script, sizeof(script)/sizeof(*script),
               expectedOutput, sizeof(expectedOutput)/sizeof(*expectedOutput),
               expectedErrOutput, sizeof(expectedErrOutput)/sizeof(*expectedErrOutput),
               inputList);
}

TEST(CONSOLE, MultyPrefixPostfix)
{
    const char* script[] = {
        "print('prefix')",
        "%%",
        "print('body1')",
        "print('body2')",
        "%%",
        "print('postfix')"
    };
    const char* expectedOutput[] = {
        "prefix",
        "body1",
        "body2",
        "body1",
        "body2",
        "postfix"
    };
    const char* expectedErrOutput[] = {};

    const char* inputList = "test:malloc test:malloc";

    run_script("1",
               script, sizeof(script)/sizeof(*script),
               expectedOutput, sizeof(expectedOutput)/sizeof(*expectedOutput),
               expectedErrOutput, sizeof(expectedErrOutput)/sizeof(*expectedErrOutput),
               inputList);
}

void run_test_mode(const char* str, const char* testIndex)
{
    const char* script[] = {
        "",
        "u8,u16,u32 = read('u8 u16 u32'); println('%d %d %d', u8, u16, u32)",
        "i16, i32, i64 = read('i16 i32 i64'); println('%d %d %d', i16, i32, i64)",
        "f32, f64 = read('f32 f64'); println('%.1f %.1f', f32, f64);",
        "s1, s2, s3, s4 = read('str#3 str str str#3'); println('%s %s %s %s', s1, s2, s3, s4);",
        "a = read('u32*8'); println('%d %d %d %d %d %d %d %d', a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8])",
        "b = read('str#2*3'); println('%s %s %s', b[1], b[2], b[3])",
        "p1 = BRIE_POS",
        "c = read('str*3'); println('%s %s %s', c[1], c[2], c[3])",
        "d = read('wstr#3*4'); println('%s %s %s %s', d[1], d[2], d[3], d[4])",
        "p2 = BRIE_POS",
        "e = read('wstr*3'); println('%s %s %s', e[1], e[2], e[3])",
        "p3 = BRIE_POS",
        "setpos(p1)",
        // Read zero terminated strings with a fixed length format
        "k = read('str#3*3'); println('%s %s %s', k[1], k[2], k[3])",
        "setpos(p2)",
        // Read a zero terminated wide string with a fixed length format
        "m = read('wstr#5'); println('%s', m)",
        "setpos(p3)",
        "q = read('u8')"
    };
    script[0] = str;

    const char* expectedOutput[] = {
        "1 2 3",
        "-5 -6 -7",
        "-8.1 9.2",
        "one two bbc fox",
        "1000 1001 1002 1003 1004 1005 1006 1007",
        "x1 x2 x3",
        "y1 y2 y3",
        "Раз Два Гоп Упс",
        "Джаз Рок Классика",
        "y1 y2 y3",
        "Джаз",
    };

    const char* expectedErrOutput[] = {
        "[string \"q = read('u8')...\"]:1: Insufficent data in source",
        "Line #19"
    };

    run_script(testIndex,
               script, sizeof(script)/sizeof(*script),
               expectedOutput, sizeof(expectedOutput)/sizeof(*expectedOutput),
               expectedErrOutput, sizeof(expectedErrOutput)/sizeof(*expectedErrOutput));
}

TEST(CONSOLE, File)
{
    run_test_mode("open('test:file')", "2");
}

TEST(CONSOLE, Malloc)
{
    run_test_mode("open('test:malloc')", "3");
}

TEST(CONSOLE, ShMem)
{
    run_test_mode("open('test:shmem')", "4");
}

TEST(CONSOLE, SysVShMem)
{
    run_test_mode("open('test:sysvshmem')", "5");
}

TEST(CONSOLE, Struct)
{
    const char* script[] = {
        "open('test:malloc')",
        "decl('one', 'u8:aaa u16:bbb')",
        "decl('two', 'u32:ccc i16:ddd')",
        "decl('three', 'one:xxx two:yyy')",
        "t = read('three')",
        "x = t.xxx",
        "y = t.yyy",
        "println('%d %d %d %d', x.aaa, x.bbb, y.ccc, y.ddd)",
    };

    const char* expectedOutput[] = {
        "1 2 3 -5",
    };

    const char* expectedErrOutput[] = {
    };

    run_script("6",
               script, sizeof(script)/sizeof(*script),
               expectedOutput, sizeof(expectedOutput)/sizeof(*expectedOutput),
               expectedErrOutput, sizeof(expectedErrOutput)/sizeof(*expectedErrOutput));
}

TEST(CONSOLE, Struct2)
{
    const char* script[] = {
        "open('test:malloc')",
        "decl('one', 'u32*8:aaa str#2*3:bbb str*3:ccc')",
        "setpos('test:u32array')",
        "t = read('one')",
        "a = t.aaa",
        "b = t.bbb",
        "c = t.ccc",
        "println('%d %d %d %d %d %d %d %d', a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8])",
        "println('%s %s %s', b[1], b[2], b[3])",
        "println('%s %s %s', c[1], c[2], c[3])",
    };

    const char* expectedOutput[] = {
        "1000 1001 1002 1003 1004 1005 1006 1007",
        "x1 x2 x3",
        "y1 y2 y3",
    };

    const char* expectedErrOutput[] = {
    };

    run_script("7",
               script, sizeof(script)/sizeof(*script),
               expectedOutput, sizeof(expectedOutput)/sizeof(*expectedOutput),
               expectedErrOutput, sizeof(expectedErrOutput)/sizeof(*expectedErrOutput));
}

TEST(CONSOLE, Struct3)
{
    const char* script[] = {
        "open('test:malloc')",
        "decl('one', 'wstr#3*4:bbb wstr*3:ccc')",
        "setpos('test:fixedWstrArray')",
        "t = read('one')",
        "b = t.bbb",
        "c = t.ccc",
        "println('%s %s %s %s', b[1], b[2], b[3], b[4])",
        "println('%s %s %s', c[1], c[2], c[3])",
    };

    const char* expectedOutput[] = {
        "Раз Два Гоп Упс",
        "Джаз Рок Классика",
    };

    const char* expectedErrOutput[] = {
    };

    run_script("8",
               script, sizeof(script)/sizeof(*script),
               expectedOutput, sizeof(expectedOutput)/sizeof(*expectedOutput),
               expectedErrOutput, sizeof(expectedErrOutput)/sizeof(*expectedErrOutput));
}

TEST(CONSOLE, Void1)
{
    const char* script[] = {
        "open('test:malloc')",
        "a, b = read('void#1 void#2 u32 i16')",
        "println('%d %d', a, b)",
    };

    const char* expectedOutput[] = {
        "3 -5",
    };

    const char* expectedErrOutput[] = {
    };

    run_script("9", 
               script, sizeof(script)/sizeof(*script),
               expectedOutput, sizeof(expectedOutput)/sizeof(*expectedOutput),
               expectedErrOutput, sizeof(expectedErrOutput)/sizeof(*expectedErrOutput));
}

TEST(CONSOLE, Void2)
{
    const char* script[] = {
        "open('test:malloc')",
        "decl('one', 'u8:aaa void#2*3 i16:bbb')"
        "t = read('one')",
        "println('%d %d', t.aaa, t.bbb)",
    };

    const char* expectedOutput[] = {
        "1 -5",
    };

    const char* expectedErrOutput[] = {
    };

    run_script("9", 
               script, sizeof(script)/sizeof(*script),
               expectedOutput, sizeof(expectedOutput)/sizeof(*expectedOutput),
               expectedErrOutput, sizeof(expectedErrOutput)/sizeof(*expectedErrOutput));
}

TEST(CONSOLE, Struct4)
{
    const char* script[] = {
        "open('test:malloc')",
        "decl('one', 'u32*4:aaa')",
        "setpos('test:u32array')",
        "t = read('one*2')",
        "a = t[1].aaa",
        "b = t[2].aaa",
        "println('%d %d %d %d %d %d %d %d', a[1], a[2], a[3], a[4], b[1], b[2], b[3], b[4])",
    };

    const char* expectedOutput[] = {
        "1000 1001 1002 1003 1004 1005 1006 1007",
    };

    const char* expectedErrOutput[] = {
    };

    run_script("A",
               script, sizeof(script)/sizeof(*script),
               expectedOutput, sizeof(expectedOutput)/sizeof(*expectedOutput),
               expectedErrOutput, sizeof(expectedErrOutput)/sizeof(*expectedErrOutput));
}

TEST(CONSOLE, Struct5)
{
    const char* script[] = {
        "open('test:malloc')",
        "function f2() return 42; end",
        "function f3() return 37; end",
        "decl('one', '@f2:aaa @f2:bbb @f3*4:ccc')",
        "q = read('one')",
        "v = q.ccc",
        "println('%d %d %d %d %d %d', q.aaa, q.bbb, v[1], v[2], v[3], v[4])",
    };

    const char* expectedOutput[] = {
        "42 42 37 37 37 37",
    };

    const char* expectedErrOutput[] = {
    };

    run_script("B",
               script, sizeof(script)/sizeof(*script),
               expectedOutput, sizeof(expectedOutput)/sizeof(*expectedOutput),
               expectedErrOutput, sizeof(expectedErrOutput)/sizeof(*expectedErrOutput));
}
