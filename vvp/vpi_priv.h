#ifndef __vpi_priv_H
#define __vpi_priv_H
/*
 * Copyright (c) 2001-2003 Stephen Williams (steve@icarus.com)
 *
 *    This source code is free software; you can redistribute it
 *    and/or modify it in source code form under the terms of the GNU
 *    General Public License as published by the Free Software
 *    Foundation; either version 2 of the License, or (at your option)
 *    any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */
#ifdef HAVE_CVS_IDENT
#ident "$Id: vpi_priv.h,v 1.47 2003/02/09 23:33:26 steve Exp $"
#endif

# include  "vpi_user.h"
# include  "pointers.h"
# include  "memory.h"

/*
 * This header file contains the internal definitions that the vvp
 * program uses to implement the public interface in the vpi_user.h
 * header file elsewhere.
 */


/*
 * The vpi_mode_flag contains the major mode for VPI use. This is used
 * to generate error messages when vpi functions are called
 * incorrectly.
 */
enum vpi_mode_t {
      VPI_MODE_NONE =0,
	/* The compiler is calling a register function. */
      VPI_MODE_REGISTER,
	/* The compiler is calling a compiletf function. */
      VPI_MODE_COMPILETF,
	/* The compiler is calling a calltf function. */
      VPI_MODE_CALLTF,
	/* We are in the midst of a RWSync callback. */
      VPI_MODE_RWSYNC,
	/* We are in a ROSync callback. */
      VPI_MODE_ROSYNC
};
extern vpi_mode_t vpi_mode_flag;

/*
 * This structure is the very base of a vpiHandle. Every handle
 * structure starts with this structure, so that the library can
 * internally pass the derived types as pointers to one of these.
 */
struct __vpiHandle {
      const struct __vpirt *vpi_type;
};

/*
 * Objects with this structure are used to represent a type of
 * vpiHandle. A specific object becomes of this type by holding a
 * pointer to an instance of this structure.
 */
struct __vpirt {
      int type_code;

	/* These methods extract information from the handle. */
      int   (*vpi_get_)(int, vpiHandle);
      char* (*vpi_get_str_)(int, vpiHandle);
      void  (*vpi_get_value_)(vpiHandle, p_vpi_value);
      vpiHandle (*vpi_put_value_)(vpiHandle, p_vpi_value, p_vpi_time, int);

	/* These methods follow references. */
      vpiHandle (*handle_)(int, vpiHandle);
      vpiHandle (*iterate_)(int, vpiHandle);
      vpiHandle (*index_)(vpiHandle, int);

	/* This implements the vpi_free_object method. */
      int (*vpi_free_object_)(vpiHandle);
};

/*
 * The vpiHandle for an iterator has this structure. The definition of
 * the methods lives in vpi_iter.c
 *
 * The args and nargs members point to the array of vpiHandle objects
 * that are to be iterated over. The next member is the index of the
 * next item to be returned by a vpi_scan.
 *
 * The free_args_flag member is true if when this iterator object is
 * released it must also free the args array.
 */
struct __vpiIterator {
      struct __vpiHandle base;
      vpiHandle *args;
      unsigned  nargs;
      unsigned  next;
      bool free_args_flag;
};

extern vpiHandle vpip_make_iterator(unsigned nargs, vpiHandle*args,
				    bool free_args_flag);

/*
 * This represents callback handles. There are some private types that
 * are defined and used in vpi_callback.cc.
 */
struct __vpiCallback {
      struct __vpiHandle base;

	// user supplied callback data
      struct t_cb_data cb_data;
      struct t_vpi_time cb_time;

	// scheduled event
      struct sync_cb* cb_sync;

	// Used for listing callbacks.
      struct __vpiCallback*next;
};

extern struct __vpiCallback* new_vpi_callback();
extern void callback_execute(struct __vpiCallback*cur);

struct __vpiSystemTime {
      struct __vpiHandle base;
      struct __vpiScope *scope;
};

/*
 * Scopes are created by .scope statements in the source. These
 * objects hold the items and properties that are knowingly bound to a
 * scope.
 */
struct __vpiScope {
      struct __vpiHandle base;
      struct __vpiScope *scope;
	/* The scope has a name. */
      const char*name;
	/* The scope has a system time of its own. */
      struct __vpiSystemTime scoped_time;
      struct __vpiSystemTime scoped_realtime;
	/* Keep an array of internal scope items. */
      struct __vpiHandle**intern;
      unsigned nintern;
	/* Keep a list of threads in the scope. */
      vthread_t threads;
      signed int time_units :8;
};

extern struct __vpiScope* vpip_peek_current_scope(void);
extern void vpip_attach_to_current_scope(vpiHandle obj);
extern vpiHandle vpip_make_root_iterator(void);

/*
 * Signals include the variable types (reg, integer, time) and are
 * distinguished by the vpiType code. They also have a parent scope,
 * a declared name and declaration indices.
 */
