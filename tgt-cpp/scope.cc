/*
 *  C++ code generation for scopes.
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

#include "cpp_target.h"
#include "cpp_element.hh"
#include "state.hh"
#include "cpp_syntax.hh"
#include "hierarchy.hh"

#include <iostream>
#include <sstream>
#include <cassert>
#include <cstring>

/*
 * This represents the portion of a nexus that is visible within
 * a C++ scope. If that nexus portion does not contain a signal,
 * then `tmpname' gives the name of the temporary that will be
 * used when this nexus is used in `scope' (e.g. for LPMs that
 * appear in instantiations). The list `connect' lists all the
 * signals that should be joined together to re-create the net.
 */
struct scope_nexus_t {
   cpp_scope *scope;
   ivl_signal_t sig;            // A real signal
   unsigned pin;                // The pin this signal is connected to
   string tmpname;              // A new temporary signal
   list<ivl_signal_t> connect;  // Other signals to wire together
};

/*
 * This structure is stored in the private part of each nexus.
 * It stores a scope_nexus_t for each C++ scope which is
 * connected to that nexus. It's stored as a list so we can use
 * contained_within to allow several nested scopes to reference
 * the same signal.
 */
struct nexus_private_t {
   list<scope_nexus_t> signals;
   cpp_const_expr *const_driver;
};

/*
 * Translate all the primitive logic gates into concurrent
 * signal assignments.
 */
static void declare_logic(cppClass *arch, ivl_scope_t scope)
{
   debug_msg("Declaring logic in scope type %s", ivl_scope_tname(scope));

   int nlogs = ivl_scope_logs(scope);
   for (int i = 0; i < nlogs; i++)
      draw_logic(arch, ivl_scope_log(scope, i));
}

// Replace consecutive underscores with a single underscore
static void replace_consecutive_underscores(string& str)
{
   size_t pos = str.find("__");
   while (pos != string::npos) {
      str.replace(pos, 2, "_");
      pos = str.find("__");
   }
}

static bool is_cpp_reserved_word(const string& word)
{
   // FIXME: need to complete the list of C++ reserved words
   const char *vhdl_reserved[] = { NULL };

   for (const char **p = vhdl_reserved; *p != NULL; p++) {
      if (strcasecmp(*p, word.c_str()) == 0)
         return true;
   }

   return false;
}

// Return a valid C++ name for a Verilog module
static string valid_class_name(const string& module_name)
{
   string name(module_name);
   replace_consecutive_underscores(name);
   if (name[0] == '_')
      name = "module" + name;
   if (*name.rbegin() == '_')
      name += "module";

   if (is_cpp_reserved_word(name))
      name += "_module";

   ostringstream ss;
   int i = 1;
   ss << name;
   while (find_class(ss.str())) {
      // Keep adding an extra number until we get a unique name
      ss.str("");
      ss << name << i++;
   }

   return ss.str();
}

/*
 * Returns the scope_nexus_t of this nexus visible within scope.
 */
static scope_nexus_t *visible_nexus(nexus_private_t *priv, cpp_scope *scope)
{
   list<scope_nexus_t>::iterator it;
   for (it = priv->signals.begin(); it != priv->signals.end(); ++it) {
      if (scope->contained_within((*it).scope))
         return &*it;
   }
   return NULL;
}

/*
 * Remember that a signal in `scope' is part of this nexus. The
 * first signal passed to this function for a scope will be used
 * as the canonical representation of this nexus when we need to
 * convert it to a variable reference (e.g. in a LPM input/output).
 */
static void link_scope_to_nexus_signal(nexus_private_t *priv, cpp_scope *scope,
                                       ivl_signal_t sig, unsigned pin)
{
   scope_nexus_t *sn;
   if ((sn = visible_nexus(priv, scope))) {
      assert(sn->tmpname == "");

      // Remember to connect this signal up later
      // If one of the signals is a input, make sure the input is not being driven
      if (ivl_signal_port(sn->sig) == IVL_SIP_INPUT)
         sn->sig = sig;
      else
         sn->connect.push_back(sig);
   }
   else {
      scope_nexus_t new_sn = { scope, sig, pin, "", list<ivl_signal_t>() };
      priv->signals.push_back(new_sn);
    }
}

/*
 * Ensure that a nexus has been initialised. I.e. all the necessary
 * statements, declarations, etc. have been generated.
 */
static void seen_nexus(ivl_nexus_t nexus)
{
   if (ivl_nexus_get_private(nexus) == NULL)
      draw_nexus(nexus);
}

/*
 * Make a temporary the representative of this nexus in scope.
 */
