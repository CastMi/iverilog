/*
 *  C++ abstract syntax elements.
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

#ifndef INC_CPP_ELEMENT_HH
#define INC_CPP_ELEMENT_HH

#include <new>
#include <vector>
#include <iosfwd>
#include <list>
#include <string>

typedef std::list<std::string> string_list_t;

// Any C++ syntax element. Each element can also contain a comment.
//
// Memory management is handled specially for cpp_element subclasses:
// The vast majority of cpp_elements will be created during code generation
// and persist until after they have been printed, at which point *all*
// cpp_element objects should be destroyed. To support this all allocations
// of cpp_element subclasses call a special operator new which records
// the pointer allocated so we can ensure that it is disposed of when
// the code generator completes -- by free_all_objects.
//
// The two big advantages of this are that we don't have to worry about
// memory leaks of cpp_element objects, and we can freely share pointers
// between different parts of the AST.
class cpp_element {
public:
   virtual ~cpp_element() {}

   void* operator new(size_t size) throw (std::bad_alloc);
   void operator delete(void* ptr);

   virtual void emit(std::ostream &of, int level=0) const = 0;
   void print() const;

   void set_comment(std::string comment);

   static int free_all_objects();
   static size_t total_allocated();

protected:
   void emit_comment(std::ostream &of, int level,
                     bool end_of_line=false) const;
private:
   std::string comment_;

   static std::vector<cpp_element*> allocated_;
   static size_t total_alloc_;
};

typedef std::list<cpp_element*> element_list_t;

int deindent(int level);
int indent(int level);
void newline(std::ostream &of, int level);
std::string nl_string(int level);
void blank_line(std::ostream &of, int level);

#endif

