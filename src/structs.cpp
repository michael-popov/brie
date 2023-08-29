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

#include "structs.h"
#include "error.h"
#include "utils/log.h"

namespace brie {

static StructDictPtr structs;

void init_structs()
{
    structs = std::make_unique<StructDict>();
}

const StructPtr get_struct(const std::string& name)
{
    const auto iter = structs->find(name);
    if (iter == structs->end()) throwex("Struct is not found");
    return iter->second;
}

void add_struct(const char* name, StructPtr sp)
{
    check_in_width(name, *sp);
    check_in_depth(name, *sp);

    StructDict& dict = *structs;
    dict[name] = sp;
}

void check_in_width(const std::string& name, const FieldList& fields)
{
    for (const Field& f: fields) {
        if (f.typeName.empty() || f.type == FUNC) continue;

        if (f.typeName == name) throwex("Self reference");

        const auto iter = structs->find(f.typeName);
        if (iter == structs->end()) throwex("Missing struct definition");
    }
}

void check_in_depth(const std::string& name, const FieldList& fields)
{
    for (const Field& f: fields) {
        if (f.typeName.empty() || f.type == FUNC) continue;
        if (name == f.typeName) throwex("Circular struct definition");

        const auto iter = structs->find(f.typeName);
        if (iter == structs->end()) throwex("Missing struct definition");

        check_in_depth(name, *iter->second);
    }
}

const char* get_type_str(Type type)
{
    switch (type) {
        case U8: return "u8";
        case U16: return "u16";
        case U32: return "u32";
        case I16: return "i16";
        case I32: return "i32";
        case I64: return "i64";
        case F32: return "f32";
        case F64: return "f64";
        case STRING: return "str";
        case WSTRING: return "wstr";
        case VOID: return "void";
        default: return "INVALID";
    }
}

void print_field(const Field& f)
{
    const char* typeStr = f.typeName.empty() ? get_type_str(f.type) : f.typeName.c_str();
    printf("%s", typeStr);
    if (f.size != 0) printf("#%lu", f.size);
    if (f.count != 1) printf("#%lu", f.count);
    printf(":%s", f.name.c_str());
    printf("\n");
}

void show_decl(const char* name)
{
    const auto iter = structs->find(name);
    if (iter == structs->end()) {
        printf("Struct '%s' not found\n", name);
        return;
    }

    const auto ptr = iter->second;
    const FieldList& fields = *ptr;

    int i = 1;
    for (const Field& f: fields) {
        printf("%d) ", i++);
        print_field(f);
    }

    printf("\n");
}


} // namespace brie