static void link_scope_to_nexus_tmp(nexus_private_t *priv, cpp_scope *scope,
                                    const string &name)
{
   scope_nexus_t new_sn = { scope, NULL, 0, name, list<ivl_signal_t>() };
   priv->signals.push_back(new_sn);
}

/*
 * Generates C++ code to fully represent a nexus.
 */
void draw_nexus(ivl_nexus_t nexus)
{
   nexus_private_t *priv = new nexus_private_t;
   priv->const_driver = NULL;

   int nptrs = ivl_nexus_ptrs(nexus);

   // Number of drivers for this nexus
   int ndrivers = 0;

   // First pass through connect all the signals up
   for (int i = 0; i < nptrs; i++) {
      ivl_nexus_ptr_t nexus_ptr = ivl_nexus_ptr(nexus, i);

      ivl_signal_t sig;
      if ((sig = ivl_nexus_ptr_sig(nexus_ptr))) {
         // this is a signal
         cpp_scope *scope = find_scope_for_signal(sig);
         if (scope) {
            unsigned pin = ivl_nexus_ptr_pin(nexus_ptr);
            link_scope_to_nexus_signal(priv, scope, sig, pin);
         }
      }
   }

   // Second pass through make sure logic have signal
   // inputs and outputs
   for (int i = 0; i < nptrs; i++) {
      ivl_nexus_ptr_t nexus_ptr = ivl_nexus_ptr(nexus, i);

      ivl_net_logic_t log;
      ivl_net_const_t con;
      if ((log = ivl_nexus_ptr_log(nexus_ptr))) {
         // this is logic
         ivl_scope_t log_scope = ivl_logic_scope(log);
         if (!is_default_scope_instance(log_scope))
            continue;

         cppClass *theclass = find_class(log_scope);
         assert(theclass);

         cpp_scope *thescope = theclass->get_scope();
         if (visible_nexus(priv, thescope)) {
            // Already seen this signal in thescope
         }
         else {
            ostringstream ss;
            ss << "LO" << ivl_logic_basename(log);
            thescope->add_decl(new cpp_var(ss.str(),  new cpp_type(CPP_TYPE_INT)));

            link_scope_to_nexus_tmp(priv, thescope, ss.str());
         }

         // If this is connected to pin-0 then this nexus is driven
         // by this logic gate
         if (ivl_logic_pin(log, 0) == nexus)
            ndrivers++;
      }
      else if ((con = ivl_nexus_ptr_con(nexus_ptr))) {
         // this is a constant
         if (ivl_const_type(con) == IVL_VT_REAL) {
            error("No C++ translation for real constant (%g)",
                  ivl_const_real(con));
            continue;
         }
         priv->const_driver =
               new cpp_const_expr(ivl_const_bits(con), new cpp_type(CPP_TYPE_INT));

         // A constant is a sort of driver
         ndrivers++;
      }
   }

   // Save the private data in the nexus
   ivl_nexus_set_private(nexus, priv);
}


/*
 * Finds the name of the nexus signal within this scope.
 */
static string visible_nexus_signal_name(nexus_private_t *priv, cpp_scope *scope,
                                        unsigned *pin)
{
   scope_nexus_t *sn = visible_nexus(priv, scope);
   assert(sn);

   *pin = sn->pin;
   return sn->sig ? get_renamed_signal(sn->sig) : sn->tmpname;
}

/*
 * Translate a nexus to a variable reference. Given a nexus and a
 * scope, this function returns a reference to a signal that is
 * connected to the nexus and within the given scope. This signal
 * might not exist in the original Verilog source (even as a
 * compiler-generated temporary). If this nexus hasn't been
 * encountered before, the necessary code to connect up the nexus
 * will be generated.
 */
cpp_var_ref *nexus_to_var_ref(cpp_scope *scope, ivl_nexus_t nexus)
{
   seen_nexus(nexus);

   nexus_private_t *priv =
      static_cast<nexus_private_t*>(ivl_nexus_get_private(nexus));
   unsigned pin;
   string renamed(visible_nexus_signal_name(priv, scope, &pin));

   cpp_decl *decl = scope->get_decl(renamed);
   assert(decl);

   cpp_type *type = new cpp_type(*(decl->get_type()));
   cpp_var_ref *ref = new cpp_var_ref(renamed.c_str(), type);

   return ref;
}

cpp_var_ref* readable_ref(cpp_scope* scope, ivl_nexus_t nex)
{
   cpp_var_ref* ref = nexus_to_var_ref(scope, nex);

   return ref;
}

