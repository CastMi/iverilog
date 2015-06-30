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
   of << "\"" << value_ << "\"";
}

void cpp_var_ref::emit(std::ostream &of, int) const
{
   of << name_;
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
   assert(lhs_ && rhs_);
   lhs_->emit(of, level);
   of << " = ";
   rhs_->emit(of, level);
}

cppClass::cppClass(const string& name, cpp_inherited_class in)
      : name_(name), in_(in)
{
   cpp_function* temp = new cpp_function(name_.c_str(), new cpp_type(CPP_TYPE_NOTYPE));
   temp->set_comment("Default constructor");
   add_function(temp);
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
   cpp_type* returnType1 = new cpp_type(CPP_TYPE_STD_STRING);
   returnType1->set_const();
   returnType1->set_reference();
   cpp_type *returnType2 = new cpp_type(CPP_TYPE_UNSIGNED_INT);
   cpp_function *rec_name = new cpp_function("receiverName", returnType1);
   rec_name->set_comment("Inherited getter method");
   rec_name->set_const();
   cpp_function *timestamp = new cpp_function("timestamp", returnType2);
   timestamp->set_comment("Inherited getter method");
   timestamp->set_const();
   cpp_var *receiver_var = new cpp_var("receiver_name", new cpp_type(CPP_TYPE_STD_STRING));
   cpp_var *timestamp_var = new cpp_var("ts_", new cpp_type(CPP_TYPE_UNSIGNED_INT));
   add_var(receiver_var);
   add_var(timestamp_var);
   add_function(rec_name);
   add_function(timestamp);
   timestamp->add_stmt(new cpp_return_stmt(new cpp_var_ref(
               timestamp_var->get_name(), timestamp_var->get_type())));
   rec_name->add_stmt(new cpp_return_stmt(new cpp_var_ref(
               receiver_var->get_name(), receiver_var->get_type())));
}

void cppClass::add_simulation_functions()
{
   cpp_type* temp = new cpp_type(CPP_TYPE_NOTYPE, new cpp_type(CPP_TYPE_BOOST_TRIBOOL));
   temp->add_type(new cpp_type(CPP_TYPE_STD_STRING));
   cpp_var *inputvar = new cpp_var("inputs_", new cpp_type(CPP_TYPE_STD_MAP, temp));
   inputvar->set_comment("std::map< signal_name, value >");
   add_var(inputvar);
   cpp_type *returnType = new cpp_type(CPP_TYPE_STD_VECTOR,
         new cpp_type(CPP_TYPE_SHARED_PTR,
         new cpp_type(CPP_TYPE_WARPED_EVENT)));
   cpp_function *init_fun = new cpp_function(WARPED_INIT_EVENT_FUN_NAME, returnType);
   // event_handler
   cpp_function *event_handler = new cpp_function(WARPED_HANDLE_EVENT_FUN_NAME, returnType);
   cpp_type *event_type = new cpp_type(CPP_TYPE_WARPED_EVENT);
   event_type->set_reference();
   event_type->set_const();
   cpp_var *event_param = new cpp_var("event", event_type);
   event_handler->add_param(event_param);
   // get_state
   cpp_type *get_state_ret_type = new cpp_type(CPP_TYPE_WARPED_OBJECT_STATE);
   get_state_ret_type->set_reference();
   cpp_function *get_state_fun = new cpp_function("get_state", get_state_ret_type);
   // TODO: handle input.
   add_function(init_fun);
   add_function(get_state_fun);
   add_function(event_handler);
}

cpp_function *cppClass::get_function(const std::string &name) const
{
   cpp_decl* temp = scope_.get_decl(name);
   assert(temp);
   cpp_function* retvalue = dynamic_cast<cpp_function*>(temp);
   assert(retvalue);
   return retvalue;
}

cpp_function *cppClass::get_costructor()
{
   return get_function(name_);
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

// True if char is not '1' or '0'
static bool is_meta_bit(char c)
{
   return c != '1' && c != '0';
}

void cpp_unaryop_expr::emit(std::ostream &of, int level) const
{
   open_parens(of);

   switch (op_) {
   case CPP_UNARYOP_NOT:
       of << "not ";
      break;
   case CPP_UNARYOP_NEG:
       of << "-";
      break;
   }
   operand_->emit(of, level);

   close_parens(of);
}

void cpp_function::emit(std::ostream &of, int level) const
{
   emit_comment(of, level);
   type_->emit(of, level);
   of << name_ << " (";
   if(!scope_.get_decls().empty())
      emit_children<cpp_decl>(of, scope_.get_decls(), indent(level), ",", false);
   of << ")";
   if(isconst)
      of << " const";
   of << " {";
   if(!statements_.empty())
      emit_children<cpp_stmt>(of, statements_, indent(level), ";");
   of << "}";
}

void cpp_param::emit(std::ostream &of, int level) const
{
   of << "const ";
   type_->emit(of, level);
   of << name_;
}

void cpp_var::emit(std::ostream &of, int level) const
{
   emit_comment(of, level);
   type_->emit(of, level);
   of << name_;
}
