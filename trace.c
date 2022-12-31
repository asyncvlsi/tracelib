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
#include <common/misc.h>
#include "trace.h"

act_extern_trace_func_t *act_trace_load_format (char *prefix, const char *dl)
{
  void *dlib;
  char *buf;
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
       { "signal_start", (void **)&t.add_signal_start, 1 },
       { "add_analog_signal", (void **)&t.add_analog_signal, 0 },
       { "add_digital_signal", (void **)&t.add_digital_signal, 0 },
       { "add_int_signal", (void **)&t.add_int_signal, 0 },
       { "add_chan_signal", (void **)&t.add_chan_signal, 0 },
       { "signal_end", (void **)&t.add_signal_end, 1 },

       /* initial block */
       { "init_start", (void **)&t.init_start, 1 },
       { "init_end", (void **)&t.init_end, 1 },

       /* floating-point time change */
       { "change_digital", (void **)&t.std.signal_change_digital, 0 },
       { "change_analog", (void **)&t.std.signal_change_analog, 0 },
       { "change_wide_digital", (void **) &t.std.signal_change_wide_digital, 0 },

       /* integer time change */
       { "change_digital_alt", (void **)&t.alt.signal_change_digital, 0 },
       { "change_analog_alt", (void **)&t.alt.signal_change_analog, 0 },
       { "change_wide_digital_alt", (void **) &t.alt.signal_change_wide_digital, 0 },

       /* close file */
       { "close", (void **) &t.close_tracefile, 1 },
       { NULL, NULL, 0 }
      };

  dlib = dlopen (dl, RTLD_LAZY);
  if (!dlib) {
    fprintf (stderr, "ERROR: failed to open `%s' as a trace library (prefix=%s)\n",
	     dl, prefix);
    return NULL;
  }

  l = strlen (prefix) + 32;
  MALLOC (buf, char, l);

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
	       buf, dl);
      err++;
    }
    else {
      *(fns[i].offset) = NULL;
    }
  }
  FREE (buf);
  if (err) {
    return NULL;
  }
  if (!t.create_tracefile && !t.create_tracefile_alt) {
    fprintf (stderr, "ERROR: missing create function in library %s\n", dl);
    return NULL;
  }
  
  NEW (fn, act_extern_trace_func_t);
  *fn = t;

  return fn;
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

  NEW (t, act_trace_t);

  t->state = 0;
  t->t = tlib;
  t->handle = NULL;

  if (mode == 0) {
    if (tlib->create_tracefile) {
      t->handle = (*tlib->create_tracefile) (name, stop_time, dt);
    }
  }
  else {
    if (tlib->create_tracefile_alt) {
      t->handle = (*tlib->create_tracefile_alt) (name, stop_time, dt);
    }
  }
  
  if (!t->handle) {
    FREE (t);
    return NULL;
  }
  t->mode = mode;
  t->state = 0;
  return t;
}
			       
void *act_trace_add_signal (act_trace_t *t, act_signal_type_t type,
			    const char *s, int width)
{
  if (!t) {
    return NULL;
  }

  if (t->state == 0) {
    t->state = 1;
  }
  
  if (t->state != 1) {
    fprintf (stderr, "ERROR; act_trace_add_signal() in illegal state (%d)\n",
	     t->state);
    return NULL;
  }
  
  switch (type) {
  case ACT_SIG_BOOL:
    if (t->t->add_digital_signal) {
      return (*t->t->add_analog_signal) (t->handle, s);
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


int act_trace_init_block (act_trace_t *t)
{
  if (!t) return 0;
  
  if (t->state == 0) {
    fprintf (stderr, "WARNING: no signals added?\n");
    t->state = 1;
  }
  
  if (t->state == 1) {
    t->state = 2;
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
  FREE (t);
  return ret;
}


int act_trace_analog_change (act_trace_t *t, void *node, float tm, float v)
{
  if (t->mode != 0) {
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


int act_trace_analog_change_alt (act_trace_t *t, void *node,
				int len, unsigned long *tm, float v)
{
  if (t->mode == 0) {
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
