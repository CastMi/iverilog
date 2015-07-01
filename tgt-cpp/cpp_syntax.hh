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

#define WARPED_INIT_EVENT_FUN_NAME "createInitialEvents"
#define WARPED_HANDLE_EVENT_FUN_NAME "receiveEvent"

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
   cpp_var_ref(const string& name, const cpp_type *type)
      : cpp_expr(type), name_(name) {}
   ~cpp_var_ref() {};

   void emit(std::ostream &of, int level) const;
   const std::string &get_name() const { return name_; }
   void set_name(const std::string &name) { name_ = name; }

private:
   std::string name_;
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

/*
 * Used, for example, when you instantiate an object.
 * It should contain only const_expr and var_ref.
 */
class cpp_const_expr_list : public cpp_expr {
public:
   cpp_const_expr_list() : cpp_expr(new cpp_type(CPP_TYPE_NOTYPE)){}
   ~cpp_const_expr_list() {};

   void add_cpp_expr(cpp_expr* el) { children_.push_back(el); };
   void emit(std::ostream &of, int level) const;

private:
   std::list<cpp_expr*> children_;
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
   virtual ~cpp_decl() {};

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

class cpp_stmt {
public:
   cpp_stmt() {};
   virtual ~cpp_stmt() {};

   virtual void emit(std::ostream &of, int level = 0) const = 0;
};

/*
 * A function call statement
 */
class cpp_fcall_stmt : public cpp_stmt {
public:
   cpp_fcall_stmt(cpp_var_ref * base, const std::string& fun_name)
      : base_(base), fun_name_(fun_name) {}
   virtual ~cpp_fcall_stmt() {};

   virtual void emit(std::ostream &of, int level = 0) const;
   virtual void add_param(cpp_expr* el) { parameters_.push_back(el); };

protected:
   cpp_var_ref *base_;
   // The parameters could be constant or reference to variable
   std::list<cpp_expr*> parameters_;
   std::string fun_name_;
};

class cpp_return_stmt : public cpp_stmt {
public:
   cpp_return_stmt(cpp_var_ref *value)
      : value_(value) {}
   virtual ~cpp_return_stmt() {};

   virtual void emit(std::ostream &of, int level = 0) const;

protected:
   cpp_var_ref *value_;
};

class cpp_assign_stmt : public cpp_stmt {
public:
   cpp_assign_stmt(cpp_var_ref *lhs, cpp_expr *rhs,
         bool instantiation = false)
      : lhs_(lhs), rhs_(rhs), isinstantiation(instantiation) {}
   virtual ~cpp_assign_stmt() {};

   virtual void emit(std::ostream &of, int level = 0) const;

protected:
   cpp_expr *lhs_;
   cpp_expr *rhs_;
   bool isinstantiation;
};

/*
 * Roughly these map onto Verilog's processes, functions
 * and tasks.
 */
class cpp_procedural {
public:
   cpp_procedural() : isconst(false), isoverride(false), isvirtual(false) {}
   virtual ~cpp_procedural() {}

   virtual cpp_scope *get_scope() { return &scope_; }
   void add_stmt(cpp_stmt* item) { statements_.push_back(item); };
   void set_const() { isconst = true; }
   void set_override() { isoverride = true; }
   void set_virtual() { isvirtual = true; }

protected:
   cpp_scope scope_;
   std::list<cpp_stmt*> statements_;
   bool isconst, isoverride, isvirtual;
};

/*
 * This class contains the main function
 */
class cpp_context {
public:
   cpp_context() {}

   void emit(std::ostream &of, int level = 0) const;
   void add_stmt(cpp_stmt* el) { statements_.push_back(el); };

private:
   std::list<cpp_stmt*> statements_;
};

class cpp_function : public cpp_decl, public cpp_procedural {
public:
   cpp_function(const char *name, cpp_type *ret_type,
         bool constructor = false)
      : cpp_decl(name, ret_type), isconstructor(constructor)
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
   // Local vars
   cpp_scope variables_;
   bool isconstructor;
};

enum cpp_inherited_class {
   CPP_WARPED_EVENT,
   CPP_WARPED_SIM_OBJ
};

/*
 * Every module is class.
 */
class cppClass : public cpp_element {
public:
   cppClass(const string& name, cpp_inherited_class in = CPP_WARPED_SIM_OBJ);
   virtual ~cppClass() {};

   void emit(std::ostream &of, int level = 0) const;
   const std::string &get_name() const { return name_; }
   void add_var(cpp_var *item) { scope_.add_decl(item); }
   void add_function(cpp_function* fun) { scope_.add_decl(fun); };

   cpp_scope *get_scope() { return &scope_; }
   cpp_function *get_costructor() { return constructor_; };
   cpp_function *get_function(const std::string &name) const;

private:
   void add_simulation_functions();
   void add_event_functions();

   cpp_function* constructor_;
   // Class name
   std::string name_;
   cpp_scope scope_;
   cpp_inherited_class in_;
};

typedef std::list<cppClass*> entity_list_t;

#endif

