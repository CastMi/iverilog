/*
 *  C++ abstract syntax elements.
 *
 *  Copyright (C) 2008-2013  Michele Castellana (michele.castellana@mail.polimi.it)
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

#include "cpp_syntax.hh"
#include "cpp_helper.hh"
#include "cpp_target.h"
#include "state.hh"

#include <cassert>
#include <cstring>
#include <iostream>
#include <sstream>
#include <typeinfo>
#include <algorithm>
#include <iomanip>

using namespace std;

// fun names inside classes
#define WARPED_HANDLE_EVENT_FUN_NAME "receiveEvent"
#define WARPED_INIT_EVENT_FUN_NAME "createInitialEvents"
#define WARPED_TIMESTAMP_FUN_NAME "timestamp"
#define SIGNAL_NAME_GETTER_FUN_NAME "signalName"
#define NEW_VALUE_GETTER_FUN_NAME "newValue"
// var names inside classes
#define INPUT_VAR_NAME "signals_"
#define HIERARCHY_VAR_NAME "hierarchy_"
// var names inside function
#define RETURN_EVENT_LIST_VAR_NAME "response_event"
#define CASTED_EVENT_VAR_NAME "my_event"

void cpp_scope::add_decl(cpp_decl *decl)
{
   to_print_.push_back(decl);
}

void cpp_scope::add_visible(cpp_decl *decl)
{
   others_.push_back(decl);
}

cpp_decl *cpp_scope::get_decl(const std::string &name) const
{
   assert(!name.empty());
   std::list<cpp_decl*>::const_iterator it;
   for (it = to_print_.begin(); it != to_print_.end(); ++it) {
      if (strcasecmp((*it)->get_name().c_str(), name.c_str()) == 0)
         return *it;
   }
   for (it = others_.begin(); it != others_.end(); ++it) {
      if (strcasecmp((*it)->get_name().c_str(), name.c_str()) == 0)
         return *it;
   }
   return parent_ ? parent_->get_decl(name) : NULL;
}

bool cpp_scope::have_declared(const std::string &name) const
{
   return get_decl(name) != NULL;
}

// True if `name' differs in all but case from another declaration
bool cpp_scope::name_collides(const string& name) const
{
   const cpp_decl* decl = get_decl(name);
   if (decl)
      return strcasecmp(decl->get_name().c_str(), name.c_str()) == 0;
   else
      return false;
}

bool cpp_scope::contained_within(const cpp_scope *other) const
{
   if (this == other)
      return true;
   else if (NULL == parent_)
      return false;
   else
      return parent_->contained_within(other);
}

cpp_scope *cpp_scope::get_parent() const
{
   assert(parent_);
   return parent_;
}

int cpp_expr::paren_levels(0);

void cpp_binop_expr::emit(std::ostream &of, int level) const
{
   if(operands_.empty())
      return;
   open_parens(of);
   std::list<cpp_expr*>::const_iterator it = operands_.begin();

   (*it)->emit(of, level);
   for(it++; it != operands_.end(); it++)
   {
      switch(op_)
      {
         case CPP_BINOP_EQ:
            of << " == ";
            break;
         case CPP_BINOP_AND:
            of << " && ";
            break;
         case CPP_BINOP_OR:
            of << " or ";
            break;
         case CPP_BINOP_NEQ:
            of << " != ";
            break;
         case CPP_BINOP_ADD:
            of << " + ";
            break;
         case CPP_BINOP_SUB:
            of << " - ";
            break;
         case CPP_BINOP_SQUARE_BRACKETS:
            of << "[";
            break;
         default:
            error("This binary operation is not supported");
      }
      (*it)->emit(of, level);
      if(op_ == CPP_BINOP_SQUARE_BRACKETS)
         of << "]";
   }
   close_parens(of);
}

void cpp_const_expr::emit(std::ostream &of, int) const
{
   switch(get_type()->get_name())
   {
      case CPP_TYPE_INT:
      case CPP_TYPE_NOTYPE:
      case CPP_TYPE_UNSIGNED_INT:
         of << value_;
         break;
      case CPP_TYPE_STD_STRING:
         of << "\"" << value_ << "\"";
         break;
      default:
         error("This constant type is not supported");
   }
}

void cpp_break::emit(std::ostream &of, int) const
{
   of << "break";
}

void cpp_var_ref::emit(std::ostream &of, int level) const
{
   if(!name_.empty())
      of << name_;
   else
      type_->emit(of, level);
}

void cpp_expr::open_parens(std::ostream& of)
{
   if (paren_levels++ > 0)
      of << "(";
}

void cpp_expr::close_parens(std::ostream& of)
{
   assert(paren_levels > 0);

   if (--paren_levels > 0)
      of << ")";
}

void cpp_assign_stmt::emit(std::ostream &of, int level) const
{
   if(is_instantiation)
   {
      lhs_->emit(of, level);
      of << "(";
      rhs_->emit(of, level);
      of << ")";
   }
   else
   {
      lhs_->emit(of, level);
      of << " = ";
      rhs_->emit(of, level);
   }
}

void cpp_expr_list::emit(std::ostream &of, int level) const
{
   emit_children<cpp_expr>(of, children_, indent(level), ",", false);
}

   cppClass::cppClass(const string& name, const cpp_inherit_class in)
: name_(name), scope_(), inherit_(in) , type_(CPP_CLASS_MODULE)
{
   cpp_function* constr = new cpp_function(name_.c_str(), new cpp_type(CPP_TYPE_NOTYPE));
   constr->set_constructor();
   add_function(constr);
   switch(in)
   {
      case CPP_INHERIT_BASE_CLASS:
         // All the user-defined modules will pass through here
         assert(find_class(BASE_CLASS_NAME) != NULL);
         scope_.set_parent(find_class(BASE_CLASS_NAME)->get_scope());
         /* DON'T ADD A BREAK HERE!! */
      case CPP_INHERIT_SIM_OBJ:
         {
            constr->set_comment("Default simulation object constructor");
            implement_simulation_functions();
         }
         break;
      case CPP_INHERIT_EVENT:
         {
            // Only the event class will execute these lines
            constr->set_comment("Default event constructor");
            add_event_functions();
         }
         break;
      default:
         assert(false);
   }
}

/*
 * This constructor is basically created to build logic gates.
 */
   cppClass::cppClass(const cpp_class_type type)
: scope_(), inherit_(CPP_INHERIT_BASE_CLASS), type_(type)
{
   scope_.set_parent(find_class(BASE_CLASS_NAME)->get_scope());
   switch(type_)
   {
      case CPP_CLASS_OR:
         {
            name_ = "Or";
            cpp_function* constr = new cpp_function(name_.c_str(), new cpp_type(CPP_TYPE_NOTYPE));
            constr->set_constructor();
            constr->set_comment(name_ + " constructor");
            add_function(constr);
            implement_simulation_functions();
         }
         break;
      case CPP_CLASS_AND:
         {
            name_ = "And";
            cpp_function* constr = new cpp_function(name_.c_str(), new cpp_type(CPP_TYPE_NOTYPE));
            constr->set_constructor();
            constr->set_comment(name_ + " constructor");
            add_function(constr);
            implement_simulation_functions();
         }
         break;
      default:
         error("Class type not handled yet");
   }
}

