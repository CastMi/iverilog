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

#ifndef INC_CPP_TYPE_HH
#define INC_CPP_TYPE_HH

#include "cpp_element.hh"
#include <cassert>

enum cpp_type_name_t {
   CPP_TYPE_BOOST_TRIBOOL,
   CPP_TYPE_CUSTOM_EVENT,
   CPP_TYPE_CUSTOM_BASE_CLASS,
   CPP_TYPE_ELEMENT_STATE,
   CPP_TYPE_INT,
   CPP_TYPE_NOTYPE,
   CPP_TYPE_STD_STRING,
   CPP_TYPE_UNSIGNED_INT,
   CPP_TYPE_VOID,
   CPP_TYPE_WARPED_EVENT,
   CPP_TYPE_WARPED_OBJECT_STATE,
   CPP_TYPE_WARPED_SIMULATION,
   CPP_TYPE_WARPED_SIMULATION_OBJECT,
   // Templates
   CPP_TYPE_CEREAL_BASE_CLASS,
   CPP_TYPE_SHARED_PTR,
   CPP_TYPE_STD_MAP,
   CPP_TYPE_STD_PAIR,
   CPP_TYPE_STD_VECTOR
};

class cpp_type : public cpp_element {
public:
   // Scalar constructors
   cpp_type(const cpp_type_name_t name, cpp_type *base = NULL)
      : name_(name), isconst(false), isreference(false),
      isiterator(false), ispointer(false)
   {
      if(base != NULL)
         base_.push_back(base);
   }
   virtual ~cpp_type() {};

   void emit(std::ostream &of, int level = 0) const;
   cpp_type_name_t get_name() const { return name_; }
   std::string get_string() const;
   std::string get_decl_string() const;
   std::string get_type_decl_string() const;
   void set_const() { isconst = true; }
   void set_reference() { isreference = true; }
   void set_pointer() { ispointer = true; }
   void set_iterator() { isiterator = true; }
   void add_type(cpp_type* el) { base_.push_front(el); }
   void add_type(const cpp_type_name_t el) { base_.push_front(new cpp_type(el)); }

   static std::string tostring(const cpp_type_name_t);

protected:
   const cpp_type_name_t name_;
   // the following field should be not empty if and only if the
   // "name_" field has a value of a template.
   std::list<cpp_type*> base_;
   bool isconst, isreference, isiterator, ispointer;
};

#endif
