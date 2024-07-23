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
       ACT_CHAN_SEND_BLOCKED = 0, /* this encoding matches atrace.h */
       ACT_CHAN_RECV_BLOCKED = 1,
       ACT_CHAN_IDLE = 2,
       ACT_CHAN_VALUE = 3
  } act_chan_state_t;

  typedef enum act_signal_type {
    ACT_SIG_BOOL = 0,
    ACT_SIG_INT = 1,
    ACT_SIG_CHAN = 2,	/* channels: unused by various formats */
    ACT_SIG_ANALOG = 3  /* real number: voltages and currents */
  } act_signal_type_t;

  typedef union act_signal_val {
    float v;			/* for analog values */
    unsigned long val;		/* for small digital values up to 63 bits */
    unsigned long *valp;	/* for >63 bit values */
  } act_signal_val_t;

#define ACT_TRACE_WIDE_NUM(w) (((w)+8*sizeof (unsigned long)-1)/(8*sizeof(unsigned long)))

  typedef struct {

    unsigned int has_reader:1;
    unsigned int has_writer:1;
    
    /*--- writer API ---*/
    
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
      /* use this for BOOL, INT */
      int (*signal_change_digital) (void *handle, void *node, float t,
				      unsigned long v);

      /* for > 64-bit values for INT */
      int (*signal_change_wide_digital) (void *handle, void *node, float t,
					   int len,
					   unsigned long *v);

      /* use this for CHAN */
      int (*signal_change_chan) (void *handle, void *node, float t,
				 act_chan_state_t s, unsigned long v);

      /* for > 64-bit CHAN */
      int (*signal_change_wide_chan) (void *handle, void *node, float t,
				      act_chan_state_t s,  int len,
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

      int (*signal_change_chan) (void *handle, void *node, int len,
				 unsigned long *tm,
				 act_chan_state_t s, unsigned long v);

      int (*signal_change_wide_chan) (void *handle, void *node, int len,
				      unsigned long *tm,
				      act_chan_state_t s, int len2,
				      unsigned long *v);

      int (*signal_change_analog) (void *handle, void *node, int len,
				   unsigned long *tm, float v);
    } alt;


    /*--- reader API ---*/

    /* open file for reading */
    void *(*open_tracefile) (const char *base);
    void *(*open_tracefile_alt) (const char *base);

    /* read metadata */
    void (*read_header) (void *handle, float *stop_time, float *dt);

    /* return signal, NULL if missing */
    void *(*signal_lookup) (void *handle, const char *name);

    /* return signal type */
    act_signal_type_t (*signal_type) (void *handle, void *node);

    void (*advance_time) (void *handle, int nsteps);
    void (*advance_time_by) (void *handle, float delta);

    act_signal_val_t (*get_signal) (void *nandle, void *node);

    int (*has_more_data) (void *handle);

    /* close trace file */
    int (*close_tracefile) (void *handle);

    /* use this to close the library */
    void *dlib;

  } act_extern_trace_func_t;

  typedef struct {
    unsigned int mode:1;	/* float t = 0, int t = 1 */
    unsigned int readonly:1;	/*  1 = reader, 0 = create */
    
    int state;
    /* 0 = open, ready for signal creation
       1 = in-signal-creation
       2 = signal create done, ready for init
       3 = in-init
       4 = init-done, ready for signal changes
       5 = in signal change or ready to read 
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
     change_chan - ...
     change_wide_chan - ...

     change_digital_alt - ... 
     change_analog_alt  - ...
     change_wide_digital_alt - ...
     change_chan_alt - ...
     change_wide_chan_alt - ...
     
     close - close_tracefile

     If your file format ooes not support a signal type, you can omit
     the funcftions from the library. Those signals will be skipped.
  */
  act_extern_trace_func_t *act_trace_load_format (const char *prefix, const char *dl);

  void act_trace_close_format (act_extern_trace_func_t *fmt);

  /* return 1 if the format files have a reader API, 0 otherwise */
  int act_trace_fmt_has_reader (act_extern_trace_func_t *);

  /* return 1 if the format files have a writer API, 0 otherwise */
  int act_trace_fmt_has_writer (act_extern_trace_func_t *);
  

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
  int act_trace_chan_change (act_trace_t *, void *node, float t,
			     act_chan_state_t s, unsigned long v);
  int act_trace_wide_chan_change (act_trace_t *, void *node, float t,
				  act_chan_state_t s, int len, unsigned long *v);

  int act_trace_analog_change_alt (act_trace_t *, void *node, int len, unsigned long *tm, float v);
  int act_trace_digital_change_alt (act_trace_t *, void *node, int len, unsigned long *tm, unsigned long v);
  int act_trace_wide_digital_change_alt (act_trace_t *, void *node, int len, unsigned long *tm, int lenv, unsigned long *v);
  int act_trace_chan_change_alt (act_trace_t *, void *node, int len, unsigned long *tm, act_chan_state_t s, unsigned long v);
  int act_trace_wide_chan_change_alt (act_trace_t *, void *node, int len, unsigned long *tm, act_chan_state_t s, int lenv, unsigned long *v);
  
  int act_trace_close (act_trace_t *);

  int act_trace_has_alt (act_extern_trace_func_t *);



  /*-- API for reading --*/

  /* mode = 0/1 : same as the .._create() function */
  act_trace_t *act_trace_open (act_extern_trace_func_t *,
			       const char *name, int mode);
   

  /* get parameters from the header: if info is missing from the file
     format, a -1 is returned in stop_time/dt */
  void act_trace_header (act_trace_t *, float *stop_time, float *dt);

  /* get a signal pointer given the signal name */
  void *act_trace_lookup (act_trace_t *, const char *name);

  /* return the signal type: bool, int, channel, analog */
  act_signal_type_t act_trace_sigtype (act_trace_t *, void *sig);

  /* advance time by steps (in units of deltaT) or by an actual amount
     in SI units. */
  void act_trace_advance_steps (act_trace_t *, int steps);
  void act_trace_advance_time (act_trace_t *, float dt);

  /* returns 1 if there are future timesteps in the trace file, 0
     otherwise */
  int act_trace_has_more_data (act_trace_t *);

  /* get the value of the specified signal */
  act_signal_val_t act_trace_get_signal (act_trace_t *, void *sig);

  /* get value, shortcuts optimized for specific signal types */
  unsigned long act_trace_get_smallval (act_trace_t *, void *sig);
  float act_trace_get_analog (act_trace_t *, void *sig);

#ifdef __cplusplus
}
#endif

#endif /* __ACT_TRACEIF_H__ */
