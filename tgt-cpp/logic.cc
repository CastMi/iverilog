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
#include "hierarchy.hh"
#include "cpp_type.hh"

#include <cassert>
#include <sstream>
#include <iostream>

static void inputs_to_expr(cppClass *theclass, cpp_class_type type,
                                 ivl_net_logic_t log)
{
   remember_logic(type);
   submodule *temp = new submodule(type);
   // The single output
   ivl_nexus_t output = ivl_logic_pin(log, 0);
   assert(output);
   cpp_var_ref* tmp = readable_ref(theclass->get_scope(), output);
   temp->insert_output(tmp->get_name(), tmp->get_name());
   
   // All the inputs
   int npins = ivl_logic_pins(log);
   for (int i = 1; i < npins; i++) {
      ivl_nexus_t pin = ivl_logic_pin(log, i);
      assert(pin);

      tmp = readable_ref(theclass->get_scope(), pin);
      temp->insert_input(tmp->get_name(), tmp->get_name());
   }

   add_submodule_to(temp, theclass);
}

static cppClass* input_to_expr(cppClass *theclass, cpp_unaryop_t,
                                ivl_net_logic_t log)
{
   cppClass *theop = new cppClass(CPP_CLASS_AND);
   // The single output
   ivl_nexus_t pin = ivl_logic_pin(log, 0);
   assert(pin);
   cpp_var_ref* tmp = readable_ref(theclass->get_scope(), pin);
   theop->add_var(new cpp_var(tmp->get_name(), tmp->get_type()));
   // The single input
   pin = ivl_logic_pin(log, 1);
   tmp = readable_ref(theclass->get_scope(), pin);
   theop->add_to_inputs(new cpp_var(tmp->get_name(), tmp->get_type()));
   return theop;
}

void translate_logic(cppClass *theclass, ivl_net_logic_t log)
{
   switch (ivl_logic_type(log)) {
   case IVL_LO_AND:
   case IVL_LO_OR:
   case IVL_LO_NAND:
   case IVL_LO_NOR:
   case IVL_LO_XOR:
   case IVL_LO_XNOR:
      // FIXME: right now they are translated as AND
      inputs_to_expr(theclass, CPP_CLASS_AND, log);
      return;
   case IVL_LO_NOT:
   case IVL_LO_PULLUP:
   case IVL_LO_PULLDOWN:
   case IVL_LO_BUF:
   case IVL_LO_BUFT:
   case IVL_LO_BUFZ:
   default:
      error("Don't know how to translate type %d to expression",
            ivl_logic_type(log));
      return;
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
      }
   default:
      {
         translate_logic(theclass, log);
      }
   }
}
