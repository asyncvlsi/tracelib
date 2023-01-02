/*************************************************************************
 *
 *  Copyright (c) 2022 Rajit Manohar
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 *
 **************************************************************************
 */
#ifndef __ACT_TRACEIF_H__
#define __ACT_TRACEIF_H__

#ifdef __cplusplus
extern "C" {
#endif

  typedef enum act_bool_val {
       ACT_SIG_BOOL_FALSE = 0,
       ACT_SIG_BOOL_TRUE = 1,
       ACT_SIG_BOOL_X = 2,
       ACT_SIG_BOOL_Z = 3
  } act_bool_val_t;

  typedef enum act_chan_state {
       ACT_CHAN_RECV_BLOCKED = 0,
       ACT_CHAN_SEND_BLOCKED = 1,
       ACT_CHAN_IDLE = 2,
       ACT_CHAN_VAL_OFFSET = 3
  } act_chan_state_t;

  typedef enum act_signal_type {
    ACT_SIG_BOOL = 0,
    ACT_SIG_INT = 1,
    ACT_SIG_CHAN = 2,	/* channels: unused by various formats */
    ACT_SIG_ANALOG = 3  /* real number: voltages and currents */
  } act_signal_type_t;

#define ACT_TRACE_CHAN_WIDTH(w) ((w) < 2 ? ((w)+2) : ((w)+1))
#define ACT_TRACE_WIDE_NUM(w) (((w)+8*sizeof (unsigned long)-1)/(8*sizeof(unsigned long))))

  typedef struct {
    /* return a handle to the trace file for subsequent calls */
    void *(*create_tracefile) (const char *base, float stop_time, float ts);

    /* create tracefile, integer timesteps of arbitrary size, with ts
       as the conversion between int and real time */
    void *(*create_tracefile_alt) (const char *base, float stop_time, float ts);

    /* start adding signal names */
    int (*add_signal_start) (void *handle);

    /* add different signal types */
    void *(*add_analog_signal) (void *handle, const char *s);
    void *(*add_digital_signal) (void *handle, const char *s);
    void *(*add_int_signal) (void *handle, const char *s, int width);
    void *(*add_chan_signal) (void *handle, const char *s, int width);

    /* finish adding signal names */
    int (*add_signal_end) (void *handle);

    /* start adding initial values */
    int (*init_start) (void *handle);

    /* 
       use the general signal change functions below to provide
       initial values for all variables here
    */

    /* finished with initial value */
    int (*init_end) (void *handle);
    

    struct {
	/* use this for BOOL, INT, and CHAN */
      int (*signal_change_digital) (void *handle, void *node, float t,
				      unsigned long v);

      /* for > 64-bit values for INT/CHAN */
      int (*signal_change_wide_digital) (void *handle, void *node, float t,
					   int len,
					   unsigned long *v);

      /* for analog signals */
      int (*signal_change_analog) (void *handle, void *node, float t, float v);
    } std;

    struct {
      int (*signal_change_digital) (void *handle, void *node, int len,
					 unsigned long *tm,  unsigned long v);

      int (*signal_change_wide_digital) (void *handle, void *node, int len,
					      unsigned long *tm,
					      int len2,
					      unsigned long *v);

      int (*signal_change_analog) (void *handle, void *node, int len,
				   unsigned long *tm, float v);
    } alt;

    /* close output file */
    int (*close_tracefile) (void *handle);

  } act_extern_trace_func_t;

  typedef struct {
    int mode;			/* float t = 0, int t = 1 */
    int state;
    /* 0 = open, ready for signal creation
       1 = in-signal-creation
       2 = signal create done, ready for init
       3 = in-init
       4 = init-done, ready for signal changes
       5 = in signal change
    */
    void *handle;
    act_extern_trace_func_t *t;
  } act_trace_t;
    

  /* Load trace file library.
     Function names are prefixed by "prefix_", and must be:
     
     create - mapped to create_tracefile (mode=0)
     create_alt - mapped to create tracefile for alt format (mode=1)

     signal_start - mapped to add_signal_start
     add_analog_signal - mapped to ... 
     add_digital_signal - ...
     add_int_signal  - ...
     add_chan_signal - ...
     signal_end - finish creating signal names, mapped to add_signal_end

     init_start - mapped to ...
     init_end - mapped to ...
     
     change_digital - ... 
     change_analog  - ...
     change_wide_digital - ...

     change_digital_alt - ... 
     change_analog_alt  - ...
     change_wide_digital_alt - ...
     
     close - close_tracefile

     If your file format ooes not support a signal type, you can omit
     the funcftions from the library. Those signals will be skipped.
  */
  act_extern_trace_func_t *act_trace_load_format (const char *prefix, const char *dl);

  /* return a handle to the trace file for subsequent calls 
     
     mode = 0 : use functions that have floating-point times
     mode = 1 : use functions that have integer times
   */
  act_trace_t *act_trace_create (act_extern_trace_func_t *,
				 const char *name, float stop_time, float ts,
				 int mode);
   

  void *act_trace_add_signal (act_trace_t *,  act_signal_type_t type,
			      const char *s, int width);

  int act_trace_init_start (act_trace_t *);
  int act_trace_init_end (act_trace_t *);

  int act_trace_analog_change (act_trace_t *, void *node, float t, float v);
  int act_trace_digital_change (act_trace_t *, void *node, float t, unsigned long v);
  int act_trace_wide_digital_change (act_trace_t *, void *node, float t,
				     int len, unsigned long *v);

  int act_trace_analog_change_alt (act_trace_t *, void *node, int len, unsigned long *tm, float v);
  int act_trace_digital_change_alt (act_trace_t *, void *node, int len, unsigned long *tm, unsigned long v);
  int act_trace_wide_digital_change_alt (act_trace_t *, void *node, int len, unsigned long *tm, int lenv, unsigned long *v);
  
  int act_trace_close (act_trace_t *);

#ifdef __cplusplus
}
#endif

#endif /* __ACT_TRACEIF_H__ */
