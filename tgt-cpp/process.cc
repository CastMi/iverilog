/*
 *  VHDL code generation for processes.
 *
 *  Copyright (C) 2008-2013  Nick Gasson (nick@nickg.me.uk)
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

#include "cpp_target.h"
#include "cpp_element.hh"
#include "state.hh"
#include "cpp_syntax.hh"

#include <iostream>
#include <cassert>
#include <sstream>

extern "C" int draw_process(ivl_process_t proc, void *)
{
   ivl_scope_t scope = ivl_process_scope(proc);

   if (!is_default_scope_instance(scope))
      return 0;  // Ignore this process at it's not in a scope that
                 // we're using to generate code

   debug_msg("Translating process in scope type %s (%s:%d)",
             ivl_scope_tname(scope), ivl_process_file(proc),
             ivl_process_lineno(proc));

   // Skip over any generate and begin scopes until we find
   // the module that contains them - this is where we will
   // generate the process
   while (ivl_scope_type(scope) == IVL_SCT_GENERATE
      || ivl_scope_type(scope) == IVL_SCT_BEGIN)
      scope = ivl_scope_parent(scope);

   assert(ivl_scope_type(scope) == IVL_SCT_MODULE);
   cppClass *ent = find_class(scope);
   assert(ent != NULL);

   return 0;
}