// Check if `name' differs from an existing name only in case and
// make it unique if it does.
static void avoid_name_collision(string& name, cpp_scope* scope)
{
   if (scope->name_collides(name)) {
      name += "_";
      ostringstream ss;
      int i = 1;
      do {
         // Keep adding an extra number until we get a unique name
         ss.str("");
         ss << name << i++;
      } while (scope->name_collides(ss.str()));

      name = ss.str();
   }
}

// Declare a single signal in a scope
static void declare_one_signal(cppClass *theclass, ivl_signal_t sig,
      ivl_scope_t scope)
{
   remember_signal(sig, theclass->get_scope());

   string name(ivl_signal_basename(sig));

   //avoid_name_collision(name, theclass->get_scope());

   rename_signal(sig, name);

   cpp_type* sig_type = new cpp_type(CPP_TYPE_INT);
   
   ivl_signal_port_t mode = ivl_signal_port(sig);
   switch (mode) {
   case IVL_SIP_NONE:
   case IVL_SIP_OUTPUT:
   case IVL_SIP_INPUT:
      {
         cpp_var *var = new cpp_var(name, sig_type);
         theclass->add_to_inputs(var);
      }
         break;
   case IVL_SIP_INOUT:
      {
         error("inout is not supported yet");
      }
         break;
   default:
      assert(false);
   }
}

// Declare all signals and ports for a scope.
// This is done in two phases: first the ports are added, then
// internal signals. Making two passes like this ensures ports get
// first pick of names when there is a collision.
static void declare_signals(cppClass *theclass, ivl_scope_t scope)
{
   debug_msg("Declaring signals in scope type %s", ivl_scope_tname(scope));

   int nsigs = ivl_scope_sigs(scope);
   for (int i = 0; i < nsigs; i++) {
      ivl_signal_t sig = ivl_scope_sig(scope, i);

      if (ivl_signal_port(sig) != IVL_SIP_NONE)
         declare_one_signal(theclass, sig, scope);
   }

   for (int i = 0; i < nsigs; i++) {
      ivl_signal_t sig = ivl_scope_sig(scope, i);

      if (ivl_signal_port(sig) == IVL_SIP_NONE)
         declare_one_signal(theclass, sig, scope);
   }
}

/*
 * Create an empty C++ class for a Verilog module.
 */
static void create_skeleton_class_for(ivl_scope_t scope)
{
   assert(ivl_scope_type(scope) == IVL_SCT_MODULE);

   // Every module will be "translated" in a class
   // The type name will become the entity name
   const string tname = ivl_scope_tname(scope);

   // Create a class for each module
   cppClass *theClass = new cppClass(tname);

   // Build a comment to print out  in order to remember where
   // this class come from
   ostringstream ss;
   ss << "Generated from Verilog module " << ivl_scope_tname(scope)
      << " (" << ivl_scope_def_file(scope) << ":"
      << ivl_scope_def_lineno(scope) << ")";

   unsigned nparams = ivl_scope_params(scope);
   for (unsigned i = 0; i < nparams; i++) {
      ivl_parameter_t param = ivl_scope_param(scope, i);
      ss << "\n  " << ivl_parameter_basename(param) << " = ";
      // FIXME: here there should be a switch based on
      // parameter type.
      // Right now, everything is an int
      cpp_procedural* constructor = theClass->get_costructor();
      constructor->add_param(new cpp_var(ivl_parameter_basename(param),
              new cpp_type(CPP_TYPE_INT)));
   }

   theClass->set_comment(ss.str());
   remember_class(theClass, scope);
}

/*
 * Map two signals together.
 */
static void map_signal(ivl_signal_t to, submodule* to_insert,
                     cppClass* parent, bool input = true)
{
   ivl_nexus_t nexus = ivl_signal_nex(to, 0);
   seen_nexus(nexus);

   cpp_scope *parent_scope = parent->get_scope();

   nexus_private_t *priv =
      static_cast<nexus_private_t*>(ivl_nexus_get_private(nexus));
   assert(priv);

   const string name(ivl_signal_basename(to));

   // We can only map ports to signals or constants
   if (visible_nexus(priv, parent_scope)) {
      cpp_var_ref* map_to = nexus_to_var_ref(parent_scope, nexus);
      if(input)
         to_insert->insert_input(name, map_to->get_name());
      else
         to_insert->insert_output(map_to->get_name(), name);
   }
   else if (priv->const_driver && ivl_signal_port(to) == IVL_SIP_INPUT) {
      cpp_const_expr* map_to = priv->const_driver;
      if(input)
         to_insert->insert_input(name, map_to->get_value());
      else
         to_insert->insert_output(map_to->get_value(), name);
      priv->const_driver = NULL;
   }
   else {
      // This nexus isn't attached to anything in the parent
      return;
   }
}

/*
 * Find all the port mappings of a module instantiation.
 */