/*
 * This method will create all methods and variables for a
 * class that inherit from warped::Event
 */
void cppClass::add_event_functions()
{
   // Create inherited function "receiverName"
   cpp_type* const_ref_string_type = new cpp_type(CPP_TYPE_STD_STRING);
   const_ref_string_type->set_const();
   const_ref_string_type->set_reference();
   cpp_function *rec_name = new cpp_function("receiverName", const_ref_string_type);
   rec_name->set_comment("Inherited getter method");
   rec_name->set_const();
   rec_name->set_virtual();
   rec_name->set_override();
   // Create inherited function "timestamp"
   cpp_type *timestamp_type = new cpp_type(CPP_TYPE_UNSIGNED_INT);
   cpp_function *timestamp = new cpp_function(WARPED_TIMESTAMP_FUN_NAME, timestamp_type);
   timestamp->set_comment("Inherited getter method");
   timestamp->set_const();
   timestamp->set_virtual();
   timestamp->set_override();
   // Create function to retrieve the name of the changed signal
   cpp_function *signal_name_getter = new cpp_function(SIGNAL_NAME_GETTER_FUN_NAME, const_ref_string_type);
   signal_name_getter->set_comment("Get the name of the changed signal");
   signal_name_getter->set_const();
   // Create function to retrieve the new value of the signal
   cpp_type* tribool_type = new cpp_type(CPP_TYPE_BOOST_TRIBOOL);
   cpp_function *new_value_getter = new cpp_function(NEW_VALUE_GETTER_FUN_NAME, tribool_type);
   new_value_getter->set_comment("Get the name of the new value of the signal");
   new_value_getter->set_const();
   // Create values to return
   add_function(rec_name);
   add_function(timestamp);
   add_function(signal_name_getter);
   add_function(new_value_getter);
   // Create all the members that simulation objects will use
   cpp_var *receiver_var = new cpp_var("receiver_name", const_ref_string_type);
   cpp_var *timestamp_var = new cpp_var("ts_", timestamp_type);
   timestamp_var->set_comment("Timestamp");
   cpp_var * signal_value = new cpp_var("new_value_", tribool_type);
   signal_value->set_comment("The new value");
   cpp_var * signal_name = new cpp_var("changed_signal_name", const_ref_string_type);
   signal_name->set_comment("The changed signal name");
   // Add all the members created
   add_var(receiver_var);
   add_var(timestamp_var);
   add_var(signal_value);
   add_var(signal_name);
   // Create the return statements
   timestamp->add_stmt(new cpp_unaryop_expr(CPP_UNARYOP_RETURN, timestamp_var->get_ref(), timestamp_var->get_type()));
   rec_name->add_stmt(new cpp_unaryop_expr(CPP_UNARYOP_RETURN, receiver_var->get_ref(), receiver_var->get_type()));
   signal_name_getter->add_stmt(new cpp_unaryop_expr(CPP_UNARYOP_RETURN, signal_name->get_ref(), signal_name->get_type()));
   new_value_getter->add_stmt(new cpp_unaryop_expr(CPP_UNARYOP_RETURN, signal_value->get_ref(), signal_value->get_type()));
   // Add init list to constructor.
   cpp_function* constr = get_costructor();
   assert(constr);
   // Add the name var
   cpp_var *name = new cpp_var("name", const_ref_string_type);
   constr->add_param(name);
   cpp_fcall_stmt* init_name = new cpp_fcall_stmt(const_ref_string_type, receiver_var->get_ref(), "");
   init_name->add_param(name->get_ref());
   constr->add_init(init_name);
   // Add the new_timestamp var
   cpp_var *new_timestamp = new cpp_var("new_timestamp", timestamp_type);
   constr->add_param(new_timestamp);
   cpp_fcall_stmt* init_timestamp = new cpp_fcall_stmt(const_ref_string_type, timestamp_var->get_ref(), "");
   init_timestamp->add_param(new_timestamp->get_ref());
   constr->add_init(init_timestamp);
   // Add the new_signal_value var
   cpp_var *new_signal_value = new cpp_var("new_signal_value", tribool_type);
   constr->add_param(new_signal_value);
   cpp_fcall_stmt* init_signal_value = new cpp_fcall_stmt(const_ref_string_type, signal_value->get_ref(), "");
   init_signal_value->add_param(new_signal_value->get_ref());
   constr->add_init(init_signal_value);
   // Add the new_signal_name var
   cpp_var *new_sig_name = new cpp_var("new_sig_name", const_ref_string_type);
   constr->add_param(new_sig_name);
   cpp_fcall_stmt* init_changed_signal_name = new cpp_fcall_stmt(const_ref_string_type, signal_name->get_ref(), "");
   init_changed_signal_name->add_param(new_sig_name->get_ref());
   constr->add_init(init_changed_signal_name);
}

