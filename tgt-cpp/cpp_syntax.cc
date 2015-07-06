/*
 *  C++ abstract syntax elements.
 *
 *  Copyright (C) 2008-2013  Michele Castellana (michele.castellana@mail.polimi.it)
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

#include "cpp_syntax.hh"
#include "cpp_helper.hh"
#include "cpp_target.h"

#include <cassert>
#include <cstring>
#include <iostream>
#include <typeinfo>
#include <algorithm>
#include <iomanip>

using namespace std;

void cpp_scope::add_decl(cpp_decl *decl)
{
   decls_.push_back(decl);
}

cpp_decl *cpp_scope::get_decl(const std::string &name) const
{
   std::list<cpp_decl*>::const_iterator it;
   for (it = decls_.begin(); it != decls_.end(); ++it) {
      if (strcasecmp((*it)->get_name().c_str(), name.c_str()) == 0)
         return *it;
   }

   return parent_ ? parent_->get_decl(name) : NULL;
}

bool cpp_scope::have_declared(const std::string &name) const
{
   return get_decl(name) != NULL;
}

// True if `name' differs in all but case from another declaration
bool cpp_scope::name_collides(const string& name) const
{
   const cpp_decl* decl = get_decl(name);
   if (decl)
      return strcasecmp(decl->get_name().c_str(), name.c_str()) == 0;
   else
      return false;
}

bool cpp_scope::contained_within(const cpp_scope *other) const
{
   if (this == other)
      return true;
   else if (NULL == parent_)
      return false;
   else
      return parent_->contained_within(other);
}

cpp_scope *cpp_scope::get_parent() const
{
   assert(parent_);
   return parent_;
}

int cpp_expr::paren_levels(0);

void cpp_binop_expr::emit(std::ostream &of, int level) const
{
   if(operands_.empty())
      return;
   open_parens(of);
   std::list<cpp_expr*>::const_iterator it = operands_.begin();

   (*it)->emit(of, level);
   while(++it != operands_.end())
   {
      of << " ";
      switch(op_)
      {
         case CPP_BINOP_EQ:
            of << "==";
            break;
         case CPP_BINOP_AND:
            of << "and";
            break;
         case CPP_BINOP_OR:
            of << "or";
            break;
         case CPP_BINOP_NEQ:
            of << "!=";
            break;
         case CPP_BINOP_ADD:
            of << "+";
            break;
         case CPP_BINOP_SUB:
            of << "-";
            break;
         default:
            error("This binary operation is not supported");
      }
      (*it)->emit(of, level);
   }
   close_parens(of);
}

void cpp_const_expr::emit(std::ostream &of, int) const
{
   switch(get_type()->get_name())
   {
      // Only primitive types are handled here
      case CPP_TYPE_INT:
      case CPP_TYPE_UNSIGNED_INT:
         of << value_;
         break;
      case CPP_TYPE_STD_STRING:
         of << "\"" << value_ << "\"";
         break;
      default:
         error("This constant type is not supported");
   }
}

void cpp_var_ref::emit(std::ostream &of, int level) const
{
   of << " ";
   if(!name_.empty())
      of << name_;
   else
      type_->emit(of, level);
}

void cpp_expr::open_parens(std::ostream& of)
{
   if (paren_levels++ > 0)
      of << "(";
}

void cpp_expr::close_parens(std::ostream& of)
{
   assert(paren_levels > 0);

   if (--paren_levels > 0)
      of << ")";
}

void cpp_assign_stmt::emit(std::ostream &of, int level) const
{
   if(is_instantiation)
   {
      lhs_->emit(of, level);
      of << "(";
      rhs_->emit(of, level);
      of << ")";
   }
   else
   {
      lhs_->emit(of, level);
      of << " =";
      rhs_->emit(of, level);
   }
}

void cpp_expr_list::emit(std::ostream &of, int level) const
{
   if(!children_.empty())
      emit_children<cpp_expr>(of, children_, indent(level), ",", false);
}

cppClass::cppClass(const string& name, cpp_class_type type)
      : name_(name), type_(type)
{
   cpp_function* constr = new cpp_function(name_.c_str(), new cpp_type(CPP_TYPE_NOTYPE));
   constr->set_constructor();
   constr->set_comment("Default constructor");
   add_function(constr);
   switch(type_)
   {
      case CPP_CLASS_WARPED_EVENT:
         add_event_functions();
         break;
      case CPP_CLASS_WARPED_SIM_OBJ:
         add_simulation_functions();
         break;
      default:
         error("Class not handled yet");
   }
}

void cppClass::add_event_functions()
{
   // Create inherited function "receiverName"
   cpp_type* const_ref_string_type = new cpp_type(CPP_TYPE_STD_STRING);
   const_ref_string_type->set_const();
   const_ref_string_type->set_reference();
   cpp_function *rec_name = new cpp_function("receiverName", const_ref_string_type);
   rec_name->set_comment("Inherited getter method");
   rec_name->set_const();
   rec_name->set_virtual();
   rec_name->set_override();
   // Create inherited function "timestamp"
   cpp_type *timestamp_type = new cpp_type(CPP_TYPE_UNSIGNED_INT);
   cpp_function *timestamp = new cpp_function("timestamp", timestamp_type);
   timestamp->set_comment("Inherited getter method");
   timestamp->set_const();
   timestamp->set_virtual();
   timestamp->set_override();
   // Create values to return
   cpp_var *receiver_var = new cpp_var("receiver_name", const_ref_string_type);
   cpp_var *timestamp_var = new cpp_var("ts_", timestamp_type);
   add_var(receiver_var);
   add_var(timestamp_var);
   add_function(rec_name);
   add_function(timestamp);
   // Create the members that simulation objects will use
   cpp_var * signal_value = new cpp_var("new_value_", new cpp_type(CPP_TYPE_BOOST_TRIBOOL));
   signal_value->set_comment("The new input value");
   cpp_var * signal_name = new cpp_var("changed_signal_name", const_ref_string_type);
   signal_name->set_comment("The changed input signal name");
   add_var(signal_value);
   add_var(signal_name);
   // Create return statements
   timestamp->add_stmt(new cpp_unaryop_expr(CPP_UNARYOP_RETURN, timestamp_var->get_ref(), timestamp_var->get_type()));
   rec_name->add_stmt(new cpp_unaryop_expr(CPP_UNARYOP_RETURN, receiver_var->get_ref(), receiver_var->get_type()));
   // Add init list to constructor.
   cpp_function* constr = get_costructor();
   cpp_var *name = new cpp_var("name", const_ref_string_type);
   constr->add_param(name);
   cpp_fcall_stmt* init_name = new cpp_fcall_stmt(const_ref_string_type, receiver_var->get_ref(), "");
   init_name->add_param(name->get_ref());
   constr->add_init(init_name);
}

/*
 * TODO:
 * This function starts to be tough.
 * Refactory needed.
 */
