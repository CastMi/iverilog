/*
 *  Managing interconnections among objects in the generated C++ code.
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

#include "hierarchy.hh"
#include "cpp_target.h"
#include <map>
#include <sstream>

static std::map<std::string, std::list<submodule* > > class_to_submodules;

static std::list<submodule*> get_submodule_list(const std::string& name)
{
   assert(class_to_submodules.find(name) != class_to_submodules.end());

   return class_to_submodules[name];
}

void add_submodule_to(const std::string& name, submodule* item)
{
   class_to_submodules[name].push_front(item);
}

void submodule::insert_output(const std::string& el)
{
   output_name.push_front(el);
}

void submodule::insert_input(const std::string& str1, const std::string& str2)
{
   signal_mapping.push_front(std::pair<std::string, std::string>(str1, str2));
}

static unsigned int andPort(0);
static unsigned int modulenum(0);

static std::string get_unique_name(cpp_class_type type)
{
   std::ostringstream ss;

   switch(type)
   {
      case CPP_CLASS_AND:
            ss << "andPort" << andPort++;
            break;
      case CPP_CLASS_MODULE:
            ss << "module" << modulenum++;
            break;
      default:
         error("Cannot find a unique name for this logic port");
   }
   return ss.str();
}

static std::string get_class_name(cpp_class_type type)
{
   std::ostringstream ss;

   switch(type)
   {
      case CPP_CLASS_AND:
            ss << "And";
            break;
      default:
         error("Cannot find a unique name for this logic port");
   }
   return ss.str();
}

std::list<cpp_stmt*> build_hierarchy(std::list<cppClass*> class_list)
{
   std::list<cpp_stmt*> return_value;
   bool isfirst = true;
   const cpp_type* string_type = new cpp_type(CPP_TYPE_STD_STRING);
   cpp_type* port_pointer_type = new cpp_type(CPP_TYPE_CUSTOM_BASE_CLASS);
   port_pointer_type->set_pointer();
   // Create the simulation variable
   cpp_var_ref* lhs = new cpp_var_ref("this_sim", new cpp_type(CPP_TYPE_WARPED_SIMULATION));
   cpp_unaryop_expr* sim_decl = new cpp_unaryop_expr(CPP_UNARYOP_DECL, lhs, lhs->get_type());
   cpp_expr_list* rhs = new cpp_expr_list();
   rhs->add_cpp_expr(new cpp_const_expr("Logic simulation", string_type));
   rhs->add_cpp_expr(new cpp_var_ref("argc", new cpp_type(CPP_TYPE_NOTYPE)));
   rhs->add_cpp_expr(new cpp_var_ref("argv", new cpp_type(CPP_TYPE_NOTYPE)));
   cpp_assign_stmt* sim_init = new cpp_assign_stmt(sim_decl, rhs, true);
   return_value.push_back(sim_init);
   // Create the object vector
   cpp_type* sim_obj_pointer_type = new cpp_type(CPP_TYPE_WARPED_SIMULATION_OBJECT);
   sim_obj_pointer_type->set_pointer();
   const cpp_type* vector = new cpp_type(CPP_TYPE_STD_VECTOR, sim_obj_pointer_type);
   cpp_var* vector_var = new cpp_var("object_pointers", vector);
   cpp_unaryop_expr *output_var = new cpp_unaryop_expr(CPP_UNARYOP_DECL, vector_var->get_ref(), vector_var->get_type());
   output_var->set_comment("Object list to pass to warped kernel");
   return_value.push_back(output_var);
   cpp_unaryop_expr* obj_pointers_literal = new cpp_unaryop_expr(CPP_UNARYOP_LITERAL, vector_var->get_ref(), vector_var->get_type());
   const cpp_type* no_type = new cpp_type(CPP_TYPE_NOTYPE);
   // From now on, analyze all the stored informations
   for(std::list<cppClass*>::iterator class_it = class_list.begin();
         class_it != class_list.end(); class_it++ )
   {
      if((*class_it)->get_inherited() != CPP_INHERIT_BASE_CLASS ||
         (*class_it)->get_type() != CPP_CLASS_MODULE)
         continue;
      // Create module instance
      cpp_var_ref* module = new cpp_var_ref("module", port_pointer_type);
      cpp_unaryop_expr* module_lhs;
      if(isfirst)
         module_lhs = new cpp_unaryop_expr(CPP_UNARYOP_DECL, module, port_pointer_type);
      else
         module_lhs = new cpp_unaryop_expr(CPP_UNARYOP_LITERAL, module, port_pointer_type);
      cpp_const_expr* module_name = new cpp_const_expr((*class_it)->get_name().c_str(), no_type);
      cpp_fcall_stmt* module_constr = new cpp_fcall_stmt(no_type, module_name, "");
      cpp_const_expr* class_name_const = new cpp_const_expr(get_unique_name((*class_it)->get_type()).c_str(), string_type);
      module_constr->add_param(class_name_const);
      cpp_unaryop_expr* module_rhs = new cpp_unaryop_expr(CPP_UNARYOP_NEW, module_constr, port_pointer_type);
      cpp_assign_stmt* module_stmt = new cpp_assign_stmt(module_lhs, module_rhs);
      return_value.push_back(module_stmt);
      // handle submodule instantiation
      std::list<submodule*> cur_submod_list = get_submodule_list((*class_it)->get_name());
      for(std::list<submodule*>::iterator submod_it = cur_submod_list.begin();
         submod_it != cur_submod_list.end(); submod_it++)
      {
         // Create the current submodule
         cpp_var_ref* new_object = new cpp_var_ref("port", port_pointer_type);
         cpp_unaryop_expr* unary;
         if(isfirst)
         {
            unary = new cpp_unaryop_expr(CPP_UNARYOP_DECL, new_object, port_pointer_type);
            isfirst = false;
         }
         else
            unary = new cpp_unaryop_expr(CPP_UNARYOP_LITERAL, new_object, port_pointer_type);
         cpp_const_expr* class_name_lhs = new cpp_const_expr(get_class_name((*submod_it)->type).c_str(), no_type);
         cpp_fcall_stmt* port_constr = new cpp_fcall_stmt(no_type, class_name_lhs, "");
         port_constr->add_param(new cpp_const_expr(get_unique_name((*submod_it)->type).c_str(), string_type));
         cpp_unaryop_expr* new_stmt = new cpp_unaryop_expr(CPP_UNARYOP_NEW, port_constr, port_pointer_type);
         cpp_assign_stmt* stmt = new cpp_assign_stmt(unary, new_stmt);
         return_value.push_back(stmt);
         // Add all the inputs
         for(std::list<std::pair<std::string, std::string> >::iterator input_it = (*submod_it)->signal_mapping.begin();
               input_it != (*submod_it)->signal_mapping.end(); input_it++ )
         {
            // Add all inputs to the port
            cpp_fcall_stmt* add_in_to_port = new cpp_fcall_stmt(no_type, new_object, ADD_SIGNAL_FUN_NAME);
            add_in_to_port->set_pointer_call();
            add_in_to_port->add_param(new cpp_const_expr((*input_it).second.c_str(), string_type));
            return_value.push_back(add_in_to_port);
         }
         for(std::list<std::string>::iterator output_it = (*submod_it)->output_name.begin();
               output_it != (*submod_it)->output_name.end(); output_it++ )
         {
            // Add all the outputs
            cpp_fcall_stmt* add_out_to_port = new cpp_fcall_stmt(no_type, new_object, ADD_OUTPUT_FUN_NAME);
            add_out_to_port->set_pointer_call();
            add_out_to_port->add_param(new cpp_const_expr((*output_it).c_str(), string_type));
            add_out_to_port->add_param(class_name_const);
            add_out_to_port->add_param(new cpp_const_expr((*output_it).c_str(), string_type));
            return_value.push_back(add_out_to_port);
         }
         // We have added input/output. Now, add the submodule
         cpp_fcall_stmt* push_port = new cpp_fcall_stmt(no_type, obj_pointers_literal, "push_back");
         push_port->add_param(new_object);
         return_value.push_back(push_port);
      }
      // Push the module
      cpp_fcall_stmt* push_module = new cpp_fcall_stmt(no_type, obj_pointers_literal, "push_back");
      push_module->add_param(module);
      return_value.push_back(push_module);
   }
   // Add the final instruction to start the simulation
   cpp_fcall_stmt* start_sim = new cpp_fcall_stmt(no_type, lhs, "simulate");
   start_sim->set_comment("Start simulation");
   start_sim->add_param(vector_var->get_ref());
   return_value.push_back(start_sim);
   return return_value;
}