void cppClass::add_simulation_functions()
{
   /*
    * Start creating vars.
    * Signal var.
    */
   cpp_type* string_type = new cpp_type(CPP_TYPE_STD_STRING);
   cpp_type* boost_type = new cpp_type(CPP_TYPE_BOOST_TRIBOOL);
   cpp_type* inside_input_map = new cpp_type(CPP_TYPE_NOTYPE, boost_type);
   inside_input_map->add_type(string_type);
   cpp_var *inputvar = new cpp_var(INPUT_VAR_NAME, new cpp_type(CPP_TYPE_STD_MAP, inside_input_map));
   inputvar->set_comment("map< signal_name, value >");
   // The Output var
   cpp_type* output_pair = new cpp_type(CPP_TYPE_STD_PAIR, string_type);
   output_pair->add_type(string_type);
   cpp_type* output_vec_pair = new cpp_type(CPP_TYPE_STD_VECTOR, output_pair);
   cpp_type* output_map_type = new cpp_type(CPP_TYPE_STD_MAP, output_vec_pair);
   output_map_type->add_type(string_type);
   cpp_var *output_var = new cpp_var(HIERARCHY_VAR_NAME, output_map_type);
   output_var->set_comment("map< mysignal, vector< pair< submodule, signals_in_submodule > > >");
   // The State var
   cpp_var *state_var = new cpp_var("state_", new cpp_type(CPP_TYPE_ELEMENT_STATE));
   state_var->set_comment("The State variable");
   /*
    * End vars creation.
    * Start creating functions.
    * Create the function to add a signal.
    */
   cpp_type *void_type = new cpp_type(CPP_TYPE_VOID);
   cpp_function *add_input_fun = new cpp_function(ADD_SIGNAL_FUN_NAME, void_type);
   cpp_type* const_ref_string_type = new cpp_type(CPP_TYPE_STD_STRING);
   const_ref_string_type->set_const();
   const_ref_string_type->set_reference();
   cpp_type* no_type = new cpp_type(CPP_TYPE_NOTYPE);
   cpp_const_expr* indeterminate_value = new cpp_const_expr("boost::indeterminate", no_type);
   cpp_var *input_name_var = new cpp_var("signal", const_ref_string_type);
   cpp_var *signal_value_var = new cpp_var("value", boost_type, indeterminate_value);
   add_input_fun->add_param(input_name_var);
   add_input_fun->add_param(signal_value_var);
   cpp_binop_expr* square = new cpp_binop_expr(inputvar->get_ref(), CPP_BINOP_SQUARE_BRACKETS, input_name_var->get_ref(), boost_type);
   cpp_assign_stmt* new_val = new cpp_assign_stmt(square, signal_value_var->get_ref());
   add_input_fun->add_stmt(new_val);
   add_input_fun->get_scope()->get_parent()->set_parent(&scope_);
   // Create the function to add an output
   cpp_function *add_output_fun = new cpp_function(ADD_OUTPUT_FUN_NAME, void_type);
   cpp_var *signal1 = new cpp_var("local_signal", const_ref_string_type);
   cpp_var *signal2 = new cpp_var("submodule", const_ref_string_type);
   cpp_var *signal3 = new cpp_var("module_signal", const_ref_string_type);
   add_output_fun->add_param(signal1);
   add_output_fun->add_param(signal2);
   add_output_fun->add_param(signal3);
   // Fill the function in with the instruction to push back
   cpp_binop_expr* at_fcall = new cpp_binop_expr(output_var->get_ref(), CPP_BINOP_SQUARE_BRACKETS, signal1->get_ref(), output_vec_pair);
   cpp_fcall_stmt* make_pair = new cpp_fcall_stmt(no_type, new cpp_const_expr("std::make_pair", no_type), "");
   make_pair->add_param(signal2->get_ref());
   make_pair->add_param(signal3->get_ref());
   cpp_fcall_stmt* push_back_fcall = new cpp_fcall_stmt(no_type, at_fcall, "push_back");
   push_back_fcall->add_param(make_pair);
   add_output_fun->add_stmt(push_back_fcall);
   add_output_fun->get_scope()->get_parent()->set_parent(&scope_);
   // Create the getState function
   cpp_type *get_state_ret_type = new cpp_type(CPP_TYPE_WARPED_OBJECT_STATE);
   get_state_ret_type->set_reference();
   cpp_function *get_state_fun = new cpp_function("getState", get_state_ret_type);
   get_state_fun->add_stmt(new cpp_unaryop_expr(CPP_UNARYOP_RETURN, state_var->get_ref(), state_var->get_type()));
   get_state_fun->set_override();
   get_state_fun->set_virtual();
   get_state_fun->get_scope()->get_parent()->set_parent(&scope_);
   /*
    * End functions creation
    * Add the init list to the constructor
    */
   cpp_function* constr = get_costructor();
   assert(constr);
   cpp_var *name = new cpp_var("name", const_ref_string_type);
   constr->add_param(name);
   cpp_var_ref* sim_obj = new cpp_var_ref("", new cpp_type(CPP_TYPE_WARPED_SIMULATION_OBJECT));
   cpp_fcall_stmt* init_name = new cpp_fcall_stmt(new cpp_type(CPP_TYPE_WARPED_SIMULATION_OBJECT), sim_obj, "");
   init_name->add_param(name->get_ref());
   constr->add_init(init_name);
   /*
    * Add all functions and vars created
    */
   add_var(inputvar);
   add_var(output_var);
   add_var(state_var);
   add_function(get_state_fun);
   add_function(add_input_fun);
   add_function(add_output_fun);
}

