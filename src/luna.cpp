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

#include "luna.h"
#include "source.h"
#include "source_test.h"
#include "parser.h"
#include "error.h"
#include "structs.h"
#include "utils/log.h"
#include <string.h>
#include <string>
#include <unordered_map>

namespace brie {

static SourcePtr source;
static bool isError = false;

// Names of global variables
static const char* BRIE_POS = "BRIE_POS";
static const char* BRIE_PATH = "BRIE_PATH";
static const char* BRIE_SIZE = "BRIE_SIZE";

/**************************************************************************
 */ 
void set_brie_pos_var(lua_State* L)
{
    lua_pushinteger(L, source->pos());
    lua_setglobal(L, BRIE_POS);
}

void set_brie_path_var(lua_State* L)
{
    lua_pushstring(L, source->name().c_str());
    lua_setglobal(L, BRIE_PATH);
}

void set_brie_size_var(lua_State* L)
{
    lua_pushinteger(L, source->size());
    lua_setglobal(L, BRIE_SIZE);
}

/**************************************************************************
 */ 
int set_source(const char* cfg, lua_State *L)
{
    source = make_source(cfg);
    if (source) {
        set_brie_pos_var(L);
        set_brie_path_var(L);
        set_brie_size_var(L);
    }

    return source.get() != nullptr ? 0 : -1;
}

/**************************************************************************
 */ 
static void retrieve(lua_State *L, const DataItem& item);

static void retrieve_struct(lua_State *L, const std::string& name)
{
    const StructPtr sp = get_struct(name);
    const FieldList& fields = *sp;

    lua_createtable(L, 0, fields.size());

    for (const Field& f: fields) {
        retrieve(L, f);
        if (f.type != VOID) lua_setfield(L, -2, f.name.c_str());
    }
}

static void retrieve(lua_State *L, const DataItem& item)
{
    if (item.type == VOID) {
        size_t pos = source->pos() + item.size * item.count;
        source->set_pos(pos);
        return;
    }

    if (item.count > 1) lua_createtable(L, item.count, 0);

    for (size_t i = 1; i <= item.count; i++) {
        switch (item.type) {
            case U8:
            case U16:
            case U32:
            case I16:
            case I32:
            case I64: {
                int64_t val = source->read_int(item.type);
                lua_pushinteger(L, val);
                break;
            }
            case F32:
            case F64: {
                double val = source->read_float(item.type);
                lua_pushnumber(L, val);
                break;
            }

            case STRING: {
                std::string val = source->read_str(item.size);
                lua_pushlstring(L, val.c_str(), val.length());
                break;
            }

            case WSTRING: {
                std::string val = source->read_wstr(item.size);
                lua_pushlstring(L, val.c_str(), val.length());
                break;
            }

            case VOID:
                throwex("Unexpected retrieve");
                break;

            case FUNC: {
                    int type = lua_getglobal(L, item.typeName.c_str());
                    if (type != LUA_TFUNCTION) {
                        lua_pop(L, 1);
                        throwex("Function not defined");
                    }
                    int stackSizeBefore = lua_gettop(L);
                    lua_call(L, 0, LUA_MULTRET);
                    int stackSizeAfter = lua_gettop(L);
                    if (stackSizeBefore != stackSizeAfter) {
                        throwex("Function returned too many results");
                    }
                }
                break;

            default:
                retrieve_struct(L, item.typeName);
                break;
        }

        if (item.count > 1) lua_rawseti(L, -2, i);
    }
}

static int func_read_wrapped(lua_State *L)
{
    LOG_DEBUG << "Execute function read";

    if (!source) throwex("Source is not set");

    const char* descr = luaL_checkstring(L, 1);
    if (descr == nullptr) {
        luaL_error(L, "%s", "Missing descriptor");
        return 0;
    }

    LOG_DEBUG << "Function read input [" << descr << "]";
    DataItemList items;
    parse_read_str(descr, items);

    size_t count = 0;
    for (const auto& item: items) {
        if (item.type != VOID) count++;
        retrieve(L, item);
    }

    set_brie_pos_var(L);

    return count;
}

static int func_read (lua_State *L) 
{
    try {
        return func_read_wrapped(L);
    } catch (const Error& err) {
        luaL_error(L, "%s", err.what().c_str());
    }

    return 0;
}

/**************************************************************************
 */
static int func_open_wrapped(lua_State* L)
{
    const char* cfg = luaL_checkstring(L, 1);
    if (cfg == nullptr) {
        luaL_error(L, "%s", "Missing config");
        return 0;
    }

    source = make_source(cfg);
    set_brie_pos_var(L);
    set_brie_path_var(L);

    return 0;
}

static int func_open(lua_State* L)
{
    try {
        return func_open_wrapped(L);
    } catch (const Error& err) {
        luaL_error(L, "%s", err.what().c_str());
    }

    return 0;
}

/**************************************************************************
 */
static int func_declare_wrapped(lua_State* L)
{
    int count = lua_gettop(L);
    if (count != 1 && count != 2) {
        luaL_error(L, "Invalid parameters count");
        return 0;
    }

    const char* name = luaL_checkstring(L, 1);
    if (name == nullptr || *name == '\0') {
        luaL_error(L, "%s", "Missing name");
        return 0;
    }

    if (count == 1) {
        show_decl(name);
        return 0;
    }

    const char* decl = luaL_checkstring(L, 2);
    if (decl == nullptr || *decl == '\0') {
        luaL_error(L, "%s", "Invalid declaration");
        return 0;
    }

    StructPtr sp = std::make_shared<FieldList>();
    parse_fields_str(decl, *sp);

    add_struct(name, sp);

    return 0;
}

static int func_declare(lua_State* L)
{
    try {
        return func_declare_wrapped(L);
    } catch (const Error& err) {
        luaL_error(L, "%s", err.what().c_str());
    }

    return 0;
}

/**************************************************************************
 */
static int func_set_position_wrapped(lua_State* L)
{
    if (!source) throwex("Source is not set");

    const int count = lua_gettop(L);
    if (count != 1) {
        luaL_error(L, "Invalid parameters count");
        return 0;
    }

    int paramType = lua_type(L, 1);
    if (paramType == LUA_TSTRING) {
        const char* param = luaL_checkstring(L, 1);
        source->set_test_pos(param);
        return 0;
    }

    int64_t pos = luaL_checkinteger(L, 1);
    if (pos < 0) {
        luaL_error(L, "Invalid position value");
        return 0;
    }

    source->set_pos(pos);

    set_brie_pos_var(L);

    return 0;
}

static int func_set_position(lua_State* L)
{
    try {
        return func_set_position_wrapped(L);
    } catch (const Error& err) {
        luaL_error(L, "%s", err.what().c_str());
    }

    return 0;
}

/**************************************************************************
 */
static int func_find_wrapped(lua_State* L)
{
    if (!source) throwex("Source is not set");

    const char* str = luaL_checkstring(L, 1);
    if (str == nullptr || *str == '\0') {
        luaL_error(L, "Invalid first parameter");
        return 0;
    }

    int offset = luaL_checkinteger(L, 2);
    if (offset == 0) {
        luaL_error(L, "Invalid second parameter");
        return 0;
    }

    size_t needle = source->find(str, offset);
    if (needle == Source::nopos()) {
        lua_pushinteger(L, -1);
    } else {
        lua_pushinteger(L, needle);
    }

    return 1;
}

static int func_find(lua_State* L)
{
    try {
        return func_find_wrapped(L);
    } catch (const Error& err) {
        luaL_error(L, "%s", err.what().c_str());
    }

    return 0;
}

/**************************************************************************
 */ 
static int func_error(lua_State* L)
{
    const char* msg = luaL_checkstring(L, 1);
    if (msg != nullptr) fprintf(stderr, "%s\n", msg);
    isError = true;
    return 0;
}

/**************************************************************************
 */ 
static int func_finish(lua_State*)
{
    finish();
    return 0;
}

/**************************************************************************
 */ 
/* mark in error messages for incomplete statements */
#define EOFMARK		"<eof>"
#define marklen		(sizeof(EOFMARK)/sizeof(char) - 1)

static int incomplete (lua_State *L, int status) {
  if (status == LUA_ERRSYNTAX) {
    size_t lmsg;
    const char *msg = lua_tolstring(L, -1, &lmsg);
    if (lmsg >= marklen && strcmp(msg + lmsg - marklen, EOFMARK) == 0) {
      lua_pop(L, 1);
      return 1;
    }
  }
  return 0;  /* else... */
}

/**************************************************************************
 */ 
Luna::Luna()
{
    m_state = luaL_newstate();
    luaL_openlibs(m_state);
}

Luna::~Luna()
{
    lua_close(m_state);
}

void Luna::init()
{
    LOG_DEBUG << "Register functions";

    init_structs();

    lua_pushcfunction(m_state, func_read);
    lua_setglobal(m_state, "read");

    lua_pushcfunction(m_state, func_open);
    lua_setglobal(m_state, "open");

    lua_pushcfunction(m_state, func_error);
    lua_setglobal(m_state, "seterr");

    lua_pushcfunction(m_state, func_declare);
    lua_setglobal(m_state, "decl");

    lua_pushcfunction(m_state, func_set_position);
    lua_setglobal(m_state, "setpos");

    lua_pushcfunction(m_state, func_find);
    lua_setglobal(m_state, "find");

    lua_pushcfunction(m_state, func_finish);
    lua_setglobal(m_state, "finish");

    const char* printf_str = "printf = function(s,...); return io.write(s:format(...)); end";
    luaL_loadstring(m_state, printf_str);
    lua_pcall(m_state, 0, LUA_MULTRET, 0);

    const char* println_str = "println = function(s,...); return print(s:format(...)); end";
    luaL_loadstring(m_state, println_str);
    lua_pcall(m_state, 0, LUA_MULTRET, 0);

    const char* scanf_str = "scanf = function(s,input); return println(s:format(read(input))); end;";
    luaL_loadstring(m_state, scanf_str);
    lua_pcall(m_state, 0, LUA_MULTRET, 0);

}

int Luna::exec(const char* line)
{
    isError = false;

    m_code += line;
    int ret = luaL_loadstring(m_state, m_code.c_str());
    if (incomplete(m_state, ret)) return 1;

    m_code = "";

    if (ret != 0) return print_error();

    ret = lua_pcall(m_state, 0, LUA_MULTRET, 0);
    if (ret != 0) return print_error();
    
    return 0;
}

int Luna::print_error()
{
    isError = true;
    size_t lmsg;
    const char *msg = lua_tolstring(m_state, -1, &lmsg);
    fprintf(stderr, "%s\n", msg);
    lua_pop(m_state, 1);
    return 0;
}

bool Luna::is_error() const
{
    return isError;
}


} // namespace brie