struct __vpiSignal {
      struct __vpiHandle base;
      struct __vpiScope* scope;
	/* The name of this reg/net object */
      const char*name;
	/* The indices that define the width and access offset. */
      int msb, lsb;
	/* Flags */
      unsigned signed_flag  : 1;
      unsigned isint_ : 1;	// original type was integer
	/* The represented value is here. */
      vvp_fvector_t bits;
        /* This is the callback event functor */
      struct callback_functor_s *callback;
};
extern vpiHandle vpip_make_int(const char*name, int msb, int lsb,
			       vvp_fvector_t vec);
extern vpiHandle vpip_make_reg(const char*name, int msb, int lsb,
			       bool signed_flag, vvp_fvector_t vec);
extern vpiHandle vpip_make_net(const char*name, int msb, int lsb,
			       bool signed_flag, vvp_fvector_t vec);

/*
 * These methods support the vpi creation of events. The name string
 * passed in will be saved, so the caller must allocate it (or not
 * free it) after it is handed to this function.
 */
struct __vpiNamedEvent {
      struct __vpiHandle base;
	/* base name of the event object */
      const char*name;
	/* Parent scope of this object. */
      struct __vpiScope*scope;
	/* List of callbacks interested in this event. */
      struct __vpiCallback*callbacks;
};

extern vpiHandle vpip_make_named_event(const char*name);
extern void vpip_run_named_event_callbacks(vpiHandle ref);

/*
 * Memory is an array of bits that is accessible in N-bit chunks, with
 * N being the width of a word. The memory word handle just points
 * back to the memory and uses an index to identify its position in
 * the memory.
 */

extern vpiHandle vpip_make_memory(vvp_memory_t mem);

/*
 * These are the various variable types.
 */
extern vpiHandle vpip_make_real_var(const char*name);

/*
 * When a loaded VPI module announces a system task/function, one
 * __vpiUserSystf object is created to hold the definition of that
 * task/function. The distinction between task and function is stored
 * in the vpi_systf_data structure data that was supplied by the
 * external module.
 *
 * When the compiler encounters a %vpi_call statement, it creates a
 * __vpiSysTaskCall to represent that particular call. The call refers
 * to the definition handle so that when the %vpi_call instruction is
 * encountered at run-time, the definition can be located and used.
 *
 * The vpiSysTaskCall handles both functions and tasks, as the two are
 * extremely similar. The different VPI type is reflected in a
 * different vpi_type pointer in the base structure. The only
 * additional part is the vbit/vwid that is used by the put of the
 * system function call to place the values in the vthread bit space.
 */
struct __vpiUserSystf {
      struct __vpiHandle base;
      s_vpi_systf_data info;
};

struct __vpiSysTaskCall {
      struct __vpiHandle base;
      struct __vpiScope* scope;
      struct __vpiUserSystf*defn;
      unsigned nargs;
      vpiHandle*args;
	/* Support for vpi_get_userdata. */
      void*userdata;
	/* These represent where in the vthread to put the return value. */
      unsigned short vbit;
      signed short   vwid;
};

extern struct __vpiSysTaskCall*vpip_cur_task;

/*
 * These are implemented in vpi_const.cc. These are vpiHandles for
 * constants.
 */
struct __vpiStringConst {
      struct __vpiHandle base;
      const char*value;
};

vpiHandle vpip_make_string_const(char*text);

struct __vpiBinaryConst {
      struct __vpiHandle base;
      unsigned nbits;
      unsigned char*bits;
      unsigned signed_flag :1;
};

vpiHandle vpip_make_binary_const(unsigned wid, char*bits);

struct __vpiDecConst {
      struct __vpiHandle base;
      int value;
};

vpiHandle vpip_make_dec_const(int value);
vpiHandle vpip_make_dec_const(struct __vpiDecConst*obj, int value);

/*
 *  This one looks like a constant, but really is a vector in the current 
 *  thread. 
 */

vpiHandle vpip_make_vthr_vector(unsigned base, unsigned wid, bool signed_flag);

vpiHandle vpip_make_vthr_word(unsigned base, const char*type);

/*
 * This function is called before any compilation to load VPI
 * modules. This gives the modules a chance to announce their
 * contained functions before compilation commences. It is called only
 * once per module.
 */
extern void vpip_load_module(const char*name);

# define VPIP_MODULE_PATH_MAX 64
extern const char* vpip_module_path[64];
extern unsigned vpip_module_path_cnt;

/*
 * The vpip_build_vpi_call function creates a __vpiSysTaskCall object
 * and returns the handle. The compiler uses this function when it
 * encounters a %vpi_call statement.
 *
 * The %vpi_call instruction has as its only parameter the handle that
 * is returned by the vpip_build_vpi_call. This includes all the
 * information needed by vpip_execute_vpi_call to actually execute the
 * call. However, the vpiSysTaskCall that is the returned handle,
 * holds a parameter argument list that is passed in here.
 *
 * The vbit and vwid fields are used if this turns out to be a system
 * function. In that case, the vbit and vwid are used to address the
 * vector is thread bit space where the result is supposed to go.
 *
 * Note that the argv array is saved in the handle, and should should
 * not be released by the caller.
 */
extern vpiHandle vpip_build_vpi_call(const char*name,
				     unsigned vbit, int vwid,
				     unsigned argc,
				     vpiHandle*argv);