void cppClass::implement_simulation_functions()
{
   if(inherit_ == CPP_INHERIT_SIM_OBJ)
   {
      add_simulation_functions();
      return;
   }
   // Add the init list to the constructor
   cpp_type* const_ref_string_type = new cpp_type(CPP_TYPE_STD_STRING);
   cpp_type* boolean_type = new cpp_type(CPP_TYPE_BOOL);
   const_ref_string_type->set_const();
   const_ref_string_type->set_reference();
   cpp_function* constr = get_costructor();
   assert(constr);
   cpp_var *name = new cpp_var("name", const_ref_string_type);
   constr->add_param(name);
   cpp_var_ref* sim_obj = new cpp_var_ref("", new cpp_type(CPP_TYPE_CUSTOM_BASE_CLASS));
   cpp_fcall_stmt* init_name = new cpp_fcall_stmt(new cpp_type(CPP_TYPE_CUSTOM_BASE_CLASS), sim_obj, "");
   init_name->add_param(name->get_ref());
   constr->add_init(init_name);
   // Create the initial event function
   cpp_type *returnType = new cpp_type(CPP_TYPE_STD_VECTOR,
         new cpp_type(CPP_TYPE_SHARED_PTR,
            new cpp_type(CPP_TYPE_WARPED_EVENT)));
   cpp_function *init_fun = new cpp_function(WARPED_INIT_EVENT_FUN_NAME, returnType);
   init_fun->set_override();
   init_fun->set_virtual();
   init_fun->get_scope()->get_parent()->set_parent(&scope_);
   add_function(init_fun);
   // Create the event handler function
   cpp_function *event_handler = new cpp_function(WARPED_HANDLE_EVENT_FUN_NAME, returnType);
   event_handler->set_override();
   event_handler->set_virtual();
   cpp_type *event_type = new cpp_type(CPP_TYPE_WARPED_EVENT);
   event_type->set_reference();
   event_type->set_const();
   cpp_var *event_param = new cpp_var("event", event_type);
   event_handler->add_param(event_param);
   event_handler->get_scope()->get_parent()->set_parent(&scope_);
   // Add statements to functions
   cpp_var *response_event = new cpp_var(RETURN_EVENT_LIST_VAR_NAME, event_handler->get_type());
   response_event->set_comment("Return value");
   cpp_unaryop_expr* response_event_decl = new cpp_unaryop_expr(CPP_UNARYOP_DECL, response_event->get_ref(), response_event->get_type());
   event_handler->add_stmt(response_event_decl);
   init_fun->add_stmt(response_event_decl);
   event_handler->get_scope()->add_decl(response_event);
   cpp_type *local_event_type = new cpp_type(CPP_TYPE_CUSTOM_EVENT);
   local_event_type->set_const();
   local_event_type->set_reference();
   cpp_var *local_event = new cpp_var(CASTED_EVENT_VAR_NAME, local_event_type);
   event_handler->get_scope()->add_decl(local_event);
   // Start handling the event
   // Cast the event to the correct type
   cpp_assign_stmt *cast_stmt = new cpp_assign_stmt(new cpp_unaryop_expr(CPP_UNARYOP_DECL, local_event->get_ref(), local_event->get_type()), new cpp_unaryop_expr(CPP_UNARYOP_STATIC_CAST, event_param->get_ref(), local_event->get_type()));
   event_handler->add_stmt(cast_stmt);
   add_function(event_handler);
   cpp_type* string_type = new cpp_type(CPP_TYPE_STD_STRING);
   // Retrieve all the vars and funs I need
   cpp_var* inputvar = get_var(INPUT_VAR_NAME);
   assert(inputvar);
   cpp_var* output_var = get_var(HIERARCHY_VAR_NAME);
   assert(output_var);
   // Add an assert(I know the signal)
   cpp_binop_expr * signal_known = new cpp_binop_expr(CPP_BINOP_NEQ, boolean_type);
   cpp_fcall_stmt* find = new cpp_fcall_stmt(inputvar->get_type(), inputvar->get_ref(), "find");
   cpp_fcall_stmt * new_signal = new cpp_fcall_stmt(string_type, local_event->get_ref(), SIGNAL_NAME_GETTER_FUN_NAME);
   find->add_param(new_signal);
   signal_known->add_expr_front(find);
   signal_known->add_expr(new cpp_fcall_stmt(inputvar->get_type(), inputvar->get_ref(), "end"));
   event_handler->add_stmt(new cpp_assert(signal_known));
   // Store the new signal value
   cpp_fcall_stmt* change_in_stmt = new cpp_fcall_stmt(inputvar->get_type(), inputvar->get_ref(), "at");
   change_in_stmt->add_param(new_signal);
   cpp_fcall_stmt* new_value_fcall = new cpp_fcall_stmt(new cpp_type(CPP_TYPE_BOOST_TRIBOOL), local_event->get_ref(), NEW_VALUE_GETTER_FUN_NAME);
   cpp_assign_stmt* update_signal = new cpp_assign_stmt(change_in_stmt, new_value_fcall);
   update_signal->set_comment("Store the new value");
   event_handler->add_stmt(update_signal);
   // Create type that will be shared later on
   cpp_type* no_type = new cpp_type(CPP_TYPE_NOTYPE);
   cpp_type* output_pair = new cpp_type(CPP_TYPE_STD_PAIR, string_type);
   output_pair->add_type(string_type);
   cpp_type* list_iterator_type = new cpp_type(CPP_TYPE_STD_VECTOR, output_pair);
   list_iterator_type->set_iterator();
   // Here there are the implementation specific instructions
   switch(type_)
   {
      case CPP_CLASS_AND:
      case CPP_CLASS_OR:
         implement_gate();
         return;
      case CPP_CLASS_MODULE:
         {
            // Create the external for
            cpp_type* iterator_type = new cpp_type(*(inputvar->get_type()));
            iterator_type->set_iterator();
            cpp_var* ext_iterator = new cpp_var ("ext_it", iterator_type);
            cpp_assign_stmt* precycle = new cpp_assign_stmt(new cpp_unaryop_expr(CPP_UNARYOP_DECL, ext_iterator->get_ref(), ext_iterator->get_type()), new cpp_fcall_stmt(ext_iterator->get_type(), inputvar->get_ref(), "begin"));
            cpp_binop_expr * cond = new cpp_binop_expr(CPP_BINOP_NEQ, boolean_type);
            cond->add_expr(new cpp_unaryop_expr(CPP_UNARYOP_LITERAL, ext_iterator->get_ref(), ext_iterator->get_type()));
            cond->add_expr(new cpp_fcall_stmt(ext_iterator->get_type(), inputvar->get_ref(), "end"));
            cpp_for * ext_for = new cpp_for(cond);
            ext_for->add_precycle(precycle);
            ext_for->add_postcycle(new cpp_unaryop_expr(CPP_UNARYOP_ADD, ext_iterator->get_ref(), ext_iterator->get_type()));
            // Create the if to determine if the current signal has an indeterminate value
            cpp_fcall_stmt* indeter_expr = new cpp_fcall_stmt(no_type, new cpp_const_expr("boost::indeterminate", no_type), "");
            cpp_binop_expr * cond_indeter = new cpp_binop_expr(CPP_BINOP_AND, boolean_type);
            cpp_unaryop_expr * first_cond = new cpp_unaryop_expr(CPP_UNARYOP_NOT, indeter_expr, local_event->get_type());
            cpp_binop_expr * second_cond = new cpp_binop_expr(CPP_BINOP_NEQ, boolean_type);
            cpp_fcall_stmt* find_el = new cpp_fcall_stmt(output_var->get_type(), output_var->get_ref(), "find");
            cpp_fcall_stmt* cur_sig_name = new cpp_fcall_stmt(string_type, new cpp_unaryop_expr(CPP_UNARYOP_DEREF, ext_iterator->get_ref(), ext_iterator->get_type()), "first");
            cur_sig_name->set_member_access();
            find_el->add_param(cur_sig_name);
            second_cond->add_expr(find_el);
            second_cond->add_expr(new cpp_fcall_stmt(output_var->get_type(), output_var->get_ref(), "end"));
            cond_indeter->add_expr(first_cond);
            cond_indeter->add_expr(second_cond);
            cpp_fcall_stmt* signal_value = new cpp_fcall_stmt(string_type, new cpp_unaryop_expr(CPP_UNARYOP_DEREF, ext_iterator->get_ref(), ext_iterator->get_type()), "second");
            signal_value->set_member_access();
            indeter_expr->add_param(signal_value);
            cpp_if * defined_signal_if = new cpp_if(cond_indeter);
            // Create the internal for to alert averyone interested in that signal
            cpp_type* int_iterator_type = new cpp_type(*(output_var->get_type()));
            cpp_var* int_iterator = new cpp_var ("int_it", list_iterator_type);
            cpp_fcall_stmt* signal_name = new cpp_fcall_stmt(string_type, new cpp_unaryop_expr(CPP_UNARYOP_DEREF, ext_iterator->get_ref(), ext_iterator->get_type()), "first");
            signal_name->set_member_access();
            cpp_fcall_stmt* at_fun = new cpp_fcall_stmt(int_iterator->get_type(), output_var->get_ref(), "at");
            at_fun->add_param(signal_name);
            cpp_assign_stmt* ext_precycle = new cpp_assign_stmt(new cpp_unaryop_expr(CPP_UNARYOP_DECL, int_iterator->get_ref(), int_iterator_type), new cpp_fcall_stmt(int_iterator_type, at_fun, "begin"));
            cpp_binop_expr* ext_cond = new cpp_binop_expr(CPP_BINOP_NEQ, boolean_type);
            ext_cond->add_expr(new cpp_unaryop_expr(CPP_UNARYOP_LITERAL, int_iterator->get_ref(), int_iterator->get_type()));
            ext_cond->add_expr(new cpp_fcall_stmt(int_iterator_type, at_fun, "end"));
            cpp_for* int_for = new cpp_for(ext_cond);
            int_for->add_precycle(ext_precycle);
            int_for->add_postcycle(new cpp_unaryop_expr(CPP_UNARYOP_ADD, int_iterator->get_ref(), int_iterator->get_type()));
            // Create the events
            cpp_fcall_stmt* add_event = new cpp_fcall_stmt(response_event->get_type(), response_event->get_ref(), "emplace_back");
            cpp_fcall_stmt* receiver_name = new cpp_fcall_stmt(string_type, new cpp_unaryop_expr(CPP_UNARYOP_DEREF, int_iterator->get_ref(), int_iterator->get_type()), "first");
            receiver_name->set_member_access();
            signal_name = new cpp_fcall_stmt(string_type, new cpp_unaryop_expr(CPP_UNARYOP_DEREF, int_iterator->get_ref(), int_iterator->get_type()), "second");
            signal_name->set_member_access();
            cpp_const_expr* event_name = new cpp_const_expr(cpp_type::tostring(CPP_TYPE_CUSTOM_EVENT).c_str(), no_type);
            cpp_fcall_stmt* new_event_fcall = new cpp_fcall_stmt(no_type, event_name, "");
            cpp_unaryop_expr* new_event = new cpp_unaryop_expr(CPP_UNARYOP_NEW, new_event_fcall, local_event_type);
            new_event_fcall->add_param(receiver_name);
            new_event_fcall->add_param(new cpp_const_expr("0", new cpp_type(CPP_TYPE_UNSIGNED_INT)));
            new_event_fcall->add_param(signal_value);
            new_event_fcall->add_param(signal_name);
            add_event->add_param(new_event);
            int_for->add_to_body(add_event);
            // Build the for hierarchy and add to the init function
            defined_signal_if->add_to_body(int_for);
            ext_for->add_to_body(defined_signal_if);
            init_fun->add_stmt(ext_for);
         }
      default:
         break;
   }
   cpp_binop_expr * second_cond = new cpp_binop_expr(CPP_BINOP_NEQ, boolean_type);
   cpp_fcall_stmt* find_el = new cpp_fcall_stmt(output_var->get_type(), output_var->get_ref(), "find");
   find_el->add_param(new_signal);
   second_cond->add_expr(find_el);
   second_cond->add_expr(new cpp_fcall_stmt(output_var->get_type(), output_var->get_ref(), "end"));
   cpp_if* check_interest = new cpp_if(second_cond);
   // Add the cycle that will create the events
   cpp_var* iterator = new cpp_var ("it", list_iterator_type);
   cpp_fcall_stmt* at_fun = new cpp_fcall_stmt(iterator->get_type(), output_var->get_ref(), "at");
   at_fun->add_param(new_signal);
   cpp_assign_stmt* precycle = new cpp_assign_stmt(new cpp_unaryop_expr(CPP_UNARYOP_DECL, iterator->get_ref(), iterator->get_type()), new cpp_fcall_stmt(at_fun->get_type(), at_fun, "begin"));
   cpp_binop_expr * cond = new cpp_binop_expr(CPP_BINOP_NEQ, boolean_type);
   cond->add_expr(new cpp_unaryop_expr(CPP_UNARYOP_LITERAL, iterator->get_ref(), iterator->get_type()));
   cond->add_expr(new cpp_fcall_stmt(at_fun->get_type(), at_fun, "end"));
   cpp_for * push_event_for = new cpp_for(cond);
   push_event_for->add_precycle(precycle);
   push_event_for->add_postcycle(new cpp_unaryop_expr(CPP_UNARYOP_ADD, iterator->get_ref(), iterator->get_type()));
   cpp_fcall_stmt* add_event = new cpp_fcall_stmt(response_event->get_type(), response_event->get_ref(), "emplace_back");
   cpp_fcall_stmt* receiver_name = new cpp_fcall_stmt(string_type, new cpp_unaryop_expr(CPP_UNARYOP_DEREF, iterator->get_ref(), iterator->get_type()), "first");
   receiver_name->set_member_access();
   cpp_fcall_stmt* signal_name = new cpp_fcall_stmt(string_type, new cpp_unaryop_expr(CPP_UNARYOP_DEREF, iterator->get_ref(), iterator->get_type()), "second");
   signal_name->set_member_access();
   cpp_const_expr* event_name = new cpp_const_expr(cpp_type::tostring(CPP_TYPE_CUSTOM_EVENT).c_str(), no_type);
   cpp_fcall_stmt* new_event_fcall = new cpp_fcall_stmt(no_type, event_name, "");
   cpp_unaryop_expr* new_event = new cpp_unaryop_expr(CPP_UNARYOP_NEW, new_event_fcall, local_event_type);
   new_event_fcall->add_param(receiver_name);
   cpp_binop_expr * sum_timestamp = new cpp_binop_expr(CPP_BINOP_ADD, no_type);
   sum_timestamp->add_expr(new cpp_fcall_stmt(new cpp_type(CPP_TYPE_UNSIGNED_INT), local_event->get_ref(), WARPED_TIMESTAMP_FUN_NAME));
   sum_timestamp->add_expr(new cpp_const_expr("1", new cpp_type(CPP_TYPE_UNSIGNED_INT)));
   new_event_fcall->add_param(sum_timestamp);
   new_event_fcall->add_param(new_value_fcall);
   new_event_fcall->add_param(signal_name);
   add_event->add_param(new_event);
   push_event_for->add_to_body(add_event);
   check_interest->add_to_body(push_event_for);
   event_handler->add_stmt(check_interest);
   // Add the return statement
   cpp_unaryop_expr* return_stmt = new cpp_unaryop_expr(CPP_UNARYOP_RETURN, response_event->get_ref(), response_event->get_type());
   init_fun->add_stmt(return_stmt);
   event_handler->add_stmt(return_stmt);
}

