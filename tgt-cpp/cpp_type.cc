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
         return returnvalue + "warped::Event";
       case CPP_TYPE_CUSTOM:
         return std::string("myCustomType");
      case CPP_TYPE_INT:
          return std::string("int ");
      default:
          error("Unhandled type");
    }
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
   if(base_ != NULL)
   {
      switch (name_)
      {
         case CPP_TYPE_SHARED_PTR:
            of << " std::shared_ptr<";
            break;
         case CPP_TYPE_VECTOR:
            of << " std::vector<";
            break;
         default:
            error("Unhandled type");
      }
      base_->emit(of, num);
      of << "> ";
   } else
      of << get_decl_string() << " ";
}

cpp_type::~cpp_type()
{
}

