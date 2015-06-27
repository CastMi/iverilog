/*
 *  C++ abstract syntax elements.
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

#ifndef INC_CPP_SYNTAX_HH
#define INC_CPP_SYNTAX_HH

#include <inttypes.h>
#include "cpp_element.hh"
#include "cpp_type.hh"
#include <set>
#include <cassert>

using namespace std;

class cpp_scope;
class cppClass;

class cpp_expr : public cpp_element {
public:
    cpp_expr(const cpp_type* type)
      : type_(type) {}
   virtual ~cpp_expr() {};

   const cpp_type *get_type() const { return type_; }

protected:
   static void open_parens(ostream& of);
   static void close_parens(ostream& of);
   static int paren_levels;

   const cpp_type *type_;
};

enum cpp_binop_t {
   CPP_BINOP_AND,
   CPP_BINOP_OR,
   CPP_BINOP_ADD,
   CPP_BINOP_SUB,
   CPP_BINOP_MULT,
   CPP_BINOP_XOR,
   CPP_BINOP_NAND,
   CPP_BINOP_NOR,
   CPP_BINOP_XNOR,
   CPP_BINOP_DIV,
};

enum cpp_unaryop_t {
   CPP_UNARYOP_NOT,
   CPP_UNARYOP_NEG
};

/*
 * A scalar variable reference.
 */
class cpp_var_ref : public cpp_expr {
public:
   cpp_var_ref(const string& name, const cpp_type *type,
                cpp_expr *slice = NULL)
      : cpp_expr(type), name_(name), slice_(slice) {}
   ~cpp_var_ref() {};

   void emit(std::ostream &of, int level) const;
   const std::string &get_name() const { return name_; }
   void set_name(const std::string &name) { name_ = name; }

private:
   std::string name_;
   cpp_expr *slice_;
};

/*
 * A binary expression contains a list of operands rather
 * than just two.
 */
class cpp_binop_expr : public cpp_expr {
public:
   cpp_binop_expr(cpp_binop_t op, const cpp_type *type)
      : cpp_expr(type), op_(op) {}
   cpp_binop_expr(cpp_expr *left, cpp_binop_t op,
                   cpp_expr *right, const cpp_type *type)
      : cpp_expr(type), op_(op)
   {
      add_expr(left);
      add_expr(right);
   }
   ~cpp_binop_expr() {};

   void add_expr(cpp_expr *e) { operands_.push_back(e); };
   void add_expr_front(cpp_expr *e) { operands_.push_front(e); };
   void emit(std::ostream &of, int level) const;

private:
   std::list<cpp_expr*> operands_;
   cpp_binop_t op_;
};

class cpp_unaryop_expr : public cpp_expr {
public:
   cpp_unaryop_expr(cpp_unaryop_t op, cpp_expr *operand,
                      cpp_type *type)
      : cpp_expr(type), op_(op), operand_(operand) {}
   ~cpp_unaryop_expr() {};

   void emit(std::ostream &of, int level) const;
private:
   cpp_unaryop_t op_;
   cpp_expr *operand_;
};

class cpp_const_expr : public cpp_expr {
public:
   cpp_const_expr(const char *op, cpp_type *type)
      : cpp_expr(type), value_(op) {}
   ~cpp_const_expr() {};

   void emit(std::ostream &of, int level) const;
private:
   std::string value_;
};

/*
 * A declaration of some sort (variable, function, etc.).
 */
class cpp_decl : public cpp_element {
public:
   cpp_decl(const string& name, const cpp_type *type = NULL)
      : name_(name), type_(type) {}
   virtual ~cpp_decl();

   const std::string &get_name() const { return name_; }
   const cpp_type *get_type() const;
   void set_type(cpp_type *t) { type_ = t; }

   // The different sorts of assignment statement
   // ASSIGN_CONST is used to generate a variable to shadow a
   // constant that cannot be assigned to (e.g. a function parameter)
   enum assign_type_t { ASSIGN_BLOCK, ASSIGN_NONBLOCK, ASSIGN_CONST };

   // Get the sort of assignment statement to generate for
   // assignments to this declaration
   // For some sorts of declarations it doesn't make sense
   // to assign to it so calling assignment_type just raises
   // an assertion failure
   virtual assign_type_t assignment_type() const { assert(false);
                                                   return ASSIGN_BLOCK; }

   // True if this declaration can be read from
   virtual bool is_readable() const { return true; }

   // Modify this declaration so it can be read from
   // This does nothing for most declaration types
   virtual void ensure_readable() {}
protected:
   std::string name_;
   const cpp_type *type_;
   // TODO: flag to understand wheter it is private or not.
   // bool isprivate;
};

