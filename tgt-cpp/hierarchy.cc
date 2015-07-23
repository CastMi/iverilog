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

static std::list<submodule*> modules;
static std::list<submodule*> seen_modules;

void remember_hierarchy(cppClass* theclass)
{
   submodule *temp = new submodule(theclass);
   for(std::list<submodule*>::iterator it = modules.begin();
         it != modules.end() ; it++)
      if((*it)->relate_class->get_name() == theclass->get_name())
         assert(false);
   modules.push_front(temp);
}

submodule* submodule::find(const cppClass* tofind)
{
   if(tofind->get_name() == relate_class->get_name())
      return this;
   if(hierarchy.empty())
      return NULL;
   submodule* tmp = NULL;
   for(std::list<submodule*>::iterator it = hierarchy.begin();
         it != hierarchy.end(); it++)
   {
      if((*it)->type == CPP_CLASS_MODULE)
      {
         tmp = find(tofind);
         if(tmp != NULL)
            return tmp;
         assert(tmp == NULL);
      }
   }
   assert(false);
}

submodule* add_submodule_to(submodule* item, cppClass* parent)
{
   if(item->type == CPP_CLASS_MODULE)
   {
      bool flag = true;
      for(std::list<submodule*>::iterator it = modules.begin();
            it != modules.end() ; it++)
      {
         if((*it)->relate_class->get_name() == item->relate_class->get_name())
         {
            item->merge((*it));
            seen_modules.push_front(*it);
            modules.remove(*it);
            flag = false;
            break;
         }
      }
      for(std::list<submodule*>::iterator it = seen_modules.begin();
            it != seen_modules.end() && flag ; it++)
      {
         if((*it)->relate_class->get_name() == item->relate_class->get_name())
         {
            item->merge((*it));
            flag = false;
            break;
         }
      }
      assert(!flag);
   }
   for(std::list<submodule* >::iterator it = modules.begin();
         it != modules.end() ; it++)
   {
      if((*it)->type == CPP_CLASS_MODULE)
      {
         submodule* found = (*it)->find(parent);
         if(found != NULL)
         {
            found->add_submodule(item);
            return found;
         }
      }
   }
   assert(false);
   return NULL;
}

void submodule::merge(submodule* el)
{
   hierarchy.insert(hierarchy.end(), el->hierarchy.begin(), el->hierarchy.end());
}

void submodule::insert_output(const std::string& str1, const std::string& str2)
{
   assert(!str1.empty() ||  !str2.empty());
   outputs_map.push_front(std::pair<std::string, std::string>(str1, str2));
}

