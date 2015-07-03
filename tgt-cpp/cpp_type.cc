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

#include "cpp_type.hh"
#include "cpp_target.h"

#include <cassert>
#include <sstream>
#include <iostream>

std::string cpp_type::tostring(cpp_type_name_t type)
{
   switch(type)
   {
      case CPP_TYPE_WARPED_EVENT:
         return std::string("warped::Event");
       case CPP_TYPE_WARPED_OBJECT_STATE:
         return std::string("warped::ObjectState");
       case CPP_TYPE_WARPED_SIMULATION_OBJECT:
         return std::string("warped::SimulationObject");
       case CPP_TYPE_WARPED_SIMULATION:
         return std::string("warped::Simulation");
       case CPP_TYPE_CUSTOM:
         return std::string("myCustomType");
       case CPP_TYPE_ELEMENT_STATE:
         return std::string("ElementState");
      case CPP_TYPE_INT:
         return std::string("int");
      case CPP_TYPE_UNSIGNED_INT:
         return std::string("unsigned int");
      case CPP_TYPE_BOOST_TRIBOOL:
         return std::string("boost::tribool");
      case CPP_TYPE_STD_STRING:
         return std::string("std::string");
      default:
          error("Unhandled type");
          return std::string("");
   }
}

/*
 * This is just the name of the type, without any parameters.
 */
std::string cpp_type::get_string() const
{
   std::string returnvalue;
   if(isconst)
      returnvalue += "const ";
   switch (name_)
   {
       case CPP_TYPE_WARPED_EVENT:
         returnvalue += "warped::Event";
         break;
       case CPP_TYPE_WARPED_OBJECT_STATE:
         returnvalue += "warped::ObjectState";
         break;
       case CPP_TYPE_WARPED_SIMULATION:
         returnvalue += "warped::Simulation";
         break;
       case CPP_TYPE_WARPED_SIMULATION_OBJECT:
         returnvalue += "warped::SimulationObject";
         break;
       case CPP_TYPE_ELEMENT_STATE:
         returnvalue += "ElementState";
         break;
       case CPP_TYPE_CUSTOM:
         returnvalue += std::string("myCustomType");
         break;
      case CPP_TYPE_INT:
         returnvalue += std::string("int");
         break;
      case CPP_TYPE_UNSIGNED_INT:
         returnvalue += std::string("unsigned int");
         break;
      case CPP_TYPE_BOOST_TRIBOOL:
         returnvalue += std::string("boost::tribool");
         break;
      case CPP_TYPE_STD_STRING:
         returnvalue += std::string("std::string");
         break;
      default:
          error("Unhandled type");
   }
   if(isreference)
      returnvalue += "&";
   return returnvalue;
}

/*
 * The is the qualified name of the type.
 */
std::string cpp_type::get_decl_string() const
{
   return get_string();
}

/*
 * Like get_decl_string but completely expands array declarations.
 */
std::string cpp_type::get_type_decl_string() const
{
   return get_decl_string();
}

void cpp_type::emit(std::ostream &of, int num) const
{
   if(!base_.empty())
   {
      switch (name_)
      {
         // handle only templates
         case CPP_TYPE_SHARED_PTR:
            of << "std::shared_ptr<";
            break;
         case CPP_TYPE_STD_MAP:
            of << "std::map<";
            break;
         case CPP_TYPE_STD_VECTOR:
            of << "std::vector<";
            break;
         case CPP_TYPE_STD_PAIR:
            of << "std::pair<";
            break;
         case CPP_TYPE_NOTYPE:
            // this is a special value to handle the hierarchy
            break;
         default:
            error("Unhandled type");
      }
      int sz = base_.size();
      for(std::list<cpp_type*>::const_iterator it = base_.begin(); it != base_.end(); ++it) {
         (*it)->emit(of, indent(num));
         if(--sz > 0)
            of << ", ";
      }
      if(name_ != CPP_TYPE_NOTYPE)
         of << "> ";
   } else if(name_ != CPP_TYPE_NOTYPE)
      of << get_decl_string() << " ";
}

