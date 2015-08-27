/*
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
#include "hierarchy.hh"

#include <iostream>
#include <cstring>
#include <cassert>
#include <sstream>
#include <typeinfo>
#include <limits>
#include <set>
#include <algorithm>

static cpp_var_ref *make_assign_lhs(ivl_lval_t lval, cpp_scope *scope)
{
   ivl_signal_t sig = ivl_lval_sig(lval);
   if (!sig) {
      error("Only signals as lvals supported at the moment");
      return NULL;
   }

   string signame(get_renamed_signal(sig));
   cpp_decl *decl = scope->get_decl(signame);
   assert(decl);

   cpp_type *ltype = new cpp_type(*decl->get_type());
   return new cpp_var_ref(decl->get_name(), ltype);
}

static bool assignment_lvals(ivl_statement_t stmt, list<cpp_var_ref*> &lvals)
{
   int nlvals = ivl_stmt_lvals(stmt);
   assert(get_active_class != NULL);
   for (int i = 0; i < nlvals; i++) {
      ivl_lval_t lval = ivl_stmt_lval(stmt, i);
      cpp_var_ref *lhs = make_assign_lhs(lval, get_active_class()->get_scope());
      if (NULL == lhs)
         assert(false);

      lvals.push_back(lhs);
   }

   return true;
}

// Generate an assignment of type T for the Verilog statement stmt.
// If a statement was generated then `assign_type' will contain the
// type of assignment that was generated; this should be initialised
// to some sensible default.
void make_assignment(ivl_statement_t stmt)
{
   list<cpp_var_ref*> lvals;
   if (!assignment_lvals(stmt, lvals))
      assert(false);

   assert(lvals.size() != 0);
   ivl_expr_t rval = ivl_stmt_rval(stmt);
   ivl_expr_type_t type = ivl_expr_type(rval);
   // FIXME: strong assumption here
   assert(type == IVL_EX_NUMBER);
   assert(ivl_expr_width(rval) == 1);
   assert(ivl_expr_bits(rval)[0] == '1' || ivl_expr_bits(rval)[0] == '0');
   boost::tribool value = (ivl_expr_bits(rval)[0] == '1') ? true : false;

   cpp_var_ref *lhs = lvals.front();
   cppClass *thisclass = get_active_class();

   assert(thisclass->get_name() == "top");
   cpp_decl *decl = thisclass->get_scope()->get_decl(lhs->get_name());
   assert(decl);

   define_value(thisclass, decl->get_name(), value);
}

static int draw_assign(ivl_statement_t stmt)
{
   make_assignment(stmt);
   return 0;
}

/*
 * Generate C++ for a block of Verilog statements. If this block
 * doesn't have its own scope then this function does nothing, other
 * than recursively translate the block's statements and add them
 * to the process. This is OK as the stmt_container class behaves
 * like a Verilog block.
 *
 * If this block has its own scope with local variables then these
 * are added to the process as local variables and the statements
 * are generated as above.
 */
static int draw_block(ivl_statement_t stmt)
{
   ivl_scope_t block_scope = ivl_stmt_block_scope(stmt);
   if (block_scope) {
      int nsigs = ivl_scope_sigs(block_scope);
      for (int i = 0; i < nsigs; i++) {
         assert(false);
      }
   }

   int count = ivl_stmt_block_count(stmt);
   for (int i = 0; i < count; i++) {
      ivl_statement_t stmt_i = ivl_stmt_block_stmt(stmt, i);
      if (draw_stmt(stmt_i) != 0)
         return 1;
   }
   return 0;
}

/*
 *
 */
int draw_stmt(ivl_statement_t stmt)
{
   assert(stmt);

   switch (ivl_statement_type(stmt)) {
      case IVL_ST_BLOCK:
         return draw_block(stmt);
      case IVL_ST_ASSIGN:
         return draw_assign(stmt);
      case IVL_ST_ASSIGN_NB:
      case IVL_ST_CASSIGN:
      case IVL_ST_CONDIT:
      case IVL_ST_FOREVER:
      case IVL_ST_CASE:
      case IVL_ST_STASK:
      case IVL_ST_NOOP:
      case IVL_ST_DELAY:
      case IVL_ST_DELAYX:
      case IVL_ST_WAIT:
      case IVL_ST_WHILE:
      case IVL_ST_REPEAT:
      case IVL_ST_UTASK:
      case IVL_ST_FORCE:
      case IVL_ST_RELEASE:
      case IVL_ST_DISABLE:
      case IVL_ST_CASEX:
      case IVL_ST_CASEZ:
      case IVL_ST_FORK:
      case IVL_ST_DEASSIGN:
      default:
         error("No translation for statement at %s:%d (type = %d)",
               ivl_stmt_file(stmt), ivl_stmt_lineno(stmt),
               ivl_statement_type(stmt));
         return 1;
   }
}
