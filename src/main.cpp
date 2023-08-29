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

/*****************************************
 *   main.cpp
 */
#include "luna.h"
#include "error.h"
#include <readline/readline.h>
#include <readline/history.h>
#include "utils/log.h"
#include <unistd.h>
#include <string>
#include <vector>

using Lines = std::vector<std::string>;
static Lines prefix;
static Lines body;
static Lines postfix;

static char* line = nullptr;
static char defaultPrompt[] = "> ";
static char inProcessPrompt[] = ">> ";
static bool scriptToRepl = false;
static size_t lineNum = 1;

struct LineCleaner { ~LineCleaner() { if (line != nullptr) { free(line); line = nullptr; }}};
static char* my_readline(char* prompt = defaultPrompt);
static int repl();
static int process_script(brie::Luna& luna, const char* path, bool first, bool last);
static int process_multiple_files(int argc, char* argv[]);
static int read_script(const char* path);
static int process_lines(brie::Luna& luna, const Lines& lines, size_t lineNum);

static brie::Luna luna;

static constexpr size_t MAX_PATH = 512;

int main(int argc, char* argv[])
{
    luna.init();
    if (argc >= 2) return process_multiple_files(argc-1, argv+1);
    return repl();
}


int repl()
{
    char* prompt = defaultPrompt;

    while (true) {
        LineCleaner cleaner;

        char* str = my_readline(prompt);
        if (str == nullptr) break;
        if (*str == '\0') continue;
        if (0 == strcmp(str, "quit")) break;
        if (0 == strcmp(str, "exit")) break;

        int ret = luna.exec(str);
        prompt = ret ? inProcessPrompt : defaultPrompt;
        luna.exec("\n");
    }

    return 0;
}

static char* my_readline(char* prompt)
{
    char* line = readline(prompt);
    if (line == nullptr) return nullptr;
    
    char* t = line;
    for (; *t != '\0'; t++) {
        if (*t != ' ' && *t != '\t') break;
    }

    if (*t == '\0') return t;

    int len = strlen(t);
    for (char* s = t + len - 1; s != t; s--) {
        if (*s != ' ' && *s != '\t') break;
        *s = '\0';
    }

    add_history(t);
    return t;
}

int read_script(const char* path)
{
    FILE* f = fopen(path, "r");
    if (f == nullptr) {
        // If it is not a file, then it's a command.
        prefix.push_back(path);        
        return 0;
    }

    enum class InputState { Body, Postfix };
    InputState inputState = InputState::Body;

    bool isPrefixFound = false;
    char buf[256];
    char* line;

    while (0 != (line = fgets(buf, sizeof(buf), f))) {

        int len = strlen(line);

        // Handle shebang
        if (len > 2 && line[0] == '#' && line[1] == '!') {
            continue;
        }

        // Handle line with %%
        if (len >= 2 && line[0] == '%' && line[1] == '%') {
            if (!isPrefixFound) {
                isPrefixFound = true;
                prefix = body;
                body.clear();
            } else {
                inputState = InputState::Postfix;
            }
            continue;
        }

        switch (inputState) {
            case InputState::Body: body.push_back(line); break;
            case InputState::Postfix: postfix.push_back(line); break;
        }
    }

    fclose(f);

    return 0;
}

int process_lines(brie::Luna& luna, const Lines& lines, size_t lineNum)
{
    for (const auto& str: lines) {
        luna.exec(str.c_str());

        if (luna.is_error()) {
            fprintf(stderr, "Line #%lu\n", lineNum);
            return 1;
        }

        lineNum++;
    }

    return 0;
}

int finish()
{
    lineNum = 3 + prefix.size() + body.size(); // 1 to start with 1 and +2 to include %% twice
    int ret = process_lines(luna, postfix, lineNum);
    return ret;
}

int process_script(brie::Luna& luna, const char* path, bool first, bool last)
{
    int ret = 0;
    if (first) {
        ret = read_script(path);
        if (ret != 0) return ret;

        ret = process_lines(luna, prefix, 1);        
        if (ret != 0) return ret;
    }

    if (scriptToRepl) return 0; // execute only prefix

    if (!prefix.empty()) lineNum = 2 + prefix.size(); // 1 to start with 1 and +1 to include %%

    ret = process_lines(luna, body, lineNum);
    if (ret != 0) return ret;

    if (last) {
        return finish();
    }

    return 0;
}

int process_source(const char* script, const char* source, bool first, bool last)
{
    try {
        int ret = brie::set_source(source, luna());
        if (ret != 0) {
            fprintf(stderr, "Failed to open source %s\n", source);
            return 1;
        }

        ret = process_script(luna, script, first, last);
        if (ret != 0) return 1;

    } catch (const brie::Error& err) {
        fprintf(stderr, "Failed: %s\n", err.what().c_str());
        return 1;
    }

    return 0;
}

int process_multiple_files(int argc, char* argv[])
{
    if (argc == 1) {
        return process_script(luna, argv[0], true, true);
    }

    if (argc == 2) {
        // Switch to REPL
        if (0 == strcmp(argv[1], "@")) {
            scriptToRepl = true;
            int ret = process_script(luna, argv[0], true, false);
            if (ret != 0) return 1;
            return repl();
        }

        // Read names of sources from stdin and execute
        // script for each source/
        if (0 == strcmp(argv[1], "-")) {
            //TempLogLevel tll(LL_DEBUG);

            char buf[MAX_PATH];
            char* s;
            bool first = true;

            while ((s = fgets(buf, sizeof(buf), stdin))) {
                int len = strlen(s);
                if (len > 0) s[len-1] = '\0';

                int ret = process_source(argv[0], s, first, false);
                if (ret != 0) return 1;
                first = false;
            }

            return finish();            
        }
    }

    // Get names of sources from the command-line arguments.
    for (int i=1; i < argc; i++) {
        bool first = (i == 1);
        bool last = (i == argc - 1);
        int ret = process_source(argv[0], argv[i], first, last);
        if (ret != 0) return 1;
    }

    return 0;
}