extern vthread_t vpip_current_vthread;

extern void vpip_execute_vpi_call(vthread_t thr, vpiHandle obj);


/*
 * These are functions used by the compiler to prepare for compilation
 * and to finish compilation in preparation for execution.
 */

vpiHandle vpip_sim_time(struct __vpiScope*scope);
vpiHandle vpip_sim_realtime(struct __vpiScope*scope);

extern int vpip_get_time_precision(void);
extern void vpip_set_time_precision(int pres);

extern void vpip_time_to_timestruct(struct t_vpi_time*ts, vvp_time64_t ti);
extern vvp_time64_t vpip_timestruct_to_time(const struct t_vpi_time*ts);

extern const char* vpip_string(const char*str);

/*
**  Functions defined in vpi_scope.cc, to keep track of functor scope.
*/

extern vpiHandle ipoint_get_scope(vvp_ipoint_t ipt);
extern void functor_set_scope(vpiHandle scope);

/*
 * This function is used to make decimal string versions of various
 * vectors. The input format is an array of bit values (0, 1, 2, 3)
 * lsb first, and the result is written into buf, without overflowing
 * nbuf.
 */
extern unsigned vpip_bits_to_dec_str(const unsigned char *bits,
				     unsigned int nbits,
				     char *buf, unsigned int nbuf,
				     int signed_flag);

extern void vpip_dec_str_to_bits(unsigned char*bits, unsigned nbits,
				 const char*buf, bool signed_flag);

extern void vpip_bin_str_to_bits(unsigned char*bits, unsigned nbits,
				 const char*buf, bool signed_flag);

extern void vpip_hex_str_to_bits(unsigned char*bits, unsigned nbits,
				 const char*buf, bool signed_flag);

extern void vpip_bits_to_oct_str(const unsigned char*bits, unsigned nbits,
				 char*buf, unsigned nbuf, bool signed_flag);

extern void vpip_oct_str_to_bits(unsigned char*bits, unsigned nbits,
				 const char*buf, bool signed_flag);

/*
 * Function defined in vpi_signal.cc to manage vpi_get_* persistent
 * storage.
 */
enum vpi_rbuf_t {
      RBUF_VAL =0,
	/* Storage for *_get_value() */
      RBUF_STR
	/* Storage for *_get_str() */
};
extern char *need_result_buf(unsigned cnt, vpi_rbuf_t type); 

/*
 * $Log: vpi_priv.h,v $
 * Revision 1.47  2003/02/09 23:33:26  steve
 *  Spelling fixes.
 *
 * Revision 1.46  2003/02/01 05:50:04  steve
 *  Make $time and $realtime available to $display uniquely.
 *
 * Revision 1.45  2003/01/27 00:14:37  steve
 *  Support in various contexts the $realtime
 *  system task.
 *
 * Revision 1.44  2003/01/26 18:16:22  steve
 *  Add %cvt/ir and %cvt/ri instructions, and support
 *  real values passed as arguments to VPI tasks.
 *
 * Revision 1.43  2003/01/25 23:48:06  steve
 *  Add thread word array, and add the instructions,
 *  %add/wr, %cmp/wr, %load/wr, %mul/wr and %set/wr.
 *
 * Revision 1.42  2003/01/09 04:09:44  steve
 *  Add vpi_put_userdata
 *
 * Revision 1.41  2002/12/21 00:55:58  steve
 *  The $time system task returns the integer time
 *  scaled to the local units. Change the internal
 *  implementation of vpiSystemTime the $time functions
 *  to properly account for this. Also add $simtime
 *  to get the simulation time.
 *
 * Revision 1.40  2002/08/12 01:35:09  steve
 *  conditional ident string using autoconfig.
 *
 * Revision 1.39  2002/07/09 03:24:37  steve
 *  Dynamic resizevpi result buf in more places.
 *
 * Revision 1.38  2002/07/05 17:14:15  steve
 *  Names of vpi objects allocated as vpip_strings.
 *
 * Revision 1.37  2002/06/21 04:58:55  steve
 *  Add support for special integer vectors.
 *
 * Revision 1.36  2002/05/19 05:18:16  steve
 *  Add callbacks for vpiNamedEvent objects.
 *
 * Revision 1.35  2002/05/18 02:34:11  steve
 *  Add vpi support for named events.
 *
 *  Add vpi_mode_flag to track the mode of the
 *  vpi engine. This is for error checking.
 *
 * Revision 1.34  2002/05/17 04:12:19  steve
 *  Rewire vpiMemory and vpiMemoryWord handles to
 *  support proper iteration of words, and the
 *  vpiIndex value.
 *
 * Revision 1.33  2002/05/11 04:39:35  steve
 *  Set and get memory words by string value.
 *
 * Revision 1.32  2002/05/03 15:44:11  steve
 *  Add vpiModule iterator to vpiScope objects.
 *
 * Revision 1.31  2002/04/20 04:33:23  steve
 *  Support specified times in cbReadOnlySync, and
 *  add support for cbReadWriteSync.
 *  Keep simulation time in a 64bit number.
 */
#endif
