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
   submodule(cpp_class_type thetype) : type(thetype), relate_class(NULL), hierarchy() {};
   submodule(const cppClass* theclass) : type(CPP_CLASS_MODULE), relate_class(theclass) {};

   void insert_output(const std::string& str1, const std::string& str2);
   void insert_input(const std::string& str1, const std::string& str2);
   submodule* find(const cppClass* elem);
   void add_submodule(submodule* item) { hierarchy.push_front(item); };
   void merge(submodule* item);

   // This is the class type to instantiate
   const cpp_class_type type;
   const std::string name;
   const cppClass* relate_class;
   std::list<submodule*> hierarchy;
   /*
    * list<pair< signal_name_supermodule, signal_name_here> >
    * For logic gates, the couple represents the same string.
    */
   std::list< std::pair< std::string, std::string > > signal_mapping;
   /*
    * list<pair< signal_name_supermodule, signal_name_here> >
    * For logic gates, the couple represents the same string
    * The size of this list must be 1 for logic gates.
    */
   std::list< std::pair< std::string, std::string > > outputs_map;
};

void remember_hierarchy(cppClass* theclass);
submodule* add_submodule_to(submodule* item, cppClass* parent);
std::list<cpp_stmt*> build_hierarchy();

#endif  // #ifndef INC_CPP_HIERARCHY_HH