static void port_map(ivl_scope_t scope, submodule* child,
                     cppClass* parent)
{
   // Find all the port mappings
   int nsigs = ivl_scope_sigs(scope);
   for (int i = 0; i < nsigs; i++) {
      ivl_signal_t sig = ivl_scope_sig(scope, i);

      ivl_signal_port_t mode = ivl_signal_port(sig);
      switch (mode) {
      case IVL_SIP_NONE:
         // Port map doesn't care about internal signals
         break;
      case IVL_SIP_INPUT:
         map_signal(sig, child, parent);
         break;
      case IVL_SIP_OUTPUT:
         map_signal(sig, child, parent, false);
         break;
      case IVL_SIP_INOUT:
      default:
         assert(false);
      }
   }
}

/*
 * A first pass through the hierarchy: create C++ class for
 * each unique Verilog module type.
 */
extern "C" int draw_skeleton_scope(ivl_scope_t scope, void *)
{
   // Have I already seen this scope?
   // If not, add to the list of seen scopes.
   if (seen_this_scope_type(scope))
      return 0;  // Already generated a skeleton for this scope type

   debug_msg("Initial visit to scope type %s", ivl_scope_tname(scope));

   switch (ivl_scope_type(scope)) {
   case IVL_SCT_MODULE:
      create_skeleton_class_for(scope);
      break;
   case IVL_SCT_FORK:
      error("No translation for fork statements yet");
      return 1;
   default:
      // The other scope types are expanded later on
      break;
   }

   int rc = ivl_scope_children(scope, draw_skeleton_scope, NULL);
   return rc;
}

extern "C" int draw_all_signals(ivl_scope_t scope, void *)
{
   // check if exists a skeleton of this scope
   if (!is_default_scope_instance(scope))
      return 0;  // Not interested in this instance

   if (ivl_scope_type(scope) == IVL_SCT_MODULE) {
      cppClass *theclass = find_class(scope);
      assert(theclass);

      declare_signals(theclass, scope);
   }
   else if (ivl_scope_type(scope) == IVL_SCT_GENERATE) {
      ivl_scope_t parent = ivl_scope_parent(scope);
      while (ivl_scope_type(parent) == IVL_SCT_GENERATE)
         parent = ivl_scope_parent(scope);

      cppClass* theclass = find_class(parent);
      assert(theclass);

      declare_signals(theclass, scope);
   }

  return ivl_scope_children(scope, draw_all_signals, scope);
}

extern "C" int draw_all_logic_and_lpm(ivl_scope_t scope, void *)
{
   // check if exists a skeleton of this scope
   if (!is_default_scope_instance(scope))
      return 0;  // Not interested in this instance

   if (ivl_scope_type(scope) == IVL_SCT_MODULE) {
      cppClass *theclass = find_class(scope);
      assert(theclass);

      set_active_class(theclass);
      {
         declare_logic(theclass, scope);
      }
      set_active_class(NULL);
   }

   return ivl_scope_children(scope, draw_all_logic_and_lpm, scope);
}

extern "C" int draw_hierarchy(ivl_scope_t scope, void *_parent)
{
   if (ivl_scope_type(scope) == IVL_SCT_MODULE && _parent) {
      ivl_scope_t parent = static_cast<ivl_scope_t>(_parent);

      // Skip over any containing generate scopes
      while (ivl_scope_type(parent) == IVL_SCT_GENERATE)
         parent = ivl_scope_parent(parent);

      if (!is_default_scope_instance(parent))
         return 0;  // Not generating code for the parent instance so
                    // don't generate for the child

      cppClass *theclass = find_class(scope);
      assert(theclass);

      cppClass *parent_class = find_class(parent);
      assert(parent_class);
      
      // And an instantiation statement
      string inst_name = ivl_scope_basename(scope);
      // FIXME: check for name correctness

      // Make sure the name doesn't collide with anything we've
      // already declared
      //avoid_name_collision(inst_name, parent_class->get_scope());

      submodule *temp = new submodule(theclass);
      add_submodule_to(temp, parent_class);
      port_map(scope, temp, parent_class);
   }

   return ivl_scope_children(scope, draw_hierarchy, scope);
}

int draw_scope(ivl_scope_t scope, void *_parent)
{
   int rc = draw_skeleton_scope(scope, _parent);
   if (rc != 0)
      return rc;

   rc = draw_all_signals(scope, _parent);
   if (rc != 0)
      return rc;

   rc = draw_all_logic_and_lpm(scope, _parent);
   if (rc != 0)
      return rc;

   rc = draw_hierarchy(scope, _parent);
   if (rc != 0)
      return rc;
   
   return 0;
}