void submodule::insert_input(const std::string& str1, const std::string& str2)
{
   assert(!str1.empty() ||  !str2.empty());
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

static cpp_type* port_pointer_type = new cpp_type(CPP_TYPE_CUSTOM_BASE_CLASS);
static const cpp_type* no_type = new cpp_type(CPP_TYPE_NOTYPE);
static cpp_type* sim_obj_pointer_type = new cpp_type(CPP_TYPE_WARPED_SIMULATION_OBJECT);
static const cpp_type* string_type = new cpp_type(CPP_TYPE_STD_STRING);

static std::string recursive_build(submodule* current, std::string father_name,
      std::list<cpp_stmt*>* list)
{
   assert(current->type == CPP_CLASS_MODULE);
   assert(current->relate_class != NULL);
   const cpp_type* vector = new cpp_type(CPP_TYPE_STD_VECTOR, sim_obj_pointer_type);
   cpp_var* vector_var = new cpp_var("object_pointers", vector);
   cpp_unaryop_expr* obj_pointers_literal = new cpp_unaryop_expr(CPP_UNARYOP_LITERAL, vector_var->get_ref(), vector_var->get_type());
   std::string my_name = get_unique_name(current->type);
   cpp_var_ref* expr_name = new cpp_var_ref(my_name, port_pointer_type);
   cpp_unaryop_expr* unary = new cpp_unaryop_expr(CPP_UNARYOP_DECL, expr_name, port_pointer_type);
   cpp_const_expr* cur_class_name = new cpp_const_expr(current->relate_class->get_name().c_str(), no_type);
   cpp_fcall_stmt* module_constr = new cpp_fcall_stmt(no_type, cur_class_name, "");
   module_constr->add_param(new cpp_const_expr(my_name.c_str(), string_type));
   cpp_unaryop_expr* new_module = new cpp_unaryop_expr(CPP_UNARYOP_NEW, module_constr, port_pointer_type);
   list->push_back(new cpp_assign_stmt(unary, new_module));
   // From now on, manage interconnections among my children.
   for(std::list<submodule*>::iterator it = current->hierarchy.begin();
         it != current->hierarchy.end(); it++)
   {
      std::string submodule_name;
      cpp_const_expr* sub_string_name;
      cpp_var_ref* sub_var_ref;
      cpp_const_expr* class_name_lhs;
      if((*it)->type == CPP_CLASS_MODULE)
      {
         assert((*it)->relate_class != NULL);
         // This child is another module: I need to build it recursively
         submodule_name = recursive_build(*it, my_name, list);
         sub_string_name = new cpp_const_expr(submodule_name.c_str(), string_type);
         sub_var_ref = new cpp_var_ref(submodule_name, port_pointer_type);
         class_name_lhs = new cpp_const_expr((*it)->relate_class->get_name().c_str(), no_type);
         for(std::list<std::pair<std::string, std::string> >::iterator input_it = (*it)->signal_mapping.begin();
               input_it != (*it)->signal_mapping.end(); input_it++)
         {
            cpp_const_expr* supermod_signal = new cpp_const_expr((*input_it).second.c_str(), string_type);
            cpp_const_expr* submod_signal = new cpp_const_expr((*input_it).first.c_str(), string_type);
            // Add The input of the logic port to the outputs of the module
            cpp_fcall_stmt* add_out_to_module = new cpp_fcall_stmt(no_type, expr_name, ADD_OUTPUT_FUN_NAME);
            add_out_to_module->set_pointer_call();
            add_out_to_module->add_param(supermod_signal);
            add_out_to_module->add_param(sub_string_name);
            add_out_to_module->add_param(submod_signal);
            list->push_back(add_out_to_module);
         }
         continue;
      }
      else
      {
         assert((*it)->relate_class == NULL && (*it)->outputs_map.size() == 1);
         submodule_name = get_unique_name(current->type);
         sub_string_name = new cpp_const_expr(submodule_name.c_str(), string_type);
         sub_var_ref = new cpp_var_ref(submodule_name, port_pointer_type);
         class_name_lhs = new cpp_const_expr(get_class_name((*it)->type).c_str(), no_type);
      }
      // From now on, I'm sure that I'm dealing with a gate
      // Create the current submodule
      cpp_unaryop_expr* sub_unary = new cpp_unaryop_expr(CPP_UNARYOP_DECL, sub_var_ref, port_pointer_type);
      cpp_fcall_stmt* port_constr = new cpp_fcall_stmt(no_type, class_name_lhs, "");
      port_constr->add_param(sub_string_name);
      cpp_unaryop_expr* new_port = new cpp_unaryop_expr(CPP_UNARYOP_NEW, port_constr, port_pointer_type);
      list->push_back(new cpp_assign_stmt(sub_unary, new_port));
      // Add all the inputs
      for(std::list<std::pair<std::string, std::string> >::iterator input_it = (*it)->signal_mapping.begin();
            input_it != (*it)->signal_mapping.end(); input_it++ )
      {
         assert((*input_it).second == (*input_it).first);
         cpp_const_expr* supermod_signal = new cpp_const_expr((*input_it).second.c_str(), string_type);
         cpp_const_expr* submod_signal = new cpp_const_expr((*input_it).first.c_str(), string_type);
         // Add signal to gates. Modules don't need.
         cpp_fcall_stmt* add_in_to_port = new cpp_fcall_stmt(no_type, sub_var_ref, ADD_SIGNAL_FUN_NAME);
         add_in_to_port->set_pointer_call();
         add_in_to_port->add_param(supermod_signal);
         list->push_back(add_in_to_port);
         // Add The input of the logic port to the outputs of the module
         cpp_fcall_stmt* add_out_to_module = new cpp_fcall_stmt(no_type, expr_name, ADD_OUTPUT_FUN_NAME);
         add_out_to_module->set_pointer_call();
         add_out_to_module->add_param(supermod_signal);
         add_out_to_module->add_param(sub_string_name);
         add_out_to_module->add_param(submod_signal);
         list->push_back(add_out_to_module);
      }
      for(std::list<std::pair<std::string, std::string> >::iterator output_it = (*it)->outputs_map.begin();
            output_it != (*it)->outputs_map.end(); output_it++ )
      {
         assert((*output_it).second == (*output_it).first);
         // Add all the outputs
         cpp_fcall_stmt* add_out_to_port = new cpp_fcall_stmt(no_type, sub_var_ref, ADD_OUTPUT_FUN_NAME);
         add_out_to_port->set_pointer_call();
         add_out_to_port->add_param(new cpp_const_expr((*output_it).first.c_str(), string_type));
         // The name of the current module
         cpp_const_expr* my_string_name = new cpp_const_expr(my_name.c_str(), string_type);
         add_out_to_port->add_param(my_string_name);
         add_out_to_port->add_param(new cpp_const_expr((*output_it).second.c_str(), string_type));
         list->push_back(add_out_to_port);
      }
      // We have added input/output. Now, add the submodule
      cpp_fcall_stmt* push_port = new cpp_fcall_stmt(no_type, obj_pointers_literal, "push_back");
      push_port->add_param(sub_var_ref);
      list->push_back(push_port);
   }
   // Unless I'm the top module, I need to inform my supermodule that my outputs are changed
   if(!father_name.empty())
      for(std::list<std::pair<std::string, std::string> >::iterator out_it = current->outputs_map.begin();
            out_it != current->outputs_map.end(); out_it++ )
      {
         cpp_const_expr* supermod_signal = new cpp_const_expr((*out_it).first.c_str(), string_type);
         cpp_const_expr* submod_signal = new cpp_const_expr((*out_it).second.c_str(), string_type);
         // Add The input of the logic port to the outputs of the module
         cpp_const_expr* father_string_name = new cpp_const_expr(father_name.c_str(), string_type);
         cpp_fcall_stmt* add_out_to_module = new cpp_fcall_stmt(no_type, expr_name, ADD_OUTPUT_FUN_NAME);
         add_out_to_module->set_pointer_call();
         add_out_to_module->add_param(submod_signal);
         add_out_to_module->add_param(father_string_name);
         add_out_to_module->add_param(supermod_signal);
         list->push_back(add_out_to_module);
      }
   // Push the module
   cpp_fcall_stmt* push_module = new cpp_fcall_stmt(no_type, obj_pointers_literal, "push_back");
   push_module->add_param(expr_name);
   list->push_back(push_module);
   return my_name;
}

std::list<cpp_stmt*> build_hierarchy()
{
   std::list<cpp_stmt*> return_value;
   port_pointer_type->set_pointer();
   // Create the simulation variable
   cpp_var_ref* lhs = new cpp_var_ref("this_sim", new cpp_type(CPP_TYPE_WARPED_SIMULATION));
   cpp_unaryop_expr* sim_decl = new cpp_unaryop_expr(CPP_UNARYOP_DECL, lhs, lhs->get_type());
   cpp_expr_list* rhs = new cpp_expr_list();
   rhs->add_expr(new cpp_const_expr("Logic simulation", string_type));
   rhs->add_expr(new cpp_var_ref("argc", no_type));
   rhs->add_expr(new cpp_var_ref("argv", no_type));
   cpp_assign_stmt* sim_init = new cpp_assign_stmt(sim_decl, rhs, true);
   return_value.push_back(sim_init);
   // Create the object vector
   sim_obj_pointer_type->set_pointer();
   const cpp_type* vector = new cpp_type(CPP_TYPE_STD_VECTOR, sim_obj_pointer_type);
   cpp_var* vector_var = new cpp_var("object_pointers", vector);
   cpp_unaryop_expr *output_var = new cpp_unaryop_expr(CPP_UNARYOP_DECL, vector_var->get_ref(), vector_var->get_type());
   output_var->set_comment("Object list to pass to warped kernel");
   return_value.push_back(output_var);
   // Analyze all the top modules
   for(std::list<submodule*>::iterator class_it = modules.begin();
         class_it != modules.end(); class_it++ )
   {
      assert((*class_it)->relate_class != NULL);
      recursive_build(*class_it, "", &return_value);
   }
   // Add the final instruction to start the simulation
   cpp_fcall_stmt* start_sim = new cpp_fcall_stmt(no_type, lhs, "simulate");
   start_sim->set_comment("Start simulation");
   start_sim->add_param(vector_var->get_ref());
   return_value.push_back(start_sim);
   return return_value;
}
