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
#include <string>

namespace brie {

// Custom exception that can be thrown by the BRIE code
struct Error
{
public:
    const std::string& what() const { return m_msg; }
    void set(const std::string& msg) { m_msg = msg; }

private:
    std::string m_msg;
};

// Utility function that throws Error exception
void throwex(const std::string& msg);

} // namespace brie
