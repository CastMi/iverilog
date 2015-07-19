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

#ifndef INC_CPP_HIERARCHY_HH
#define INC_CPP_HIERARCHY_HH

#include "cpp_syntax.hh"
#include <list>
#include <utility>
#include <iostream>
#include <string>

struct submodule {
   submodule(cpp_class_type thetype) : type(thetype) {};
   submodule(const std::string& name) : type(CPP_CLASS_MODULE), name_(name) {};

   void insert_output(const std::string& el);
   void insert_input(const std::string& str1, const std::string& str2);

   // This is the class type to instantiate
   const cpp_class_type type;
   const std::string name_;
   // map<signal_name_in_supermodule, signal_name_in_submodule> >
   // The number of element of this map is the number of input of the logic port
   // For logic ports, the couple represent the same string
   std::list< std::pair< std::string, std::string > > signal_mapping;
   // The size of this list must be 1 for logic ports
   std::list<std::string> output_name;
};

void add_submodule_to(const std::string& name, submodule* item);
std::list<cpp_stmt*> build_hierarchy(std::list<cppClass*> class_list);

#endif  // #ifndef INC_CPP_HIERARCHY_HH
