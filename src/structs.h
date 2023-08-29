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
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace brie {

/*****************************************************************
 *   Support for declaring and using structs.
 */

/*****************************************************************
 *   Descriptor for a field in a struct or an item to read.
 *   Used in parser.h/.cpp
 */
struct DataItem
{
    Type type;
    size_t size;
    size_t count;
    std::string typeName;
};
using DataItemList = std::vector<DataItem>;

struct Field : public DataItem
{
    std::string name;
};
using FieldList = std::vector<Field>;

using StructPtr = std::shared_ptr<FieldList>;
using StructDict = std::unordered_map<std::string, StructPtr>;
using StructDictPtr = std::unique_ptr<StructDict>;

void init_structs();
void add_struct(const char* name, StructPtr sp);
const StructPtr get_struct(const std::string& name);
void check_in_width(const std::string& name, const FieldList& fields);
void check_in_depth(const std::string& name, const FieldList& fields);
void print_field(const Field& f);
const char* get_type_str(Type type); // Use result quickly!!!
void print_field(const Field& f);
void show_decl(const char* name);
size_t get_sizeof(const char* name);

} // namespace brie