void cppClass::add_simulation_functions()
{
   // Create vars
   cpp_type* inside_input_map = new cpp_type(CPP_TYPE_NOTYPE, new cpp_type(CPP_TYPE_BOOST_TRIBOOL));
   inside_input_map->add_type(new cpp_type(CPP_TYPE_STD_STRING));
   cpp_var *inputvar = new cpp_var("inputs_", new cpp_type(CPP_TYPE_STD_MAP, inside_input_map));
   inputvar->set_comment("std::map< signal_name, value >");
   add_var(inputvar);
   cpp_type* output_vec = new cpp_type(CPP_TYPE_STD_VECTOR, new cpp_type(CPP_TYPE_STD_STRING));
   cpp_type* output_map_type = new cpp_type(CPP_TYPE_STD_MAP, output_vec);
   output_map_type->add_type(new cpp_type(CPP_TYPE_STD_STRING));
   cpp_var *output_var = new cpp_var("hierarchy_", output_map_type);
   output_var->set_comment("map<submodule, vector<signals>");
   add_var(output_var);
   cpp_var *state_var = new cpp_var("state_", new cpp_type(CPP_TYPE_ELEMENT_STATE));
   state_var->set_comment("The State variable");
   add_var(state_var);
   cpp_type *returnType = new cpp_type(CPP_TYPE_STD_VECTOR,
         new cpp_type(CPP_TYPE_SHARED_PTR,
         new cpp_type(CPP_TYPE_WARPED_EVENT)));
   // Create initial event function
   cpp_function *init_fun = new cpp_function(WARPED_INIT_EVENT_FUN_NAME, returnType);
   init_fun->set_override();
   init_fun->set_virtual();
   // Create event handler function
   cpp_function *event_handler = new cpp_function(WARPED_HANDLE_EVENT_FUN_NAME, returnType);
   event_handler->set_override();
   event_handler->set_virtual();
   cpp_type *event_type = new cpp_type(CPP_TYPE_WARPED_EVENT);
   event_type->set_reference();
   event_type->set_const();
   cpp_var *event_param = new cpp_var("event", event_type);
   event_handler->add_param(event_param);
   // Create getState function
   cpp_type *get_state_ret_type = new cpp_type(CPP_TYPE_WARPED_OBJECT_STATE);
   get_state_ret_type->set_reference();
   cpp_function *get_state_fun = new cpp_function("getState", get_state_ret_type);
   get_state_fun->add_stmt(new cpp_unaryop_expr(CPP_UNARYOP_RETURN, state_var->get_ref(), state_var->get_type()));
   get_state_fun->set_override();
   get_state_fun->set_virtual();
   // Add all functions to the class
   add_function(init_fun);
   add_function(get_state_fun);
   add_function(event_handler);
   // Add init list to the constructor
   cpp_function* constr = get_costructor();
   cpp_type* const_ref_string_type = new cpp_type(CPP_TYPE_STD_STRING);
   const_ref_string_type->set_const();
   const_ref_string_type->set_reference();
   cpp_var *name = new cpp_var("name", const_ref_string_type);
   constr->add_param(name);
   cpp_var_ref* sim_obj = new cpp_var_ref("", new cpp_type(CPP_TYPE_WARPED_SIMULATION_OBJECT));
   cpp_fcall_stmt* init_name = new cpp_fcall_stmt(new cpp_type(CPP_TYPE_WARPED_SIMULATION_OBJECT), sim_obj, "");
   init_name->add_param(name->get_ref());
   constr->add_init(init_name);
   // Add statements to functions
   cpp_type *local_event_type = new cpp_type(CPP_TYPE_CUSTOM_EVENT);
   local_event_type->set_const();
   local_event_type->set_reference();
   cpp_var *local_event = new cpp_var("my_event", local_event_type);
   // Start handling the event
   cpp_assign_stmt *cast_stmt = new cpp_assign_stmt(new cpp_unaryop_expr(CPP_UNARYOP_DECL, local_event->get_ref(), local_event->get_type()), new cpp_unaryop_expr(CPP_UNARYOP_STATIC_CAST, event_param->get_ref(), local_event->get_type()));
   event_handler->add_stmt(cast_stmt);

   cpp_type* input_type = new cpp_type(CPP_TYPE_STD_MAP, inside_input_map);
   input_type->set_iterator();
   cpp_var* iterator = new cpp_var ("it", input_type);
   cpp_assign_stmt* precycle = new cpp_assign_stmt(new cpp_unaryop_expr(CPP_UNARYOP_DECL, iterator->get_ref(), iterator->get_type()), new cpp_fcall_stmt(iterator->get_type(), inputvar->get_ref(), "begin"));
   cpp_for * change_input_for = new cpp_for();
   change_input_for->add_precycle(precycle);
   cpp_binop_expr * cond = new cpp_binop_expr(CPP_BINOP_NEQ, iterator->get_type());
   cond->add_expr(new cpp_unaryop_expr(CPP_UNARYOP_LITERAL, iterator->get_ref(), iterator->get_type()));
   cond->add_expr(new cpp_fcall_stmt(iterator->get_type(), inputvar->get_ref(), "end"));
   change_input_for->set_condition(cond);
   change_input_for->add_postcycle(new cpp_unaryop_expr(CPP_UNARYOP_ADD, iterator->get_ref(), iterator->get_type()));
   event_handler->add_stmt(change_input_for);
}

