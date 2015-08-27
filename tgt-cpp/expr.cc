/*
 *  Translate IR into expressions.
 *
 *  Copyright (C) 2015  Michele Castellana (michele.castellana@mail.polimi.it)
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
#include "state.hh"

#include <iostream>
#include <cassert>
#include <cstring>


/*
 * A numeric literal ends up as std_logic bit string.
 */
static cpp_expr *translate_number(ivl_expr_t e)
{
   assert(ivl_expr_width(e) == 1);
   return new cpp_const_expr("", ivl_expr_bits(e)[0]);
}

/*
 * Generate a C++ expression from a Verilog expression.
 */
cpp_expr *translate_expr(ivl_expr_t e)
{
   assert(e);
   ivl_expr_type_t type = ivl_expr_type(e);

   switch (type) {
   case IVL_EX_NUMBER:
      return translate_number(e);
   case IVL_EX_STRING:
   case IVL_EX_SIGNAL:
   case IVL_EX_ULONG:
   case IVL_EX_UNARY:
   case IVL_EX_BINARY:
   case IVL_EX_SELECT:
   case IVL_EX_UFUNC:
   case IVL_EX_TERNARY:
   case IVL_EX_CONCAT:
   case IVL_EX_SFUNC:
   case IVL_EX_DELAY:
   case IVL_EX_REALNUM:
   default:
      error("No translation for expression at %s:%d (type = %d)",
            ivl_expr_file(e), ivl_expr_lineno(e), type);
      return NULL;
   }
}
