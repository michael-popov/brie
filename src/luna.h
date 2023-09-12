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
#include "lua.hpp"
#include <string>

namespace brie {

/*****************************************************************
 *  Wrapper class for Lua engine
 */
class Luna
{
public:
    Luna();
    ~Luna();

    Luna(const Luna&);
    Luna& operator=(const Luna&);
    
    lua_State* operator()() { return m_state; }

    void init();
    int exec(const char* line);
    bool is_error() const;
    const std::string get_error();

private:
    lua_State* m_state;
    std::string m_code;

private:
    int print_error();
};

/*****************************************************************
 *  Global function that sets a current source for binary reading
 */
int set_source(const char* cfg, lua_State *L);

} // namespace brie


/*****************************************************************
 *  Utility function that allows executing a postfix part of a 
 *  brie script.
 *  Function is defined in main.cpp.
 */
int finish();
