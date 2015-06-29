/*
 *  C++ variable types.
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

#ifndef INC_VHDL_TYPE_HH
#define INC_VHDL_TYPE_HH

#include "cpp_element.hh"
#include <cassert>

enum cpp_type_name_t {
   CPP_TYPE_INT,
   CPP_TYPE_STD_STRING,
   CPP_TYPE_WARPED_EVENT,
   CPP_TYPE_CUSTOM,
   CPP_TYPE_NOTYPE,
   // templates
   CPP_TYPE_STD_PAIR,
   CPP_TYPE_SHARED_PTR,
   CPP_TYPE_STD_VECTOR
};

class cpp_type : public cpp_element {
public:
   // Scalar constructors
   cpp_type(cpp_type_name_t name, cpp_type *base = NULL, bool constant = false)
      : name_(name), isconst(constant)
   {
      if(base != NULL)
         base_.push_back(base);
   }
   virtual ~cpp_type();

   void emit(std::ostream &of, int level) const;
   cpp_type_name_t get_name() const { return name_; }
   std::string get_string() const;
   std::string get_decl_string() const;
   std::string get_type_decl_string() const;
   void set_const() { isconst = true; }
   bool get_const() const { return isconst; }
   void add_type(cpp_type* el) { base_.push_front(el); }

protected:
   cpp_type_name_t name_;
   // the following field should be not empty if and only if the
   // "name_" field has a value of a template.
   std::list<cpp_type*> base_;
   bool isconst;
};

#endif