void cppClass::implement_gate()
{
   // Retrieve all the vars and funs I need
   cpp_var* inputvar = get_var(INPUT_VAR_NAME);
   assert(inputvar);
   cpp_var* output_var = get_var(HIERARCHY_VAR_NAME);
   assert(output_var);
   cpp_type *local_event_type = new cpp_type(CPP_TYPE_CUSTOM_EVENT);
   cpp_var *local_event = new cpp_var(CASTED_EVENT_VAR_NAME, local_event_type);
   cpp_type* string_type = new cpp_type(CPP_TYPE_STD_STRING);
   cpp_type* boolean_type = new cpp_type(CPP_TYPE_BOOL);
   cpp_type* const_ref_string_type = new cpp_type(CPP_TYPE_STD_STRING);
   cpp_type* no_type = new cpp_type(CPP_TYPE_NOTYPE);
   cpp_function *event_handler = get_function(WARPED_HANDLE_EVENT_FUN_NAME);
   assert(event_handler);
   cpp_function *init_fun = get_function(WARPED_INIT_EVENT_FUN_NAME);
   cpp_type* output_pair = new cpp_type(CPP_TYPE_STD_PAIR, string_type);
   output_pair->add_type(string_type);
   cpp_type* list_iterator_type = new cpp_type(CPP_TYPE_STD_VECTOR, output_pair);
   list_iterator_type->set_iterator();
   cpp_var *response_event = event_handler->get_var(RETURN_EVENT_LIST_VAR_NAME);
   assert(response_event);
   cpp_fcall_stmt* new_value_fcall = new cpp_fcall_stmt(new cpp_type(CPP_TYPE_BOOST_TRIBOOL), local_event->get_ref(), NEW_VALUE_GETTER_FUN_NAME);
   // Add an assert(only one output)
   cpp_binop_expr * eq_to_one = new cpp_binop_expr(CPP_BINOP_EQ, boolean_type);
   cpp_fcall_stmt* size_call = new cpp_fcall_stmt(output_var->get_type(), output_var->get_ref(), "size");
   eq_to_one->add_expr(size_call);
   eq_to_one->add_expr(new cpp_const_expr("1", new cpp_type(CPP_TYPE_UNSIGNED_INT)));
   init_fun->add_stmt(new cpp_assert(eq_to_one));
   // Create the cycle to scan the inputs in order to determine the output
   cpp_type* iterator_type = new cpp_type(*(inputvar->get_type()));
   iterator_type->set_iterator();
   cpp_var* iterator = new cpp_var ("it", iterator_type);
   cpp_assign_stmt* precycle = new cpp_assign_stmt(new cpp_unaryop_expr(CPP_UNARYOP_DECL, iterator->get_ref(), iterator->get_type()), new cpp_fcall_stmt(iterator->get_type(), inputvar->get_ref(), "begin"));
   cpp_binop_expr * cond = new cpp_binop_expr(CPP_BINOP_NEQ, boolean_type);
   cond->add_expr(new cpp_unaryop_expr(CPP_UNARYOP_LITERAL, iterator->get_ref(), iterator->get_type()));
   cond->add_expr(new cpp_fcall_stmt(iterator->get_type(), inputvar->get_ref(), "end"));
   cpp_for * output_for = new cpp_for(cond);
   output_for->add_precycle(precycle);
   output_for->add_postcycle(new cpp_unaryop_expr(CPP_UNARYOP_ADD, iterator->get_ref(), iterator->get_type()));
   // Create the if to handle a false value on an input
   cpp_binop_expr * cond_gate_dep = new cpp_binop_expr(CPP_BINOP_EQ, boolean_type);
   cpp_unaryop_expr* it_deref = new cpp_unaryop_expr(CPP_UNARYOP_DEREF, iterator->get_ref(), inputvar->get_type());
   cpp_fcall_stmt * thesignal = new cpp_fcall_stmt(string_type, it_deref, "second");
   thesignal->set_member_access();
   cond_gate_dep->add_expr_front(thesignal);
   cpp_const_expr* gate_dep_expr = new cpp_const_expr("false", CPP_TYPE_NOTYPE);
   // Create/Add the output default value before
   switch(type_)
   {
      case CPP_CLASS_AND:
         gate_dep_expr = new cpp_const_expr("false", CPP_TYPE_NOTYPE);
         break;
      case CPP_CLASS_OR:
         gate_dep_expr = new cpp_const_expr("true", CPP_TYPE_NOTYPE);
         break;
      default:
         assert(false);
   }
   cond_gate_dep->add_expr(gate_dep_expr);
   cpp_fcall_stmt* begin_fun = new cpp_fcall_stmt(iterator->get_type(), output_var->get_ref(), "begin");
   cpp_fcall_stmt* deref = new cpp_fcall_stmt(const_ref_string_type, begin_fun, "first");
   deref->set_pointer_call();
   deref->set_member_access();
   cpp_fcall_stmt* get_el = new cpp_fcall_stmt(inputvar->get_type(), inputvar->get_ref(), "at");
   get_el->add_param(deref);
   cpp_assign_stmt* false_assign = new cpp_assign_stmt(get_el, gate_dep_expr);
   cpp_if * false_if = new cpp_if(cond_gate_dep);
   false_if->add_to_body(false_assign);
   false_if->add_to_body(new cpp_break());
   output_for->add_to_body(false_if);
   // Create the if to handle an indeterminate value on an input
   cpp_const_expr * boost_const = new cpp_const_expr("boost::indeterminate", no_type);
   cpp_fcall_stmt* indeter_expr = new cpp_fcall_stmt(boolean_type, boost_const, "");
   indeter_expr->add_param(thesignal);
   cpp_if *indeter_if = new cpp_if(indeter_expr);
   cpp_assign_stmt* indeter_assign = new cpp_assign_stmt(get_el, boost_const);
   indeter_if->add_to_body(indeter_assign);
   output_for->add_to_body(indeter_if);
   cpp_assign_stmt* assign_default;
   // Create/Add the output default value before
   switch(type_)
   {
      case CPP_CLASS_AND:
         assign_default = new cpp_assign_stmt(get_el, new cpp_const_expr("true", CPP_TYPE_NOTYPE));
         break;
      case CPP_CLASS_OR:
         assign_default = new cpp_assign_stmt(get_el, new cpp_const_expr("false", CPP_TYPE_NOTYPE));
         break;
      default:
         assert(false);
   }
   event_handler->add_stmt(assign_default);
   init_fun->add_stmt(assign_default);
   // Add the for to the functions
   event_handler->add_stmt(output_for);
   init_fun->add_stmt(output_for);
   /*
    * Add the cycle that will create the events
    */
   // Create the for.
   iterator = new cpp_var ("it", list_iterator_type);
   cpp_fcall_stmt* at_fun = new cpp_fcall_stmt(iterator->get_type(), output_var->get_ref(), "begin");
   cpp_fcall_stmt* to_vector = new cpp_fcall_stmt(string_type, at_fun, "second");
   to_vector->set_pointer_call();
   to_vector->set_member_access();
   precycle = new cpp_assign_stmt(new cpp_unaryop_expr(CPP_UNARYOP_DECL, iterator->get_ref(), iterator->get_type()), new cpp_fcall_stmt(at_fun->get_type(), to_vector, "begin"));
   cond = new cpp_binop_expr(CPP_BINOP_NEQ, boolean_type);
   cond->add_expr(new cpp_unaryop_expr(CPP_UNARYOP_LITERAL, iterator->get_ref(), iterator->get_type()));
   cond->add_expr(new cpp_fcall_stmt(at_fun->get_type(), to_vector, "end"));
   cpp_for * push_event_for_handler = new cpp_for(cond);
   push_event_for_handler->add_precycle(precycle);
   push_event_for_handler->add_postcycle(new cpp_unaryop_expr(CPP_UNARYOP_ADD, iterator->get_ref(), iterator->get_type()));
   cpp_for * push_event_for_init = new cpp_for(*push_event_for_handler);
   // Start creating stuff to instantiate the new event
   cpp_fcall_stmt* receiver_name = new cpp_fcall_stmt(string_type, new cpp_unaryop_expr(CPP_UNARYOP_DEREF, iterator->get_ref(), iterator->get_type()), "first");
   receiver_name->set_member_access();
   cpp_fcall_stmt* signal_name = new cpp_fcall_stmt(string_type, new cpp_unaryop_expr(CPP_UNARYOP_DEREF, iterator->get_ref(), iterator->get_type()), "second");
   signal_name->set_member_access();
   cpp_const_expr* event_name = new cpp_const_expr(cpp_type::tostring(CPP_TYPE_CUSTOM_EVENT).c_str(), no_type);
   cpp_fcall_stmt* new_event_fcall_handler = new cpp_fcall_stmt(no_type, event_name, "");
   cpp_fcall_stmt* new_event_fcall_init = new cpp_fcall_stmt(no_type, event_name, "");
   cpp_unaryop_expr* new_event_handler = new cpp_unaryop_expr(CPP_UNARYOP_NEW, new_event_fcall_handler, local_event_type);
   cpp_unaryop_expr* new_event_init = new cpp_unaryop_expr(CPP_UNARYOP_NEW, new_event_fcall_init, local_event_type);
   cpp_fcall_stmt* is_indeter = new cpp_fcall_stmt(boolean_type, boost_const, "");
   is_indeter->add_param(get_el);
   cpp_unaryop_expr * not_is_indeter = new cpp_unaryop_expr(CPP_UNARYOP_NOT, is_indeter, is_indeter->get_type());
   // Create the instruction that push back the new events
   cpp_fcall_stmt* add_event_handler = new cpp_fcall_stmt(response_event->get_type(), response_event->get_ref(), "emplace_back");
   cpp_fcall_stmt* add_event_init = new cpp_fcall_stmt(response_event->get_type(), response_event->get_ref(), "emplace_back");
   // Create the if that will contain the for
   cpp_if *if_not_indeter_handler = new cpp_if(not_is_indeter);
   cpp_if *if_not_indeter_init = new cpp_if(*if_not_indeter_handler);
   if_not_indeter_init->add_to_body(push_event_for_init);
   if_not_indeter_handler->add_to_body(push_event_for_handler);
   new_event_fcall_handler->add_param(receiver_name);
   cpp_binop_expr * sum_timestamp = new cpp_binop_expr(CPP_BINOP_ADD, no_type);
   sum_timestamp->add_expr(new cpp_fcall_stmt(new cpp_type(CPP_TYPE_UNSIGNED_INT), local_event->get_ref(), WARPED_TIMESTAMP_FUN_NAME));
   sum_timestamp->add_expr(new cpp_const_expr("3", new cpp_type(CPP_TYPE_UNSIGNED_INT)));
   new_event_fcall_handler->add_param(sum_timestamp);
   new_event_fcall_handler->add_param(new_value_fcall);
   new_event_fcall_handler->add_param(signal_name);
   new_event_fcall_init->add_param(receiver_name);
   new_event_fcall_init->add_param(new cpp_const_expr("0", new cpp_type(CPP_TYPE_UNSIGNED_INT)));
   new_event_fcall_init->add_param(get_el);
   new_event_fcall_init->add_param(signal_name);
   add_event_handler->add_param(new_event_handler);
   add_event_init->add_param(new_event_init);
   push_event_for_handler->add_to_body(add_event_handler);
   push_event_for_init->add_to_body(add_event_init);
   event_handler->add_stmt(if_not_indeter_handler);
   init_fun->add_stmt(if_not_indeter_init);
   // Add the return statements
   cpp_unaryop_expr* return_stmt = new cpp_unaryop_expr(CPP_UNARYOP_RETURN, response_event->get_ref(), response_event->get_type());
   init_fun->add_stmt(return_stmt);
   event_handler->add_stmt(return_stmt);
}


