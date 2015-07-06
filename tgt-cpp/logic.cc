/*
 *  C++ code generation for logic devices.
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

#include "cpp_target.h"
#include "cpp_element.hh"
#include "state.hh"
#include "cpp_type.hh"

#include <cassert>
#include <sstream>
#include <iostream>

static cpp_expr *inputs_to_expr(cpp_scope *scope, cpp_binop_t op,
                                 ivl_net_logic_t log)
{
   cpp_binop_expr *theop =
      new cpp_binop_expr(op, new cpp_type(CPP_TYPE_INT));

   int npins = ivl_logic_pins(log);
   for (int i = 1; i < npins; i++) {
      ivl_nexus_t input = ivl_logic_pin(log, i);

      theop->add_expr(readable_ref(scope, input));
   }

   return theop;
}

static cpp_expr *input_to_expr(cpp_scope *scope, cpp_unaryop_t op,
                                ivl_net_logic_t log)
{
   ivl_nexus_t input = ivl_logic_pin(log, 1);
   assert(input);

   cpp_var_ref *operand = readable_ref(scope, input);
   return new cpp_unaryop_expr(op, operand, operand->get_type());
}

static cpp_expr *translate_logic_inputs(cpp_scope *scope, ivl_net_logic_t log)
{
   switch (ivl_logic_type(log)) {
   case IVL_LO_NOT:
      return input_to_expr(scope, CPP_UNARYOP_NOT, log);
   case IVL_LO_BUF:
   case IVL_LO_BUFT:
   case IVL_LO_BUFZ:
      return nexus_to_var_ref(scope, ivl_logic_pin(log, 1));
   case IVL_LO_AND:
      return inputs_to_expr(scope, CPP_BINOP_AND, log);
   case IVL_LO_OR:
      return inputs_to_expr(scope, CPP_BINOP_OR, log);
   case IVL_LO_NAND:
      return inputs_to_expr(scope, CPP_BINOP_NAND, log);
   case IVL_LO_NOR:
      return inputs_to_expr(scope, CPP_BINOP_NOR, log);
   case IVL_LO_XOR:
      return inputs_to_expr(scope, CPP_BINOP_XOR, log);
   case IVL_LO_XNOR:
      return inputs_to_expr(scope, CPP_BINOP_XNOR, log);
   case IVL_LO_PULLUP:
      return new cpp_const_expr("1", new cpp_type(CPP_TYPE_INT));
   case IVL_LO_PULLDOWN:
      return new cpp_const_expr("0", new cpp_type(CPP_TYPE_INT));
   default:
      return new cpp_const_expr("NULL", new cpp_type(CPP_TYPE_WARPED_SIMULATION_OBJECT));
      /*
      error("Don't know how to translate type %d to expression",
            ivl_logic_type(log));
      return NULL;
      */
   }
}

void draw_logic(cppClass *theclass, ivl_net_logic_t log)
{
   switch (ivl_logic_type(log)) {
   case IVL_LO_BUFIF0:
   case IVL_LO_BUFIF1:
   case IVL_LO_NOTIF0:
   case IVL_LO_NOTIF1:
   case IVL_LO_UDP:
      {
         error("Don't know how to translate logic type = %d to expression",
               ivl_logic_type(log));
         return;
      }
   default:
      {
         // The output is always pin zero
         ivl_nexus_t output = ivl_logic_pin(log, 0);
         cpp_var_ref *lhs = nexus_to_var_ref(theclass->get_scope(), output);
  
         cpp_expr *rhs = translate_logic_inputs(theclass->get_scope(), log);
         cpp_assign_stmt *ass = new cpp_assign_stmt(lhs, rhs);

         cpp_procedural* fun = theclass->get_function(WARPED_HANDLE_EVENT_FUN_NAME);
         fun->add_stmt(ass);
      }
   }
}
