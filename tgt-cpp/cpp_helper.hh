/*
 *  Helper functions for C++ syntax elements.
 *
 *  Copyright (C) 2015 Michele Castellana (michele.castellana@mail.polimi.it)
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

#ifndef INC_CPP_HELPER_HH
#define INC_CPP_HELPER_HH

#include <fstream>
#include <list>
#include <cassert>

template <class T>
void emit_children(std::ostream &of,
                   const std::list<T*> &children,
                   int level, const char *delim = "",
                   bool final_delim = true)
{
   // Don't indent if there are no children
   if (children.empty())
      return;
   else {
      typename std::list<T*>::const_iterator it;
      int sz = children.size();
      for (it = children.begin(); it != children.end(); ++it) {
         (*it)->emit(of, indent(level));
         if(--sz > 0) {
            of << delim;
            newline(of, indent(level));
         }
      }
      if(final_delim)
      {
         of << delim;
         newline(of, level);
      }
   }
}

static inline char vl_to_vhdl_bit(char bit)
{
   switch (bit) {
   case '0':
   case 'Z':
   case '1':
      return bit;
   case 'z':
      return 'Z';
   case 'x':
   case 'X':
      return 'U';
   case '?':
      return '-';
   }
   assert(false);
   return 0;
}

#endif