/*
 * A variable declaration.
 */
class cpp_var : public cpp_decl {
public:
   cpp_var(const string& name, const cpp_type* type)
      : cpp_decl(name, type) {}
   virtual void emit(std::ostream &of, int level) const;
   assign_type_t assignment_type() const { return ASSIGN_NONBLOCK; }
};

/*
 * A parameter of a module.
 * It becomes a parameter of the constructor of the class.
 */
class cpp_param : public cpp_decl {
public:
   cpp_param(const char *name, cpp_type *type, bool needSetter = false)
      : cpp_decl(name, type), needSetter_(needSetter) {}
   void emit(std::ostream &of, int level) const;
   assign_type_t assignment_type() const { return ASSIGN_CONST; }

private:
   bool needSetter_;
};

/*
 * Contains a list of declarations in a hierarchy.
 */
class cpp_scope {
public:
   cpp_scope() :
      parent_(NULL) {}
   ~cpp_scope() {}

   void add_decl(cpp_decl *decl);
   cpp_decl *get_decl(const std::string &name) const;
   bool have_declared(const std::string &name) const;
   bool name_collides(const string& name) const;
   bool contained_within(const cpp_scope *other) const;
   cpp_scope *get_parent() const;

   bool empty() const { return decls_.empty(); }
   const std::list<cpp_decl*> &get_decls() const { return decls_; }
   void set_parent(cpp_scope *p) { parent_ = p; }

private:
   // TODO:
   // distinguish between private/public?
   // Something like:
   // std::list<cpp_decl*> private_;
   // std::list<cpp_decl*> public_;
   std::list<cpp_decl*> decls_;
   cpp_scope *parent_;
};

/*
 * Roughly these map onto Verilog's processes, functions
 * and tasks.
 */
class cpp_procedural {
public:
   cpp_procedural() : contains_wait_stmt_(false) {}
   virtual ~cpp_procedural() {}

   virtual cpp_scope *get_scope() { return &scope_; }

   void added_wait_stmt() { contains_wait_stmt_ = true; }
   bool contains_wait_stmt() const { return contains_wait_stmt_; }

protected:
   cpp_scope scope_;

   // If this is true then the body contains a `wait' statement
   // embedded in it somewhere
   // If this is the case then we can't use a sensitivity list for
   // the process
   bool contains_wait_stmt_;

   // The set of variable we have performed a blocking
   // assignment to
   set<string> blocking_targets_;
};


class cpp_function : public cpp_decl, public cpp_procedural {
public:
   cpp_function(const char *name, cpp_type *ret_type)
      : cpp_decl(name, ret_type)
   {
      // A function contains two scopes:
      // scope_ = The parameters
      // variables_ = Local variables
      // A call to get_scope returns variables_ whose parent is scope_
      variables_.set_parent(&scope_);
   }

   virtual void emit(std::ostream &of, int level) const;
   cpp_scope *get_scope() { return &variables_; }
   void add_param(cpp_var *p) { scope_.add_decl(p); }

private:
   // Parameters
   cpp_scope scope_;
   // Local vars
   cpp_scope variables_;
};

class cpp_assign_stmt {
public:
   cpp_assign_stmt(cpp_var_ref *lhs, cpp_expr *rhs)
      : lhs_(lhs), rhs_(rhs), after_(NULL) {}
   virtual ~cpp_assign_stmt() {};

   void emit(std::ostream &of, int level = 0) const;
   void set_after(cpp_expr *after) { after_ = after; }
protected:
   cpp_var_ref *lhs_;
   cpp_expr *rhs_, *after_;
};

/*
 * Every module is class.
 */
class cppClass : public cpp_element {
public:
   cppClass(const string& name)
      : name_(name)
   {
      add_event_function();
   }
   
   virtual ~cppClass() {};

   void emit(std::ostream &of, int level = 0) const;
   const std::string &get_name() const { return name_; }
   void add_var(cpp_var *item) { scope_.add_decl(item); }
   void add_stmt(cpp_assign_stmt* ass) { statements_.push_back(ass); };
   void add_function(cpp_function* fun) { scope_.add_decl(fun); };

   cpp_scope *get_scope() { return &scope_; }
   cpp_function *get_costructor();

private:
   void add_event_function();

   // Functions
   std::list<cpp_assign_stmt*> statements_;
   // Class name
   std::string name_;
   cpp_scope scope_;
};

typedef std::list<cppClass*> entity_list_t;

#endif

