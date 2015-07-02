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
   open_parens(of);

   assert(! operands_.empty());
   std::list<cpp_expr*>::const_iterator it = operands_.begin();

   (*it)->emit(of, level);
   while (++it != operands_.end()) {
      const char* ops[] = {
         "and", "or", "=", "/=", "+", "-", "*", "<",
         ">", "<=", ">=", "sll", "srl", "xor", "&",
          "nand", "nor", "xnor", "/", "mod", "**", "sra", NULL
      };

      of << " " << ops[op_] << " ";

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

void cpp_return_stmt::emit(std::ostream &of, int level) const
{
   assert(value_);
   of << "return ";
   value_->emit(of, level);
}

void cpp_assign_stmt::emit(std::ostream &of, int level) const
{
   if(is_instantiation)
   {
      lhs_->get_type()->emit(of, level);
      lhs_->emit(of, level);
      of << "(";
      rhs_->emit(of, level);
      of << ")";
   }
   else
   {
      lhs_->emit(of, level);
      of << " = ";
      rhs_->emit(of, level);
   }
}

void cpp_expr_list::emit(std::ostream &of, int level) const
{
   if(!children_.empty())
      emit_children<cpp_expr>(of, children_, indent(level), ",", false);
}

cppClass::cppClass(const string& name, cpp_inherited_class in)
      : name_(name), in_(in)
{
   cpp_function* constr = new cpp_function(name_.c_str(), new cpp_type(CPP_TYPE_NOTYPE));
   constr->set_constructor();
   constr->set_comment("Default constructor");
   add_function(constr);
   switch(in_)
   {
      case CPP_WARPED_EVENT:
         add_event_functions();
         break;
      case CPP_WARPED_SIM_OBJ:
         add_simulation_functions();
         break;
   }
}

void cppClass::add_event_functions()
{
   cpp_type* const_ref_string_type = new cpp_type(CPP_TYPE_STD_STRING);
   const_ref_string_type->set_const();
   const_ref_string_type->set_reference();
   cpp_type *returnType2 = new cpp_type(CPP_TYPE_UNSIGNED_INT);
   cpp_function *rec_name = new cpp_function("receiverName", const_ref_string_type);
   rec_name->set_comment("Inherited getter method");
   rec_name->set_const();
   rec_name->set_virtual();
   rec_name->set_override();
   cpp_function *timestamp = new cpp_function("timestamp", returnType2);
   timestamp->set_comment("Inherited getter method");
   timestamp->set_const();
   timestamp->set_virtual();
   timestamp->set_override();
   cpp_var *receiver_var = new cpp_var("receiver_name", new cpp_type(CPP_TYPE_STD_STRING));
   cpp_var *timestamp_var = new cpp_var("ts_", new cpp_type(CPP_TYPE_UNSIGNED_INT));
   add_var(receiver_var);
   add_var(timestamp_var);
   add_function(rec_name);
   add_function(timestamp);
   timestamp->add_stmt(new cpp_return_stmt(timestamp_var->get_ref()));
   rec_name->add_stmt(new cpp_return_stmt(receiver_var->get_ref()));
   // add init list to constructor.
   cpp_function* constr = get_costructor();
   cpp_var *name = new cpp_var("name", const_ref_string_type);
   constr->add_param(name);
   cpp_fcall_stmt* init_name = new cpp_fcall_stmt(receiver_var->get_ref(), "");
   init_name->add_param(name->get_ref());
   constr->add_init(init_name);
}

void cppClass::add_simulation_functions()
{
   cpp_type* temp = new cpp_type(CPP_TYPE_NOTYPE, new cpp_type(CPP_TYPE_BOOST_TRIBOOL));
   temp->add_type(new cpp_type(CPP_TYPE_STD_STRING));
   cpp_var *inputvar = new cpp_var("inputs_", new cpp_type(CPP_TYPE_STD_MAP, temp));
   inputvar->set_comment("std::map< signal_name, value >");
   add_var(inputvar);
   cpp_var *state_var = new cpp_var("state_", new cpp_type(CPP_TYPE_CUSTOM));
   state_var->set_comment("This will become the State variable");
   add_var(state_var);
   cpp_type *returnType = new cpp_type(CPP_TYPE_STD_VECTOR,
         new cpp_type(CPP_TYPE_SHARED_PTR,
         new cpp_type(CPP_TYPE_WARPED_EVENT)));
   cpp_function *init_fun = new cpp_function(WARPED_INIT_EVENT_FUN_NAME, returnType);
   init_fun->set_override();
   init_fun->set_virtual();
   // event_handler
   cpp_function *event_handler = new cpp_function(WARPED_HANDLE_EVENT_FUN_NAME, returnType);
   event_handler->set_override();
   event_handler->set_virtual();
   cpp_type *event_type = new cpp_type(CPP_TYPE_WARPED_EVENT);
   event_type->set_reference();
   event_type->set_const();
   cpp_var *event_param = new cpp_var("event", event_type);
   event_handler->add_param(event_param);
   // get_state
   cpp_type *get_state_ret_type = new cpp_type(CPP_TYPE_WARPED_OBJECT_STATE);
   get_state_ret_type->set_reference();
   cpp_function *get_state_fun = new cpp_function("getState", get_state_ret_type);
   get_state_fun->add_stmt(new cpp_return_stmt(state_var->get_ref()));
   get_state_fun->set_override();
   get_state_fun->set_virtual();
   // TODO: handle input.
   add_function(init_fun);
   add_function(get_state_fun);
   add_function(event_handler);
   // add init list to constructor.
   cpp_function* constr = get_costructor();
   cpp_var *name = new cpp_var("name", new cpp_type(CPP_TYPE_NOTYPE));
   constr->add_param(name);
   cpp_var_ref* sim_obj = new cpp_var_ref("", new cpp_type(CPP_TYPE_WARPED_SIMULATION_OBJECT));
   cpp_fcall_stmt* init_name = new cpp_fcall_stmt(sim_obj, "");
   init_name->add_param(name->get_ref());
   constr->add_init(init_name);
}

cpp_function* cppClass::get_function(const std::string &name) const
{
   cpp_decl* temp = scope_.get_decl(name);
   assert(temp);
   cpp_function* retvalue = dynamic_cast<cpp_function*>(temp);
   assert(retvalue);
   return retvalue;
}

void cpp_context::emit(std::ostream &of, int level) const
{
   newline(of, level);
   of << "int main(int argc, const char** argv) {";
   emit_children<cpp_stmt>(of, statements_, indent(level), ";");
   newline(of, level);
   of << "}; ";
}

void cppClass::emit(std::ostream &of, int level) const
{
   newline(of, level);
   emit_comment(of, level);
   of << "class " << name_;
   switch(in_)
   {
      case CPP_WARPED_SIM_OBJ:
         of << " : public warped::SimulationObject";
         break;
      case CPP_WARPED_EVENT:
         of << " : public warped::Event";
         break;
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
   case CPP_UNARYOP_LITERAL:
      operand_->emit(of, level);
      break;
   case CPP_UNARYOP_NEW:
      of << "new ";
      // FIXME
      operand_->get_type()->emit(of, level),
      operand_->emit(of, level);
      break;
   case CPP_UNARYOP_NEG:
       of << "-";
      break;
   }
   operand_->emit(of, level);

   close_parens(of);
}

void cpp_fcall_stmt::emit(std::ostream &of, int level) const
{
   base_->emit(of, level);
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
   of << name_ << " (";
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
   of << name_;
}