cpp_var* cppClass::get_var(const std::string &name) const
{
   cpp_decl* temp = scope_.get_decl(name);
   assert(temp);
   cpp_var* retvalue = dynamic_cast<cpp_var*>(temp);
   assert(retvalue);
   return retvalue;
}

cpp_function* cppClass::get_function(const std::string &name) const
{
   cpp_decl* temp = scope_.get_decl(name);
   assert(temp);
   cpp_function* retvalue = dynamic_cast<cpp_function*>(temp);
   assert(retvalue);
   return retvalue;
}


void cpp_context::emit_after_classes(std::ostream &of, int level) const
{
   newline(of, level);
   of << "int main(int argc, const char** argv) {";
   newline(of, indent(level));
   emit_children<cpp_stmt>(of, statements_, level, ";");
   newline(of, level);
   of << "}; ";
}

void cpp_if::emit(std::ostream &of, int level) const
{
   assert(condition_);
   newline(of, level);
   of << "if(";
   condition_->emit(of);
   of << ") {";
   if(!statements_.empty())
   {
      newline(of, indent(level));
      emit_children<cpp_stmt>(of, statements_, level, ";", false);
      of << ";";
   }
   newline(of, level);
   of << "}";
   if(!else_statements_.empty())
   {
      of << " else {";
      newline(of, indent(level));
      emit_children<cpp_stmt>(of, else_statements_, indent(level), ";", false);
      of << ";";
      newline(of, level);
      of << "}";
   }
}

