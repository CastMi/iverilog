/*
 *  Managing global state for the C++ code generator.
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

#include "state.hh"
#include "cpp_syntax.hh"
#include "cpp_target.h"
#include "hierarchy.hh"

#include <algorithm>
#include <string>
#include <map>
#include <vector>
#include <cstring>
#include <iostream>
#include <iterator>

using namespace std;

/*
 * Maps a signal to the scope it is defined within. Also
 * provides a mechanism for renaming signals -- i.e. when
 * an output has the same name as register: valid in Verilog
 * but not in VHDL, so two separate signals need to be
 * defined.
 */
struct signal_defn_t {
   std::string renamed;     // The name of the VHDL signal
   cpp_scope *scope;       // The scope where it is defined
};

// All classes to emit.
// These are stored in a list rather than a set so the first
// class added will correspond to the first (top) Verilog module
// encountered and hence it will appear first in the output file.
static std::list<cppClass*> g_classes;

// Store the mapping of ivl scope names to class names
static map<ivl_scope_t, string> g_scope_names;

typedef std::map<ivl_signal_t, signal_defn_t> signal_defn_map_t;
static signal_defn_map_t g_known_signals;

static cppClass *g_active_class = NULL;

// Set of scopes that are treated as the default examples of
// that type. Any other scopes of the same type are ignored.
typedef vector<ivl_scope_t> default_scopes_t;
static default_scopes_t g_default_scopes;

// There is one and only one context
static cpp_context *context = new cpp_context();

// This variable contain the set of logic gate
// that the design needs in order to run
static std::set<cpp_class_type> design_logic;

void remember_logic(cpp_class_type type)
{
   design_logic.insert(type);
}

// True if signal `sig' has already been encountered by the code
// generator. This means we have already assigned it to a C++
// object and possibly renamed it.
bool seen_signal_before(ivl_signal_t sig)
{
   return g_known_signals.find(sig) != g_known_signals.end();
}

// Remember the association of signal to a C++ object
void remember_signal(ivl_signal_t sig, cpp_scope *scope)
{
   assert(!seen_signal_before(sig));

   signal_defn_t defn = { ivl_signal_basename(sig), scope };
   g_known_signals[sig] = defn;
}

// Change the C++ name of a Verilog signal.
void rename_signal(ivl_signal_t sig, const std::string &renamed)
{
   assert(seen_signal_before(sig));

   g_known_signals[sig].renamed = renamed;
}

// Given a Verilog signal, return the C++ object where it should
// be defined. Note that this can return a NULL pointer if `sig' hasn't
// be encountered yet.
cpp_scope *find_scope_for_signal(ivl_signal_t sig)
{
   if (seen_signal_before(sig))
      return g_known_signals[sig].scope;
   else
      return NULL;
}

// Get the name of the C++ var corresponding to Verilog signal `sig'.
const std::string &get_renamed_signal(ivl_signal_t sig)
{
   assert(seen_signal_before(sig));

   return g_known_signals[sig].renamed;
}

static void only_remember_class(cppClass* theclass, bool a = true)
{
   /*
    * The first 2 classes added are the event class and
    * the base class. They NEED to stay on top on the others.
    */
   if(g_classes.size() <= 1)
   {
      g_classes.push_front(theclass);
      return;
   }
   std::list<cppClass*>::iterator it = g_classes.begin();
   std::advance(it,2);
   g_classes.insert(it, theclass);
   if(a)
      remember_hierarchy(theclass);
}

/*
 * Create the base functions that every design needs to have
 */
void build_basic_classes()
{
   // Create the base class
   cppClass *thebaseclass = new cppClass(BASE_CLASS_NAME, CPP_INHERIT_SIM_OBJ);
   thebaseclass->set_comment("Created to implements basic functions");
   only_remember_class(thebaseclass, false);
   // Create the event class
   cppClass *theeventclass = new cppClass(CUSTOM_EVENT_CLASS_NAME, CPP_INHERIT_EVENT);
   theeventclass->set_comment("Created to store information about the triggered event");
   only_remember_class(theeventclass, false);
   context->add_include("boost/logic/tribool.hpp");
   context->add_include("boost/logic/tribool_io.hpp");
   context->add_include("cassert");
   context->add_include("map");
   context->add_include("vector");
   context->add_include("warped.hpp");
}

void build_net()
{
   // Create all the logic gate classes
   for(std::set<cpp_class_type>::iterator it = design_logic.begin();
         it != design_logic.end(); it++)
   {
      only_remember_class(new cppClass(*it), false);
   }
   context->add_stmt(build_hierarchy());
}

// TODO: Can we dispose of this???
// -> This is only used in logic.cc to get the type of a signal connected
//    to a logic device -> we should be able to get this from the nexus
ivl_signal_t find_signal_named(const std::string &name, const cpp_scope *scope)
{
   signal_defn_map_t::const_iterator it;
   for (it = g_known_signals.begin(); it != g_known_signals.end(); ++it) {
      if (((*it).second.scope == scope
           || (*it).second.scope == scope->get_parent())
          && (*it).second.renamed == name)
         return (*it).first;
   }
   assert(false);
   return NULL;
}

