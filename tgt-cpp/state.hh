/*
 *  Managing global state for the VHDL code generator.
 *
 *  Copyright (c) 2015 Michele Castellana (michele.castellana@mail.polimi.it)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef INC_CPP_STATE_HH
#define INC_CPP_STATE_HH

#include "ivl_target.h"
#include "cpp_syntax.hh"
#include <iosfwd>
#include <string>

class cpp_scope;
class cppClass;

bool seen_signal_before(ivl_signal_t sig);
void remember_signal(ivl_signal_t sig, cpp_scope *scope);
void rename_signal(ivl_signal_t sig, const std::string &renamed);
cpp_scope *find_scope_for_signal(ivl_signal_t sig);
const std::string &get_renamed_signal(ivl_signal_t sig);
ivl_signal_t find_signal_named(const std::string &name, const cpp_scope *scope);

void only_remember_class(cppClass* theclass);
void remember_class(cppClass *ent, ivl_scope_t scope);
cppClass* find_class(ivl_scope_t scope);
cppClass* find_class(const std::string& name);

void emit_everything(std::ostream& os);
void free_all_cpp_objects();

cpp_context* get_context();
cppClass *get_active_class();
void set_active_class(cppClass *ent);

bool is_default_scope_instance(ivl_scope_t s);
bool seen_this_scope_type(ivl_scope_t s);

#endif  // #ifndef INC_VHDL_STATE_HH