cpp_function* cppClass::get_function(const std::string &name) const
{
   cpp_decl* temp = scope_.get_decl(name);
   assert(temp);
   cpp_function* retvalue = dynamic_cast<cpp_function*>(temp);
   assert(retvalue);
   return retvalue;
}

void cpp_context::emit_after_classes(std::ostream &of, int level) const
{
   newline(of, level);
   of << "int main(int argc, const char** argv) {";
   emit_children<cpp_stmt>(of, statements_, indent(level), ";");
   newline(of, level);
   of << "}; ";
}

void cpp_for::set_condition(cpp_expr* p) {
   assert(!condition_);
   condition_ = p;
};

void cpp_for::emit(std::ostream &of, int level) const
{
   newline(of, level);
   of << "for(";
   if(!precycle_.empty())
      emit_children<cpp_expr>(of, precycle_, indent(level), ",", false);
   of << "; ";
   if(condition_ != NULL)
      condition_->emit(of);
   of << "; ";
   if(!postcycle_.empty())
      emit_children<cpp_expr>(of, postcycle_, indent(level), ",", false);
   of << "){ ";
   newline(of, level);
   if(!statements_.empty())
      emit_children<cpp_stmt>(of, statements_, indent(level), ";");
   of << "}";
}

void cpp_context::emit_before_classes(std::ostream &of, int level) const
{
   newline(of, level);
   of << "WARPED_DEFINE_OBJECT_STATE_STRUCT(" << cpp_type::tostring(CPP_TYPE_ELEMENT_STATE) << "){";
   if(!elem_parts_.empty())
      emit_children<cpp_var>(of, elem_parts_, indent(level), ";");
   of << "};";
   newline(of, level);
}

