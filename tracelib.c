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
#include <stdio.h>
#include <dlfcn.h>
#include <string.h>
#include <stdlib.h>
#include "tracelib.h"

#define NEW(a,b)							\
  do {									\
    (a) = (b *) malloc (sizeof (b));					\
    if (!(a)) {								\
      fprintf (stderr, "FATAL: could not allocate %lu bytes\n", sizeof (b)); \
      exit (1);								\
    }									\
  } while (0)

act_extern_trace_func_t *act_trace_load_format (const char *prefix, const char *dl)
{
  void *dlib;
  char *buf;
  char *tmpdl;
  int l;
  int i;
  int err;
  act_extern_trace_func_t *fn, t;

  struct {
    const char *name;
    void **offset;
    int mustbe;
  } fns[] =
      {
       /* create file */
       { "create", (void **)&t.create_tracefile, 0 },
       { "create_alt", (void **)&t.create_tracefile_alt, 0 },

       /* record signals */
       { "signal_start", (void **)&t.add_signal_start, 0 },
       { "add_analog_signal", (void **)&t.add_analog_signal, 0 },
       { "add_digital_signal", (void **)&t.add_digital_signal, 0 },
       { "add_int_signal", (void **)&t.add_int_signal, 0 },
       { "add_chan_signal", (void **)&t.add_chan_signal, 0 },
       { "signal_end", (void **)&t.add_signal_end, 0 },

       /* initial block */
       { "init_start", (void **)&t.init_start, 0 },
       { "init_end", (void **)&t.init_end, 0 },

       /* floating-point time change */
       { "change_digital", (void **)&t.std.signal_change_digital, 0 },
       { "change_analog", (void **)&t.std.signal_change_analog, 0 },
       { "change_wide_digital", (void **) &t.std.signal_change_wide_digital, 0 },
       { "change_chan", (void **)&t.std.signal_change_chan, 0 },
       { "change_wide_chan", (void **) &t.std.signal_change_wide_chan, 0 },

       /* integer time change */
       { "change_digital_alt", (void **)&t.alt.signal_change_digital, 0 },
       { "change_analog_alt", (void **)&t.alt.signal_change_analog, 0 },
       { "change_wide_digital_alt", (void **) &t.alt.signal_change_wide_digital, 0 },
       { "change_chan_alt", (void **)&t.alt.signal_change_chan, 0 },
       { "change_wide_chan_alt", (void **) &t.alt.signal_change_wide_chan, 0 },

       /* close file */
       { "close", (void **) &t.close_tracefile, 1 },


       /* open file */
       { "open", (void **)&t.open_tracefile, 0 },
       { "open_alt", (void **)&t.open_tracefile_alt, 0 },

       { "header", (void **)&t.read_header, 0 },
       { "signal_lookup", (void **)&t.signal_lookup, 0 },
       { "signal_type", (void **)&t.signal_type, 0 },

       { "advance_time", (void **)&t.advance_time, 0 },
       { "advance_time_by", (void **)&t.advance_time_by, 0 },
       { "get_signal", (void **)&t.get_signal, 0 },
       { "has_more_data", (void **)&t.has_more_data, 0 },
       
       { NULL, NULL, 0 }
      };

  if (!prefix) {
    return NULL;
  }

  tmpdl = NULL;
  buf = NULL;

  /* if library name not specified, use the default name */
  if (!dl) {
    l = strlen (prefix) + 14;
    tmpdl = (char *) malloc (l);
    if (!tmpdl) return NULL;
    snprintf (tmpdl, l, "libtrace_%s.so", prefix);
  }

#if defined(TRACELIB_ENV)
  /* check default location: TRACELIB_ENV/lib/name */
  if (getenv (TRACELIB_ENV)) {
    FILE *fp;
    l = strlen (getenv (TRACELIB_ENV)) + strlen(dl ? dl : tmpdl) + 6;
    buf = (char *) malloc (l);
    if (!buf) {
      return NULL;
    }
    snprintf (buf, l, "%s/lib/%s", getenv (TRACELIB_ENV), dl ? dl : tmpdl);
    fp = fopen (buf, "r");
    if (!fp) {
      free (buf);
      buf = NULL;
    }
  }
#endif
  
  if (!buf) {
    l = strlen (dl ? dl : tmpdl) + 1;
    buf = (char *) malloc (l);
    if (!buf) {
      if (tmpdl) {
	free (tmpdl);
      }
      return NULL;
    }
    snprintf (buf, l, "%s", dl ? dl : tmpdl);
  }

  dlib = dlopen (buf, RTLD_LAZY);
  if (!dlib) {
    fprintf (stderr, "ERROR: failed to open `%s' as a trace library (prefix=%s)\n",
	     buf, prefix);
    free (buf);
    if (tmpdl) {
      free (tmpdl);
    }
    return NULL;
  }
  free (buf);

  l = strlen (prefix) + 32;
  buf = (char *) malloc (sizeof (char)*l);
  if (!buf) {
    fprintf (stderr, "FATAL: could not allocate %d bytes\n", l);
    exit (1);
  }

  err = 0;
  for (i=0; fns[i].name; i++) {
    void *sym;
    snprintf (buf, l, "%s_%s", prefix, fns[i].name);

    sym = dlsym (dlib, buf);
    if (sym) {
      *(fns[i].offset) = sym;
    }
    else if (fns[i].mustbe) {
      fprintf (stderr, "ERROR: missing required symbol %s from library %s\n",
	       buf, dl ? dl : tmpdl);
      err++;
    }
    else {
      *(fns[i].offset) = NULL;
    }
  }
  free (buf);
  if (err) {
    if (tmpdl) {
      free (tmpdl);
    }
    dlclose (dlib);
    return NULL;
  }

  t.has_reader = 0;


#define CHECK_FOR(func)							\
    do {								\
      if (!t.func) {							\
	fprintf (stderr, "ERROR: missing " #func " functionality in library %s\n", dl ? dl : tmpdl); \
	if (tmpdl) {							\
	  free (tmpdl);							\
	}								\
	dlclose (dlib);							\
	return NULL;							\
      }									\
    } while (0)
  

  if (!t.open_tracefile_alt && !t.open_tracefile) {
    t.has_reader = 0;
  }
  else {
    t.has_reader = 1;
    /* check for rest of reader functions */

    CHECK_FOR (read_header);
    CHECK_FOR (signal_lookup);
    CHECK_FOR (signal_type);
    if (!t.advance_time && !t.advance_time_by) {
      CHECK_FOR (advance_time);
    }
    CHECK_FOR (get_signal);
    CHECK_FOR (has_more_data);
  }

  if (!t.create_tracefile && !t.create_tracefile_alt) {
    t.has_writer = 0;
    if (!t.has_reader) {
      fprintf (stderr, "ERROR: missing read or write functionality in library %s\n",
	       dl ? dl : tmpdl);
      if (tmpdl) {
	free (tmpdl);
      }
      dlclose (dlib);
      return NULL;
    }
  }
  else {
    t.has_writer = 1;
  }
  if (t.has_writer) {
    CHECK_FOR (add_signal_start);
    CHECK_FOR (add_signal_end);
    CHECK_FOR (init_start);
    CHECK_FOR (init_end);
    
    if (!t.add_analog_signal && !t.add_digital_signal && !t.add_int_signal &&
	!t.add_chan_signal) {
      fprintf (stderr, "ERROR: missing all add_signal functions in library %s\n",
	       dl ? dl : tmpdl);
      if (tmpdl) {
	free (tmpdl);
      }
      dlclose (dlib);
      return NULL;
    }
    if (t.create_tracefile) {
      if (t.add_analog_signal && !t.std.signal_change_analog) {
	fprintf (stderr, "ERROR: missing analog signal change in library %s\n",
		 dl ? dl : tmpdl);
	if (tmpdl) {
	  free (tmpdl);
	}
	dlclose (dlib);
	return NULL;
      }
      if (t.add_digital_signal || t.add_int_signal) {
	if (!t.std.signal_change_digital) {
	  fprintf (stderr, "ERROR: missing digital signal change in library %s\n",
		   dl ? dl : tmpdl);
	  if (tmpdl) {
	    free (tmpdl);
	  }
	  dlclose (dlib);
	  return NULL;
	}
	if (t.add_int_signal && !t.std.signal_change_wide_digital) {
	  fprintf (stderr, "WARNING: missing wide digital signal change in library %s\n",
		   dl ? dl : tmpdl);
	}
      }
      if (t.add_chan_signal) {
	if (!t.std.signal_change_chan) {
	  fprintf (stderr, "ERROR: missing chan signal change in library %s\n",
		   dl ? dl : tmpdl);
	  if (tmpdl) {
	    free (tmpdl);
	  }
	  dlclose (dlib);
	  return NULL;
	}
	if (!t.std.signal_change_wide_chan) {
	  fprintf (stderr, "WARNING: missing wide chan signal change in library %s\n",
		   dl ? dl : tmpdl);
	}
      }
    }
    if (t.create_tracefile_alt) {
      if (t.add_analog_signal && !t.alt.signal_change_analog) {
	fprintf (stderr, "ERROR: missing alt analog signal change in library %s\n",
		 dl ? dl : tmpdl);
	if (tmpdl) {
	  free (tmpdl);
	}
	dlclose (dlib);
	return NULL;
      }
      if (t.add_digital_signal || t.add_int_signal) {
	if (!t.alt.signal_change_digital) {
	  fprintf (stderr, "ERROR: missing alt digital signal change in library %s\n",
		   dl ? dl : tmpdl);
	  if (tmpdl) {
	    free (tmpdl);
	  }
	  dlclose (dlib);
	  return NULL;
	}
	if (t.add_int_signal && !t.alt.signal_change_wide_digital) {
	  fprintf (stderr, "WARNING: missing alt wide digital signal change in library %s\n",
		   dl ? dl : tmpdl);
	}
      }
      if (t.add_chan_signal) {
	if (!t.alt.signal_change_chan) {
	  fprintf (stderr, "ERROR: missing alt chan signal change in library %s\n",
		   dl ? dl : tmpdl);
	  if (tmpdl) {
	    free (tmpdl);
	  }
	  dlclose (dlib);
	  return NULL;
	}
	if (!t.alt.signal_change_wide_chan) {
	  fprintf (stderr, "WARNING: missing alt wide chan signal change in library %s\n",
		   dl ? dl : tmpdl);
	}
      }
    }
  }
  
  NEW (fn, act_extern_trace_func_t);
  *fn = t;
  fn->dlib = dlib;

  if (tmpdl) {
    free (tmpdl);
  }
  return fn;
}

void act_trace_close_format (act_extern_trace_func_t *fmt)
{
  if (!fmt) return;
  dlclose (fmt->dlib);
  free (fmt);
}

int act_trace_has_alt (act_extern_trace_func_t *tlib)
{
  if (!tlib) {
    return 0;
  }
  if (tlib->create_tracefile_alt) {
    return 1;
  }
  return 0;
}

act_trace_t *act_trace_create (act_extern_trace_func_t *tlib,
			       const char *name,
			       float stop_time,
			       float dt,
			       int mode)
{
  act_trace_t *t;

  if (!tlib) {
    return NULL;
  }

  if (!tlib->has_writer) {
    fprintf (stderr, "act_trace_create: library is missing writer API!\n");
    return NULL;
  }

  NEW (t, act_trace_t);

  t->state = 0;
  t->t = tlib;
  t->handle = NULL;
  t->readonly = 0;

  if (mode == 0) {
    if (tlib->create_tracefile) {
      t->handle = (*tlib->create_tracefile) (name, stop_time, dt);
    }
    t->mode = 0;
  }
  else {
    if (tlib->create_tracefile_alt) {
      t->handle = (*tlib->create_tracefile_alt) (name, stop_time, dt);
    }
    t->mode = 1;
  }
  
  if (!t->handle) {
    free (t);
    return NULL;
  }
  t->state = 0;
  return t;
}
			       
void *act_trace_add_signal (act_trace_t *t, act_signal_type_t type,
			    const char *s, int width)
{
  if (!t) {
    return NULL;
  }
  if (t->readonly) {
    fprintf (stderr, "WARNING: act_trace_add_signal() called while reading\n");
    return NULL;
  }

  if (t->state == 0) {
    t->state = 1;
    (*t->t->add_signal_start) (t->handle);
  }
  
  if (t->state != 1) {
    fprintf (stderr, "ERROR; act_trace_add_signal() in illegal state (%d)\n",
	     t->state);
    return NULL;
  }
  
  switch (type) {
  case ACT_SIG_BOOL:
    if (t->t->add_digital_signal) {
      return (*t->t->add_digital_signal) (t->handle, s);
    }
    break;

  case ACT_SIG_INT:
    if (t->t->add_int_signal) {
      return (*t->t->add_int_signal) (t->handle, s, width);
    }
    break;

  case ACT_SIG_CHAN:
    if (t->t->add_chan_signal) {
      return (*t->t->add_chan_signal) (t->handle, s, width);
    }
    break;

  case ACT_SIG_ANALOG:
    if (t->t->add_analog_signal) {
      return (*t->t->add_analog_signal) (t->handle, s);
    }
    break;

  default:
    fprintf (stderr, "Unknown signal type!\n");
    break;
  }
  return NULL;
}


int act_trace_init_start (act_trace_t *t)
{
  if (!t) return 0;
  if (t->readonly) {
    fprintf (stderr, "WARNING: act_trace_init_start() called while reading\n");
    return 0;
  }
  
  if (t->state == 0) {
    fprintf (stderr, "WARNING: no signals added?\n");
    t->state = 1;
    (*t->t->add_signal_start) (t->handle);
  }
  
  if (t->state == 1) {
    t->state = 2;
    (*t->t->add_signal_end) (t->handle);
  }
  else {
    fprintf (stderr, "ERROR: initial block in illegal state (%d)\n", t->state);
    return 0;
  }

  return (*t->t->init_start) (t->handle);
}

int act_trace_init_end (act_trace_t *t)
{
  if (!t) return 0;
  if (t->readonly) {
    fprintf (stderr, "WARNING: act_trace_init_end() called while reading\n");
    return 0;
  }
  
  if (t->state == 2) {
    fprintf (stderr, "WARNING: no initial signals?\n");
    t->state = 3;
  }
  
  if (t->state == 3) {
    t->state = 4;
  }
  else {
    fprintf (stderr, "ERROR: initial block end in illegal state (%d)\n", t->state);
    return 0;
  }

  return (*t->t->init_end) (t->handle);
}

int act_trace_close (act_trace_t *t)
{
  int ret;
  if (!t) return 0;

  if (t->state == 3 || t->state == 4) {
    fprintf (stderr, "WARNING: empty trace file?\n");
    t->state = 5;
  }
  if (t->state != 5) {
    fprintf (stderr, "ERROR: close called in illegal state (%d)\n",
	     t->state);
    return 0;
  }
  ret = (*t->t->close_tracefile) (t->handle);
  free (t);
  return ret;
}


int act_trace_analog_change (act_trace_t *t, void *node, float tm, float v)
{
  if (t->mode != 0) {
    return 0;
  }
  if (t->readonly) {
    fprintf (stderr, "WARNING: act_trace_analog_change() called while reading\n");
    return 0;
  }

  if (t->state == 2) {
    t->state = 3;
  }
  else if (t->state == 4) {
    t->state = 5;
  }
  if (t->state != 3 && t->state != 5) {
    fprintf (stderr, "ERROR: signal change in illegal state (%d)\n",
	     t->state);
    return 0;
  }

  if (t->t->std.signal_change_analog) {
    return (*t->t->std.signal_change_analog) (t->handle, node, tm, v);
  }
  return 0;
}

int act_trace_digital_change (act_trace_t *t, void *node, float tm,
			      unsigned long v)
{
  if (t->mode != 0) {
    return 0;
  }
  if (t->readonly) {
    fprintf (stderr, "WARNING: act_trace_digital_change() called while reading\n");
    return 0;
  }

  if (t->state == 2) {
    t->state = 3;
  }
  else if (t->state == 4) {
    t->state = 5;
  }
  if (t->state != 3 && t->state != 5) {
    fprintf (stderr, "ERROR: signal change in illegal state (%d)\n",
	     t->state);
    return 0;
  }
  
  if (t->t->std.signal_change_digital) {
    return (*t->t->std.signal_change_digital) (t->handle, node, tm, v);
  }
  
  return 0;
}
				    
int act_trace_wide_digital_change (act_trace_t *t, void *node, float tm,
				   int len,
				   unsigned long *v)
{
  if (t->mode != 0) {
    return 0;
  }
  if (t->readonly) {
    fprintf (stderr, "WARNING: act_trace_wide_digital_change() called while reading\n");
    return 0;
  }

  if (t->state == 2) {
    t->state = 3;
  }
  else if (t->state == 4) {
    t->state = 5;
  }
  if (t->state != 3 && t->state != 5) {
    fprintf (stderr, "ERROR: signal change in illegal state (%d)\n",
	     t->state);
    return 0;
  }
  
  if (t->t->std.signal_change_wide_digital) {
    return (*t->t->std.signal_change_wide_digital) (t->handle, node, tm, len,v);
  }
  
  return 0;
}


int act_trace_chan_change (act_trace_t *t, void *node, float tm,
			   act_chan_state_t s,
			   unsigned long v)
{
  if (t->mode != 0) {
    return 0;
  }
  if (t->readonly) {
    fprintf (stderr, "WARNING: act_trace_chan_change() called while reading\n");
    return 0;
  }
  
  if (t->state == 2) {
    t->state = 3;
  }
  else if (t->state == 4) {
    t->state = 5;
  }
  if (t->state != 3 && t->state != 5) {
    fprintf (stderr, "ERROR: signal change in illegal state (%d)\n",
	     t->state);
    return 0;
  }
  
  if (t->t->std.signal_change_chan) {
    return (*t->t->std.signal_change_chan) (t->handle, node, tm, s, v);
  }
  
  return 0;
}
				    
int act_trace_wide_chan_change (act_trace_t *t, void *node, float tm,
				act_chan_state_t s, int len,
				unsigned long *v)
{
  if (t->mode != 0) {
    return 0;
  }
  if (t->readonly) {
    fprintf (stderr, "WARNING: act_trace_wide_chan_change() called while reading\n");
    return 0;
  }

  if (t->state == 2) {
    t->state = 3;
  }
  else if (t->state == 4) {
    t->state = 5;
  }
  if (t->state != 3 && t->state != 5) {
    fprintf (stderr, "ERROR: signal change in illegal state (%d)\n",
	     t->state);
    return 0;
  }
  
  if (t->t->std.signal_change_wide_chan) {
    return (*t->t->std.signal_change_wide_chan) (t->handle, node, tm, s, len,v);
  }
  
  return 0;
}


int act_trace_analog_change_alt (act_trace_t *t, void *node,
				int len, unsigned long *tm, float v)
{
  if (t->mode == 0) {
    return 0;
  }
  if (t->readonly) {
    fprintf (stderr, "WARNING: act_trace_analog_change_alt() called while reading\n");
    return 0;
  }

  if (t->state == 2) {
    t->state = 3;
  }
  else if (t->state == 4) {
    t->state = 5;
  }
  if (t->state != 3 && t->state != 5) {
    fprintf (stderr, "ERROR: signal change in illegal state (%d)\n",
	     t->state);
    return 0;
  }

  if (t->t->alt.signal_change_analog) {
    return (*t->t->alt.signal_change_analog) (t->handle, node, len, tm, v);
  }
  
  return 0;
}

int act_trace_digital_change_alt (act_trace_t *t, void *node,
				 int len, unsigned long *tm, unsigned long v)
{
  if (t->mode == 0) {
    return 0;
  }
  if (t->readonly) {
    fprintf (stderr, "WARNING: act_trace_digital_change_alt() called while reading\n");
    return 0;
  }
  
  if (t->state == 2) {
    t->state = 3;
  }
  else if (t->state == 4) {
    t->state = 5;
  }
  if (t->state != 3 && t->state != 5) {
    fprintf (stderr, "ERROR: signal change in illegal state (%d)\n",
	     t->state);
    return 0;
  }
  
  if (t->t->alt.signal_change_digital) {
    return (*t->t->alt.signal_change_digital) (t->handle, node, len, tm, v);
  }
  
  return 0;
}
				    
int act_trace_wide_digital_change_alt (act_trace_t *t, void *node,
				       int len, unsigned long *tm,
				       int lenv, unsigned long *v)
{
  if (t->mode == 0) {
    return 0;
  }
  if (t->readonly) {
    fprintf (stderr, "WARNING: act_trace_wide_digital_change_alt() called while reading\n");
    return 0;
  }
  
  if (t->state == 2) {
    t->state = 3;
  }
  else if (t->state == 4) {
    t->state = 5;
  }
  if (t->state != 3 && t->state != 5) {
    fprintf (stderr, "ERROR: signal change in illegal state (%d)\n",
	     t->state);
    return 0;
  }
  
  if (t->t->alt.signal_change_wide_digital) {
    return (*t->t->alt.signal_change_wide_digital) (t->handle, node, len, tm,
						    lenv, v);
  }
  
  return 0;
}

int act_trace_chan_change_alt (act_trace_t *t, void *node,
			       int len, unsigned long *tm,
			       act_chan_state_t s, unsigned long v)
{
  if (t->mode == 0) {
    return 0;
  }
  if (t->readonly) {
    fprintf (stderr, "WARNING: act_trace_chan_change_alt() called while reading\n");
    return 0;
  }
  
  if (t->state == 2) {
    t->state = 3;
  }
  else if (t->state == 4) {
    t->state = 5;
  }
  if (t->state != 3 && t->state != 5) {
    fprintf (stderr, "ERROR: signal change in illegal state (%d)\n",
	     t->state);
    return 0;
  }
  
  if (t->t->alt.signal_change_chan) {
    return (*t->t->alt.signal_change_chan) (t->handle, node, len, tm, s, v);
  }
  
  return 0;
}
				    
int act_trace_wide_chan_change_alt (act_trace_t *t, void *node,
				    int len, unsigned long *tm,
				    act_chan_state_t s, 
				    int lenv, unsigned long *v)
{
  if (t->mode == 0) {
    return 0;
  }
  if (t->readonly) {
    fprintf (stderr, "WARNING: act_trace_wide_chan_change_alt() called while reading\n");
    return 0;
  }
  
  if (t->state == 2) {
    t->state = 3;
  }
  else if (t->state == 4) {
    t->state = 5;
  }
  if (t->state != 3 && t->state != 5) {
    fprintf (stderr, "ERROR: signal change in illegal state (%d)\n",
	     t->state);
    return 0;
  }
  
  if (t->t->alt.signal_change_wide_chan) {
    return (*t->t->alt.signal_change_wide_chan) (t->handle, node, len, tm,
						 s, lenv, v);
  }
  
  return 0;
}


act_trace_t *act_trace_open (act_extern_trace_func_t *tlib,
			     const char *name,
			     int mode)
{
  act_trace_t *t;

  if (!tlib) {
    return NULL;
  }

  if (!tlib->has_reader) {
    fprintf (stderr, "act_trace_create: library is missing reader API!\n");
    return NULL;
  }

  NEW (t, act_trace_t);

  t->state = 0;
  t->t = tlib;
  t->handle = NULL;
  t->readonly = 1;

  if (mode == 0) {
    if (tlib->open_tracefile) {
      t->handle = (*tlib->open_tracefile) (name);
    }
    t->mode = 0;
  }
  else {
    if (tlib->open_tracefile_alt) {
      t->handle = (*tlib->open_tracefile_alt) (name);
    }
    t->mode = 1;
  }
  
  if (!t->handle) {
    free (t);
    return NULL;
  }
  t->state = 5;
  return t;
}

void act_trace_header (act_trace_t *t, float *stop_time, float *dt)
{
  *stop_time = -1;
  *dt = -1;
  if (!t) {
    return;
  }
  if (!t->readonly) {
    fprintf (stderr, "WARNING: act_trace_header() called while writing\n");
    return;
  }
  (*t->t->read_header) (t->handle, stop_time, dt);
  return;
}

void *act_trace_lookup (act_trace_t *t, const char *name)
{
  if (!t) {
    return NULL;
  }
  if (!t->readonly) {
    fprintf (stderr, "WARNING: act_trace_lookup() called while writing\n");
    return NULL;
  }
  return (*t->t->signal_lookup) (t->handle, name);
}

act_signal_type_t act_trace_sigtype (act_trace_t *t, void *sig)
{
  if (!t) {
    return ACT_SIG_BOOL;
  }
  if (!t->readonly) {
    fprintf (stderr, "WARNING: act_trace_sigtype() called while writing\n");
    return ACT_SIG_BOOL;
  }
  return (*t->t->signal_type) (t->handle, sig);
}

void act_trace_advance_steps (act_trace_t *t, int steps)
{
  if (!t) {
    return;
  }
  if (!t->readonly) {
    fprintf (stderr, "WARNING: act_trace_advance_steps() called while writing\n");
    return;
  }
  (*t->t->advance_time) (t->handle, steps);
}

void act_trace_advance_time (act_trace_t *t, float dt)
{
  if (!t) {
    return;
  }
  if (!t->readonly) {
    fprintf (stderr, "WARNING: act_trace_advance_time() called while writing\n");
    return;
  }
  (*t->t->advance_time_by) (t->handle, dt);
}

int act_trace_has_more_data (act_trace_t *t)
{
  if (!t) {
    return 0;
  }
  if (!t->readonly) {
    fprintf (stderr, "WARNING: act_trace_has_more_data() called while writing\n");
    return 0;
  }
  return (*t->t->has_more_data) (t->handle);
}

act_signal_val_t act_trace_get_signal (act_trace_t *t, void *sig)
{
  act_signal_val_t v;
  v.v = 0;
  if (!t) {
    return v;
  }
  if (!t->readonly) {
    fprintf (stderr, "WARNING: act_trace_has_more_data() called while writing\n");
    return v;
  }
  return (t->t->get_signal) (t->handle, sig);
}

unsigned long act_trace_get_smallval (act_trace_t *t, void *sig)
{
  act_signal_val_t v = act_trace_get_signal (t, sig);
  return v.val;
}


float act_trace_get_analog (act_trace_t *t, void *sig)
{
  act_signal_val_t v = act_trace_get_signal (t, sig);
  return v.v;
}