// Compare the name of a class against a string
struct cmp_ent_name {
   cmp_ent_name(const string& n) : name_(n) {}

   bool operator()(const cppClass* ent) const
   {
      return ent->get_name() == name_;
   }

   const string& name_;
};

// Find a class given its name.
cppClass* find_class(const string& name)
{
   std::list<cppClass*>::const_iterator it
      = find_if(g_classes.begin(), g_classes.end(),
                cmp_ent_name(name));

   if (it != g_classes.end())
      return *it;
   else
      return NULL;
}

// Find a C++ class given a Verilog module scope. The C++ class
// name should be the same as the Verilog module type name.
// Note that this will return NULL if no class has been recorded
// for this scope type.
cppClass* find_class(ivl_scope_t scope)
{
   // Skip over generate scopes
   while (ivl_scope_type(scope) == IVL_SCT_GENERATE)
      scope = ivl_scope_parent(scope);

   assert(ivl_scope_type(scope) == IVL_SCT_MODULE);

   if (is_default_scope_instance(scope)) {
      map<ivl_scope_t, string>::iterator it = g_scope_names.find(scope);
      if (it != g_scope_names.end())
         return find_class((*it).second);
      else
         return NULL;
   }
   else {
      const char *tname = ivl_scope_tname(scope);

      for (map<ivl_scope_t, string>::iterator it = g_scope_names.begin();
           it != g_scope_names.end(); ++it) {
         if (strcmp(tname, ivl_scope_tname((*it).first)) == 0)
            return find_class((*it).second);
      }

      return NULL;
   }
}

// Add the class to the list of classes to emit.
void remember_class(cppClass* theclass, ivl_scope_t scope)
{
   g_classes.push_back(theclass);
   g_scope_names[scope] = theclass->get_name();
   remember_hierarchy(theclass);
}

// Print all C++ classes, in order, to the specified output stream.
void emit_everything(std::ostream& os)
{
   context->emit_before_classes(os);
   for (entity_list_t::iterator it = g_classes.begin();
        it != g_classes.end();
        ++it) {
      // TODO: divide in "emit_declaration" and "emit_implementation"
      // Something like:
      // emit_declaration(outfile);
      // emit_implementation(outfile);
      (*it)->emit(os);
   }
   context->emit_after_classes(os);
}

// Release all memory for the C++ objects.
void free_all_cpp_objects()
{
   int freed = cpp_element::free_all_objects();
   debug_msg("Deallocated %d C++ syntax objects", freed);

   size_t total = cpp_element::total_allocated();
   debug_msg("%d total bytes used for C++ syntax objects", total);

   g_classes.clear();
}

// Return the currently active entity
cppClass *get_active_class()
{
   return g_active_class;
}

// Change the currently active entity
void set_active_class(cppClass *ent)
{
   g_active_class = ent;
}

/*
 * True if two scopes have the same type name.
 */
static bool same_scope_type_name(ivl_scope_t a, ivl_scope_t b)
{
   if (strcmp(ivl_scope_tname(a), ivl_scope_tname(b)) != 0)
      return false;

   unsigned nparams_a = ivl_scope_params(a);
   unsigned nparams_b = ivl_scope_params(b);

   if (nparams_a != nparams_b)
      return false;

   for (unsigned i = 0; i < nparams_a; i++) {
      ivl_parameter_t param_a = ivl_scope_param(a, i);
      ivl_parameter_t param_b = ivl_scope_param(b, i);

      if (strcmp(ivl_parameter_basename(param_a),
                 ivl_parameter_basename(param_b)) != 0)
         return false;

      ivl_expr_t value_a = ivl_parameter_expr(param_a);
      ivl_expr_t value_b = ivl_parameter_expr(param_b);

      if (ivl_expr_type(value_a) != ivl_expr_type(value_b))
         return false;

      switch (ivl_expr_type(value_a)) {
         case IVL_EX_STRING:
            if (strcmp(ivl_expr_string(value_a), ivl_expr_string(value_b)) != 0)
               return false;
            break;

         case IVL_EX_NUMBER:
            if (ivl_expr_uvalue(value_a) != ivl_expr_uvalue(value_b))
               return false;
            break;

      default:
         assert(false);
      }
   }

   return true;
}

/*
 * True if we have already seen a scope with this type before.
 * If the result is `false' then s is stored in the set of seen
 * scopes.
 */
bool seen_this_scope_type(ivl_scope_t s)
{
   if (find_if(g_default_scopes.begin(), g_default_scopes.end(),
               bind1st(ptr_fun(same_scope_type_name), s))
       == g_default_scopes.end()) {
      g_default_scopes.push_back(s);
      return false;
   }
   else
      return true;
}

/*
 * True if this scope is the default example of this scope type.
 * All other instances of this scope type are ignored.
 */
bool is_default_scope_instance(ivl_scope_t s)
{
   return find(g_default_scopes.begin(), g_default_scopes.end(), s)
      != g_default_scopes.end();
}