void cppClass::emit(std::ostream &of, int level) const
{
   newline(of, level);
   emit_comment(of, level);
   of << "class " << name_;
   switch(type_)
   {
      case CPP_CLASS_WARPED_SIM_OBJ:
         of << " : public warped::SimulationObject";
         break;
      case CPP_CLASS_WARPED_EVENT:
         of << " : public warped::Event";
         break;
      default:
         error("Class not handled yet");
   }
   of << " {";
   newline(of, level);
   of << "public:";

   if (!scope_.empty()) {
      newline(of, indent(level));
      emit_children<cpp_decl>(of, scope_.get_decls(), indent(level), ";");
   }

   newline(of, level);
   of << "}; ";
   newline(of, indent(level));
}

const cpp_type *cpp_decl::get_type() const
{
   assert(type_);
   return type_;
}

void cpp_unaryop_expr::emit(std::ostream &of, int level) const
{
   open_parens(of);

   switch (op_) {
   case CPP_UNARYOP_NOT:
       of << "not ";
      break;
   case CPP_UNARYOP_STATIC_CAST:
       of << "static_cast<";
       type_->emit(of);
       of << ">( ";
      break;
   case CPP_UNARYOP_LITERAL:
      break;
   case CPP_UNARYOP_DECL:
      operand_->get_type()->emit(of, level);
      break;
   case CPP_UNARYOP_NEG:
       of << "-";
      break;
   case CPP_UNARYOP_ADD:
      break;
   case CPP_UNARYOP_RETURN:
      of << "return";
      break;
   default:
      error("Unary operation not supported");
   }
   operand_->emit(of, level);

   // Cast needs to close the parenthesis
   if(op_ == CPP_UNARYOP_STATIC_CAST)
       of << " )";
   
   if(op_ == CPP_UNARYOP_ADD)
       of << "++";
   close_parens(of);
}

void cpp_fcall_stmt::emit(std::ostream &of, int level) const
{
   base_->emit(of, level);
   if(!fun_name_.empty())
      of << "." << fun_name_;
   of << "(";
   parameters_->emit(of, level);
   of << ")";
}

void cpp_function::emit(std::ostream &of, int level) const
{
   emit_comment(of, level);
   if(isvirtual)
      of << "virtual ";
   type_->emit(of, level);
   of << " " << name_ << " (";
   if(!scope_.get_decls().empty())
      emit_children<cpp_decl>(of, scope_.get_decls(), indent(level), ",", false);
   of << ")";
   if(isconst)
      of << " const";
   if(isoverride)
      of << " override";
   if(isconstructor)
   {
      of << " : ";
      emit_children<cpp_fcall_stmt>(of, init_list_, indent(level), ",", false);
   }
   of << " {";
   if(!statements_.empty())
      emit_children<cpp_stmt>(of, statements_, indent(level), ";");
   of << "}";
}

cpp_var_ref* cpp_var::get_ref()
{
   if(ref_to_this_var == NULL)
      ref_to_this_var = new cpp_var_ref(name_, type_);
   return ref_to_this_var;
}

void cpp_var::emit(std::ostream &of, int level) const
{
   emit_comment(of, level);
   type_->emit(of, level);
   of << " " << name_;
}