void cpp_for::emit(std::ostream &of, int level) const
{
   assert(condition_);
   newline(of, level);
   of << "for(";
   if(!precycle_.empty())
      emit_children<cpp_expr>(of, precycle_, indent(level), ",", false);
   of << ";";
   if(condition_ != NULL) {
      newline(of, indent(level));
      condition_->emit(of);
   }
   of << ";";
   if(!postcycle_.empty()) {
      newline(of, indent(level));
      emit_children<cpp_expr>(of, postcycle_, indent(level), ",", false);
   }
   of << ") {";
   if(!statements_.empty()){
      newline(of, indent(indent(level)));
      emit_children<cpp_stmt>(of, statements_, indent(indent(level)), ";", false);
      of << ";";
   }
   newline(of, level);
   of << "}";
}

void cpp_context::emit_before_classes(std::ostream &of, int level) const
{
   newline(of, level);
   for(std::set<std::string>::iterator it = includes_.begin();
         it != includes_.end(); it++)
   {
      of << "#include <" + *it + ">";
      newline(of, level);
   }
   newline(of, level);
   of << "WARPED_DEFINE_OBJECT_STATE_STRUCT(" << cpp_type::tostring(CPP_TYPE_ELEMENT_STATE) << "){";
   if(!elem_parts_.empty())
      emit_children<cpp_var>(of, elem_parts_, indent(level), ";");
   of << "};";

   newline(of, level);
}

