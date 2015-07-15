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
   while(++it != operands_.end())
   {
      switch(op_)
      {
         case CPP_BINOP_EQ:
            of << " == ";
            break;
         case CPP_BINOP_AND:
            of << " and ";
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
         default:
            error("This binary operation is not supported");
      }
      (*it)->emit(of, level);
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
      : scope_(), inherit_(in) , type_(CPP_CLASS_MODULE)
{
   switch(in)
   {
      case CPP_INHERIT_BASE_CLASS:
         scope_.set_parent(find_class(BASE_CLASS_NAME)->get_scope());
         /* DON'T ADD A BREAK HERE!! */
      case CPP_INHERIT_SIM_OBJ:
         {
            name_ = name;
            cpp_function* constr = new cpp_function(name_.c_str(), new cpp_type(CPP_TYPE_NOTYPE));
            constr->set_constructor();
            constr->set_comment("Default simulation object constructor");
            add_function(constr);
            implement_simulation_functions();
         }
         break;
      case CPP_INHERIT_EVENT:
         {
            name_ = name;
            cpp_function* constr = new cpp_function(name_.c_str(), new cpp_type(CPP_TYPE_NOTYPE));
            constr->set_constructor();
            constr->set_comment("Default event constructor");
            add_function(constr);
            add_event_functions();
         }
         break;
      default:
         assert(false);
   }
}

cppClass::cppClass(const string& name, const cpp_class_type type)
      : scope_(), inherit_(CPP_INHERIT_BASE_CLASS), type_(type)
{
   scope_.set_parent(find_class(BASE_CLASS_NAME)->get_scope());
   switch(type_)
   {
      case CPP_CLASS_MODULE:
         {
            name_ = name;
            cpp_function* constr = new cpp_function(name_.c_str(), new cpp_type(CPP_TYPE_NOTYPE));
            constr->set_constructor();
            constr->set_comment("Default simulation object constructor");
            add_function(constr);
            implement_simulation_functions();
         }
         break;
      case CPP_CLASS_AND:
         {
            name_ = name + "And";
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
   cpp_function *new_value_getter = new cpp_function("newValue", tribool_type);
   new_value_getter->set_comment("Get the name of the new value of the signal");
   new_value_getter->set_const();
   // Create values to return
   cpp_var *receiver_var = new cpp_var("receiver_name", const_ref_string_type);
   cpp_var *timestamp_var = new cpp_var("ts_", timestamp_type);
   add_var(receiver_var);
   add_var(timestamp_var);
   add_function(rec_name);
   add_function(timestamp);
   add_function(signal_name_getter);
   add_function(new_value_getter);
   // Create the members that simulation objects will use
   cpp_var * signal_value = new cpp_var("new_value_", tribool_type);
   signal_value->set_comment("The new input value");
   cpp_var * signal_name = new cpp_var("changed_signal_name", const_ref_string_type);
   signal_name->set_comment("The changed input signal name");
   add_var(signal_value);
   add_var(signal_name);
   // Create return statements
   timestamp->add_stmt(new cpp_unaryop_expr(CPP_UNARYOP_RETURN, timestamp_var->get_ref(), timestamp_var->get_type()));
   rec_name->add_stmt(new cpp_unaryop_expr(CPP_UNARYOP_RETURN, receiver_var->get_ref(), receiver_var->get_type()));
   signal_name_getter->add_stmt(new cpp_unaryop_expr(CPP_UNARYOP_RETURN, signal_name->get_ref(), signal_name->get_type()));
   new_value_getter->add_stmt(new cpp_unaryop_expr(CPP_UNARYOP_RETURN, signal_value->get_ref(), signal_value->get_type()));
   // Add init list to constructor.
   cpp_function* constr = get_costructor();
   // Add name var
   cpp_var *name = new cpp_var("name", const_ref_string_type);
   constr->add_param(name);
   cpp_fcall_stmt* init_name = new cpp_fcall_stmt(const_ref_string_type, receiver_var->get_ref(), "");
   init_name->add_param(name->get_ref());
   constr->add_init(init_name);
   // Add new_signal_name var
   cpp_var *new_sig_name = new cpp_var("new_sig_name", const_ref_string_type);
   constr->add_param(new_sig_name);
   cpp_fcall_stmt* init_changed_signal_name = new cpp_fcall_stmt(const_ref_string_type, signal_name->get_ref(), "");
   init_changed_signal_name->add_param(new_sig_name->get_ref());
   constr->add_init(init_changed_signal_name);
   // Add new_timestamp var
   cpp_var *new_timestamp = new cpp_var("new_timestamp", timestamp_type);
   constr->add_param(new_timestamp);
   cpp_fcall_stmt* init_timestamp = new cpp_fcall_stmt(const_ref_string_type, timestamp_var->get_ref(), "");
   init_timestamp->add_param(new_timestamp->get_ref());
   constr->add_init(init_timestamp);
   // Add new_signal_value var
   cpp_var *new_signal_value = new cpp_var("new_signal_value", timestamp_type);
   constr->add_param(new_signal_value);
   cpp_fcall_stmt* init_signal_value = new cpp_fcall_stmt(const_ref_string_type, signal_value->get_ref(), "");
   init_signal_value->add_param(new_signal_value->get_ref());
   constr->add_init(init_signal_value);
}

void cppClass::add_simulation_functions()
{
   // Start creating vars
   // Signal var
   cpp_type* string_type = new cpp_type(CPP_TYPE_STD_STRING);
   cpp_type* inside_input_map = new cpp_type(CPP_TYPE_NOTYPE, new cpp_type(CPP_TYPE_BOOST_TRIBOOL));
   inside_input_map->add_type(string_type);
   cpp_var *inputvar = new cpp_var(INPUT_VAR_NAME, new cpp_type(CPP_TYPE_STD_MAP, inside_input_map));
   inputvar->set_comment("map< signal_name, value >");
   // Output var
   cpp_type* output_pair = new cpp_type(CPP_TYPE_STD_PAIR, string_type);
   output_pair->add_type(string_type);
   cpp_type* output_vec_pair = new cpp_type(CPP_TYPE_STD_VECTOR, output_pair);
   cpp_type* output_map_type = new cpp_type(CPP_TYPE_STD_MAP, output_vec_pair);
   output_map_type->add_type(string_type);
   cpp_var *output_var = new cpp_var(HIERARCHY_VAR_NAME, output_map_type);
   output_var->set_comment("map< mysignal, vector< pair< submodule, signals_in_submodule > > >");
   // state var
   cpp_var *state_var = new cpp_var("state_", new cpp_type(CPP_TYPE_ELEMENT_STATE));
   state_var->set_comment("The State variable");
   // End vars creation
   // Start creating functions
   // Create addInput function
   cpp_type *void_type = new cpp_type(CPP_TYPE_VOID);
   cpp_function *add_input_fun = new cpp_function("addInput", void_type);
   cpp_type* const_ref_string_type = new cpp_type(CPP_TYPE_STD_STRING);
   const_ref_string_type->set_const();
   const_ref_string_type->set_reference();
   cpp_var *input_name_var = new cpp_var("signal", const_ref_string_type);
   add_input_fun->add_param(input_name_var);
   cpp_fcall_stmt* emplace_fcall = new cpp_fcall_stmt(void_type, inputvar->get_ref(), "emplace");
   cpp_type* no_type = new cpp_type(CPP_TYPE_NOTYPE);
   emplace_fcall->add_param(new cpp_const_expr(input_name_var->get_name().c_str(), no_type));
   emplace_fcall->add_param(new cpp_const_expr("boost::indeterminate", no_type));
   add_input_fun->add_stmt(emplace_fcall);
   add_input_fun->get_scope()->get_parent()->set_parent(&scope_);
   // Create getState function
   cpp_type *get_state_ret_type = new cpp_type(CPP_TYPE_WARPED_OBJECT_STATE);
   get_state_ret_type->set_reference();
   cpp_function *get_state_fun = new cpp_function("getState", get_state_ret_type);
   get_state_fun->add_stmt(new cpp_unaryop_expr(CPP_UNARYOP_RETURN, state_var->get_ref(), state_var->get_type()));
   get_state_fun->set_override();
   get_state_fun->set_virtual();
   get_state_fun->get_scope()->get_parent()->set_parent(&scope_);
   // End functions creation
   // Add init list to the constructor
   cpp_function* constr = get_costructor();
   cpp_var *name = new cpp_var("name", const_ref_string_type);
   constr->add_param(name);
   cpp_var_ref* sim_obj = new cpp_var_ref("", new cpp_type(CPP_TYPE_WARPED_SIMULATION_OBJECT));
   cpp_fcall_stmt* init_name = new cpp_fcall_stmt(new cpp_type(CPP_TYPE_WARPED_SIMULATION_OBJECT), sim_obj, "");
   init_name->add_param(name->get_ref());
   constr->add_init(init_name);
   // Add all functions and vars created
   // Add all variables
   add_var(inputvar);
   add_var(output_var);
   add_var(state_var);
   // Add all functions
   add_function(get_state_fun);
   add_function(add_input_fun);
}

void cppClass::implement_simulation_functions()
{
   if(inherit_ == CPP_INHERIT_SIM_OBJ)
   {
      add_simulation_functions();
      return;
   }
   // Add init list to the constructor
   cpp_type* const_ref_string_type = new cpp_type(CPP_TYPE_STD_STRING);
   cpp_function* constr = get_costructor();
   cpp_var *name = new cpp_var("name", const_ref_string_type);
   constr->add_param(name);
   cpp_var_ref* sim_obj = new cpp_var_ref("", new cpp_type(CPP_TYPE_CUSTOM_BASE_CLASS));
   cpp_fcall_stmt* init_name = new cpp_fcall_stmt(new cpp_type(CPP_TYPE_CUSTOM_BASE_CLASS), sim_obj, "");
   init_name->add_param(name->get_ref());
   constr->add_init(init_name);
   // Create initial event function
   cpp_type *returnType = new cpp_type(CPP_TYPE_STD_VECTOR,
         new cpp_type(CPP_TYPE_SHARED_PTR,
         new cpp_type(CPP_TYPE_WARPED_EVENT)));
   cpp_function *init_fun = new cpp_function(WARPED_INIT_EVENT_FUN_NAME, returnType);
   init_fun->set_override();
   init_fun->set_virtual();
   init_fun->get_scope()->get_parent()->set_parent(&scope_);
   add_function(init_fun);
   // Create event handler function
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
   event_handler->add_stmt(new cpp_unaryop_expr(CPP_UNARYOP_DECL, response_event->get_ref(), response_event->get_type()));
   event_handler->get_scope()->add_decl(response_event);
   cpp_type *local_event_type = new cpp_type(CPP_TYPE_CUSTOM_EVENT);
   local_event_type->set_const();
   local_event_type->set_reference();
   cpp_var *local_event = new cpp_var(CASTED_EVENT_VAR_NAME, local_event_type);
   event_handler->get_scope()->add_decl(local_event);
   // Start handling the event
   cpp_assign_stmt *cast_stmt = new cpp_assign_stmt(new cpp_unaryop_expr(CPP_UNARYOP_DECL, local_event->get_ref(), local_event->get_type()), new cpp_unaryop_expr(CPP_UNARYOP_STATIC_CAST, event_param->get_ref(), local_event->get_type()));
   event_handler->add_stmt(cast_stmt);
   add_function(event_handler);
   //
   cpp_type* string_type = new cpp_type(CPP_TYPE_STD_STRING);
   // Retrieve all the vars and funs I need
   cpp_var* inputvar = get_var(INPUT_VAR_NAME);
   cpp_var* output_var = get_var(HIERARCHY_VAR_NAME);
   // Change my internal state according to the event received
   cpp_if * change_input_if = new cpp_if();
   cpp_binop_expr * cond_if = new cpp_binop_expr(CPP_BINOP_NEQ, local_event->get_type());
   cpp_fcall_stmt* find = new cpp_fcall_stmt(inputvar->get_type(), inputvar->get_ref(), "find");
   cpp_fcall_stmt * new_signal = new cpp_fcall_stmt(string_type, local_event->get_ref(), SIGNAL_NAME_GETTER_FUN_NAME);
   find->add_param(new_signal);
   cond_if->add_expr_front(find);
   cond_if->add_expr(new cpp_fcall_stmt(inputvar->get_type(), inputvar->get_ref(), "end"));
   change_input_if->set_condition(cond_if);
   event_handler->add_stmt(change_input_if);
   // Here there are the implementation specific instructions
   switch(type_)
   {
      case CPP_CLASS_MODULE:
      default:
         break;
   }
   // Add the cycle that will create the events
   cpp_type* iterator_type = new cpp_type(*(output_var->get_type()));
   iterator_type->set_iterator();
   cpp_var* iterator = new cpp_var ("it", iterator_type);
   cpp_assign_stmt* precycle = new cpp_assign_stmt(new cpp_unaryop_expr(CPP_UNARYOP_DECL, iterator->get_ref(), iterator->get_type()), new cpp_fcall_stmt(iterator->get_type(), output_var->get_ref(), "begin"));
   cpp_for * push_event_for = new cpp_for();
   push_event_for->add_precycle(precycle);
   cpp_binop_expr * cond = new cpp_binop_expr(CPP_BINOP_NEQ, iterator_type);
   cond->add_expr(new cpp_unaryop_expr(CPP_UNARYOP_LITERAL, iterator->get_ref(), iterator->get_type()));
   cond->add_expr(new cpp_fcall_stmt(iterator->get_type(), output_var->get_ref(), "end"));
   push_event_for->set_condition(cond);
   push_event_for->add_postcycle(new cpp_unaryop_expr(CPP_UNARYOP_ADD, iterator->get_ref(), iterator->get_type()));
   cpp_fcall_stmt* add_event = new cpp_fcall_stmt(response_event->get_type(), new cpp_unaryop_expr(CPP_UNARYOP_LITERAL, response_event->get_ref(), response_event->get_type()), "emplace_back");
   add_event->add_param(new cpp_unaryop_expr(CPP_UNARYOP_DEREF, iterator->get_ref(), iterator->get_type()));
   add_event->add_param(new_signal);
   add_event->add_param(new cpp_fcall_stmt(new cpp_type(CPP_TYPE_UNSIGNED_INT), local_event->get_ref(), WARPED_TIMESTAMP_FUN_NAME));
   add_event->add_param(new cpp_fcall_stmt(new cpp_type(CPP_TYPE_BOOST_TRIBOOL), local_event->get_ref(), "newValue"));
   push_event_for->add_to_body(add_event);
   event_handler->add_stmt(push_event_for);
   // Add the return statement
   event_handler->add_stmt(new cpp_unaryop_expr(CPP_UNARYOP_RETURN, response_event->get_ref(), response_event->get_type()));
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
   emit_children<cpp_stmt>(of, statements_, indent(level), ";");
   newline(of, level);
   of << "}; ";
}

void cpp_if::set_condition(cpp_expr* p) {
   assert(!condition_);
   condition_ = p;
};

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
      emit_children<cpp_stmt>(of, statements_, indent(level), ";", false);
      of << ";";
   }
   newline(of, level);
   of << "}";
}

void cpp_for::emit(std::ostream &of, int level) const
{
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
   of << "WARPED_DEFINE_OBJECT_STATE_STRUCT(" << cpp_type::tostring(CPP_TYPE_ELEMENT_STATE) << "){";
   if(!elem_parts_.empty())
      emit_children<cpp_var>(of, elem_parts_, indent(level), ";");
   of << "};";

   newline(of, level);
}

void cppClass::add_to_inputs(cpp_var* item)
{
   cpp_function* constr = get_costructor();
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
       of << "not ";
      break;
   case CPP_UNARYOP_NEW:
       of << "new ";
      break;
   case CPP_UNARYOP_STATIC_CAST:
       of << "static_cast<";
       type_->emit(of);
       of << ">(";
      break;
   case CPP_UNARYOP_DEREF:
       of << "*(";
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

   // Cast needs to close the parenthesis
   if(op_ == CPP_UNARYOP_STATIC_CAST ||
         op_ == CPP_UNARYOP_DEREF)
       of << ")";
   
   if(op_ == CPP_UNARYOP_ADD)
       of << "++";
   close_parens(of);
}

void cpp_fcall_stmt::emit(std::ostream &of, int level) const
{
   base_->emit(of, level);
   if(!fun_name_.empty())
   {
      if(is_pointer_call)
         of << "->" << fun_name_;
      else
         of << "." << fun_name_;
   }
   of << "(";
   parameters_->emit(of, indent(level));
   of << ")";
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

void cpp_var::emit(std::ostream &of, int level) const
{
   newline(of, level);
   emit_comment(of, level);
   type_->emit(of, level);
   of << " " << name_;
}
