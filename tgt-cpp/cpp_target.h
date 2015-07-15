// -*- mode: c++ -*-
#ifndef INC_CPP_TARGET_H
#define INC_CPP_TARGET_H

#include "cpp_config.h"
#include "ivl_target.h"

#include "support.hh"
#include "cpp_syntax.hh"

#include <string>

using namespace std;

void error(const char *fmt, ...);
void debug_msg(const char *fmt, ...);

int draw_scope(ivl_scope_t scope, void *_parent);
extern "C" int draw_process(ivl_process_t net, void *cd);
void draw_nexus(ivl_nexus_t nexus);

cpp_var_ref *nexus_to_var_ref(cpp_scope *scope, ivl_nexus_t nexus);
cpp_var_ref* readable_ref(cpp_scope* scope, ivl_nexus_t nex);
string make_safe_name(ivl_signal_t sig);
void require_support_function(support_function_t f);
void draw_logic(cppClass *arch, ivl_net_logic_t log);

#endif /* #ifndef INC_CPP_TARGET_H */