void cppClass::add_to_inputs(cpp_var* item)
{
   cpp_function* constr = get_costructor();
   assert(constr);
   cpp_decl* input_var = get_scope()->get_decl(INPUT_VAR_NAME);
   assert(input_var);
   cpp_fcall_stmt* add_event = new cpp_fcall_stmt(input_var->get_type(), new cpp_unaryop_expr(CPP_UNARYOP_LITERAL, new cpp_var_ref(input_var->get_name(), input_var->get_type()), input_var->get_type()), "emplace");
   add_event->add_param(new cpp_const_expr(item->get_name().c_str(), new cpp_type(CPP_TYPE_STD_STRING)));
   add_event->add_param(new cpp_const_expr("boost::indeterminate", new cpp_type(CPP_TYPE_NOTYPE)));
   constr->add_stmt(add_event);
   // The following instruction is to avoid problems handling nexus
   get_scope()->add_visible(item);
}

void cppClass::add_to_hierarchy(cpp_var* item)
{
   cpp_function* constr = get_costructor();
   assert(constr);
   cpp_decl* hierarchy_var = get_scope()->get_decl(HIERARCHY_VAR_NAME);
   assert(hierarchy_var);
   cpp_fcall_stmt* add_event = new cpp_fcall_stmt(hierarchy_var->get_type(), new cpp_unaryop_expr(CPP_UNARYOP_LITERAL, new cpp_var_ref(hierarchy_var->get_name(), hierarchy_var->get_type()), hierarchy_var->get_type()), "emplace_back");
   add_event->add_param(new cpp_const_expr(item->get_name().c_str(), new cpp_type(CPP_TYPE_STD_STRING)));
   constr->add_stmt(add_event);
   // The following instruction is to avoid problems handling nexus
   get_scope()->add_visible(item);
}

void cppClass::emit(std::ostream &of, int level) const
{
   newline(of, level);
   emit_comment(of, level);

   of << "class " << name_;
   switch(inherit_)
   {
      case CPP_INHERIT_SIM_OBJ:
         of << " : public warped::SimulationObject";
         break;
      case CPP_INHERIT_EVENT:
         of << " : public warped::Event";
         break;
      case CPP_INHERIT_BASE_CLASS:
         of << " : public " << BASE_CLASS_NAME;
         break;
      default:
         break;
   }
   of << " {";
   newline(of, level);
   of << "public:";

   emit_children<cpp_decl>(of, scope_.get_printable(), indent(level), ";");

   newline(of, level);
   of << "};";
   newline(of, 0);
}

const cpp_type *cpp_decl::get_type() const
{
   assert(type_);
   return type_;
}

void cpp_unaryop_expr::emit(std::ostream &of, int level) const
{
   open_parens(of);

   switch (op_) {
      case CPP_UNARYOP_NOT:
         of << "!";
         break;
      case CPP_UNARYOP_NEW:
         of << "new ";
         break;
      case CPP_UNARYOP_STATIC_CAST:
         of << "static_cast<";
         type_->emit(of);
         of << ">";
         open_parens(of);
         break;
      case CPP_UNARYOP_DEREF:
         open_parens(of);
         of << "*";
         break;
      case CPP_UNARYOP_LITERAL:
         break;
      case CPP_UNARYOP_DECL:
         operand_->get_type()->emit(of, level);
         of << " ";
         break;
      case CPP_UNARYOP_NEG:
         of << "- ";
         break;
      case CPP_UNARYOP_ADD:
         break;
      case CPP_UNARYOP_RETURN:
         of << "return ";
         break;
      default:
         error("Unary operation not supported");
   }
   operand_->emit(of, level);

   // Some unaryop need to close the parenthesis
   switch(op_)
   {
      case CPP_UNARYOP_STATIC_CAST:
      case CPP_UNARYOP_DEREF:
         close_parens(of);
         break;
      case CPP_UNARYOP_ADD:
         of << "++";
         break;
      default:
         break;
   }
   close_parens(of);
}

void cpp_fcall_stmt::emit(std::ostream &of, int level) const
{
   base_->emit(of, level);
   if(!member_name_.empty())
   {
      if(is_pointer_call)
         of << "->" << member_name_;
      else
         of << "." << member_name_;
   }
   if(is_function_call)
   {
      of << "(";
      parameters_->emit(of, indent(level));
      of << ")";
   }
}

cpp_var* cpp_function::get_var(const std::string &name) const
{
   cpp_decl* temp = variables_.get_decl(name);
   assert(temp);
   cpp_var* retvalue = dynamic_cast<cpp_var*>(temp);
   assert(retvalue);
   return retvalue;
}

void cpp_function::emit(std::ostream &of, int level) const
{
   newline(of, level);
   emit_comment(of, level);
   if(isvirtual)
      of << "virtual ";
   type_->emit(of, level);
   of << " " << name_ << " (";
   emit_children<cpp_decl>(of, scope_.get_printable(), indent(level), ",", false);
   of << ")";
   if(isconst)
      of << " const";
   if(isoverride)
      of << " override";
   if(isconstructor)
   {
      of << " : ";
      emit_children<cpp_fcall_stmt>(of, init_list_, indent(level), ",", false);
   }
   of << " {";
   if(!statements_.empty()) {
      newline(of, indent(level));
      emit_children<cpp_stmt>(of, statements_, level, ";");
   }
   of << "}";
}

cpp_var_ref* cpp_var::get_ref()
{
   if(ref_to_this_var == NULL)
      ref_to_this_var = new cpp_var_ref(name_, type_);
   return ref_to_this_var;
}

void cpp_assert::emit(std::ostream &of, int level) const
{
   // expr_ == NULL means assert(false)
   of << "assert(";
   if(expr_ != NULL){
      assert(expr_->get_type()->get_name() == CPP_TYPE_INT ||
            expr_->get_type()->get_name() == CPP_TYPE_BOOL);
      expr_->emit(of, level);
      }
   else
      of << "false";
   of << ")";
}

void cpp_var::emit(std::ostream &of, int level) const
{
   newline(of, level);
   emit_comment(of, level);
   type_->emit(of, level);
   of << " " << name_;
   if(default_value != NULL)
   {
      of << " = ";
      default_value->emit(of, level);
   }
}
