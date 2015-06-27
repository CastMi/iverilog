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

void cpp_var_ref::emit(std::ostream &of, int level) const
{
   of << name_;
   if (slice_) {
      of << "("; 
      slice_->emit(of, level);
      of << ")";
   }
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
   assert(lhs_ && rhs_);
   of << lhs_ << " = ";
   this->rhs_->emit(of, level);
   of << " ";
   //this->after_->emit(of, level);
}

void cppClass::add_event_function()
{
   cpp_type *returnType = new cpp_type(CPP_TYPE_VECTOR,
         new cpp_type(CPP_TYPE_SHARED_PTR,
         new cpp_type(CPP_TYPE_WARPED_EVENT)));
   cpp_function *functionOne = new cpp_function("createInitialEvents", returnType);
   cpp_function *functionTwo = new cpp_function("receiveEvent", returnType);
   cpp_type *event_type = new cpp_type(CPP_TYPE_WARPED_EVENT);
   event_type->set_const();
   cpp_var *event_param = new cpp_var("event", event_type);
   functionTwo->add_param(event_param);
   add_function(functionOne);
   add_function(functionTwo);
}

cpp_function *cppClass::get_costructor()
{
   cpp_decl* temp = scope_.get_decl(name_);
   assert(temp);
   cpp_function* retvalue = dynamic_cast<cpp_function*>(temp);
   assert(retvalue);
   return retvalue;
}

void cppClass::emit(std::ostream &of, int level) const
{
   // TODO:
   // Handle includes
   of << "#include <warped.hpp>" << std::endl;
   of << "#include <vector>" << std::endl;         
   of << std::endl;

   emit_comment(of, level);
   // TODO:
   // Should every class inherit from simulation Object?
   of << "class " << name_ << " : public warped::SimulationObject {";
   newline(of, level);
   of << "public:";

   if (!scope_.empty()) {
      newline(of, indent(level));
      emit_children<cpp_decl>(of, scope_.get_decls(), indent(level), ";");
   }

   if(!statements_.empty())
   {
      newline(of, level);
      of << "instructions: ";
      newline(of, level);
      // FIXME: handle syntax
      emit_children<cpp_assign_stmt>(of, statements_, indent(level), ";");
   }
   
   newline(of, level);
   of << "}; ";
   newline(of, indent(level));
}

cpp_decl::~cpp_decl()
{
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
   // constructor doesn't have return type
   if(type_)
      type_->emit(of, level);
   of << name_ << " (";
   if(!scope_.get_decls().empty())
      emit_children<cpp_decl>(of, scope_.get_decls(), indent(level), ",", false);
   of << ")";
}

void cpp_param::emit(std::ostream &of, int level) const
{
   of << "const ";
   type_->emit(of, level);
   of << name_;
}

void cpp_var::emit(std::ostream &of, int level) const
{
   type_->emit(of, level); 
   of << name_;
}
