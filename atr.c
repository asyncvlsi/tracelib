/*
 *
 * Functions that have to be provided
 *
 */
#include <common/atrace.h>
#include "tracelib.h"

void *atr_create (const  char *nm, float stop_time, float ts)
{
  return atrace_create (nm, ATRACE_DELTA, stop_time, ts);
}

int atr_signal_start (void *handle)
{
  return 1;
}

void *atr_add_analog_signal (void *handle, const char *s)
{
  name_t *nm;
  nm = atrace_create_node ((atrace *)handle, s);
  atrace_mk_analog (nm);
  return nm;
}

void *atr_add_digital_signal (void *handle, const char *s)
{
  name_t *nm;
  nm = atrace_create_node ((atrace *)handle, s);
  atrace_mk_digital (nm);
  return nm;
}

void *atr_add_int_signal (void *handle, const char *s, int width)
{
  name_t *nm;
  nm = atrace_create_node ((atrace *)handle, s);
  atrace_mk_digital (nm);
  atrace_mk_width (nm, width);
  return nm;
}

void *atr_add_chan_signal (void *handle, const char *s, int width)
{
  name_t *nm;
  nm = atrace_create_node ((atrace *)handle, s);
  atrace_mk_channel (nm);
  atrace_mk_width (nm, width);
  return nm;
}

int atr_signal_end (void *handle)
{
  return 1;
}


int atr_init_start (void *handle)
{
  return 1;
}

int atr_init_end (void *handle)
{
  return 1;
}

int atr_change_analog (void *handle, void *node, float t, float v)
{
  atrace_signal_change ((atrace *)handle, (name_t *)node, t, v);
  return 1;
}

int atr_change_digital (void *handle, void *node, float t, unsigned long v)
{
  atrace_val_t av;
  name_t *nm = (name_t *)node;
  av.val = v;
  atrace_general_change ((atrace *)handle, nm, t, &av);
  return 1;
}

int atr_change_wide_digital (void *handle, void *node, float t, int len, unsigned long *v)
{
  atrace_val_t av;
  name_t *nm = (name_t *)node;
  av.valp = v;
  atrace_general_change ((atrace *)handle, nm, t, &av);
  return 1;
}

int atr_change_chan (void *handle, void *node, float t,
		     act_chan_state_t s, unsigned long v)
{
  atrace_val_t av;
  name_t *nm = (name_t *)node;

  atrace_alloc_val_entry (nm, &av);
  if (ATRACE_WIDE_NODE(nm)) {
    for (int i=0; i < ATRACE_WIDE_NUM(nm); i++) {
      av.valp[i] = 0;
    }
    if (s == ACT_CHAN_SEND_BLOCKED) {
      av.valp[0] = ATRACE_CHAN_SEND_BLOCKED;
    }
    else if (s == ACT_CHAN_RECV_BLOCKED) {
      av.valp[0] = ATRACE_CHAN_RECV_BLOCKED;
    }
    else if (s == ACT_CHAN_IDLE) {
      av.valp[0] = ATRACE_CHAN_IDLE;
    }
    else {
      av.valp[0] = v + ATRACE_CHAN_VAL_OFFSET;
      if (av.valp[0] < v) {
	av.valp[1]++;
      }
    }
  }
  else {
    if (s == ACT_CHAN_VALUE) {
      av.val = v + ATRACE_CHAN_VAL_OFFSET;
    }
    else if (s == ACT_CHAN_SEND_BLOCKED) {
      av.val = ATRACE_CHAN_SEND_BLOCKED;
    }
    else if (s == ACT_CHAN_RECV_BLOCKED) {
      av.val = ATRACE_CHAN_RECV_BLOCKED;
    }
    else {
      av.val = ATRACE_CHAN_IDLE;
    }
  } 
  atrace_general_change ((atrace *)handle, nm, t, &av);
  atrace_free_val_entry (nm, &av);
  return 1;
}

int atr_change_wide_chan (void *handle, void *node, float t,
			  act_chan_state_t s, int len, unsigned long *v)
{
  atrace_val_t av;
  name_t *nm = (name_t *)node;
  
  atrace_alloc_val_entry (nm, &av);
  for (int i=0; i < ATRACE_WIDE_NUM(nm); i++) {
    av.valp[i] = 0;
  }

  if (s == ACT_CHAN_SEND_BLOCKED) {
    av.valp[0] = ATRACE_CHAN_SEND_BLOCKED;
  }
  else if (s == ACT_CHAN_RECV_BLOCKED) {
    av.valp[0] = ATRACE_CHAN_RECV_BLOCKED;
  }
  else if (s == ACT_CHAN_IDLE) {
    av.valp[0] = ATRACE_CHAN_IDLE;
  }
  else {
    int carry = ATRACE_CHAN_VAL_OFFSET;
    if (len > ATRACE_WIDE_NUM(nm)) {
      len = ATRACE_WIDE_NUM(nm);
    }
    for (int i=0; i < len; i++) {
      unsigned long oval = v[i];
      av.valp[i] = v[i] + carry;
      if (av.valp[i] < oval) {
	carry = 1;
      }
      else {
	carry = 0;
      }
    }
    if (carry) {
      av.valp[len]++;
    }
  }
  atrace_general_change ((atrace *)handle, nm, t, &av);
  atrace_free_val_entry (nm, &av);
  return 1;
}

int atr_close (void *handle)
{
  atrace_close ((atrace *)handle);
  return 1;
}


void *atr_open (const char *name)
{
  return atrace_open (name);
}

void atr_header (void *handle, float *stop_time, float *dt)
{
  int nnodes, nsteps, fmt, ts;
  atrace_header ((atrace *)handle, &ts, &nnodes, &nsteps, &fmt);
  *stop_time = ((atrace *)handle)->stop_time;
  *dt = ((atrace *)handle)->vdt;
}

void *atr_signal_lookup (void *handle, const char *name)
{
  return atrace_lookup ((atrace *)handle, name);
}

act_signal_type_t atr_signal_type (void *handle, void *sig)
{
  name_t *n = (name_t *)sig;
  if (n->type == 0) {
    return ACT_SIG_ANALOG;
  }
  else if (n->type == 1) {
    if (n->width == 1) {
      return ACT_SIG_BOOL;
    }
    else {
      return ACT_SIG_INT;
    }
  }
  else if (n->type == 2) {
    return ACT_SIG_CHAN;
  }
  else {
    /* other, report as integer */
    return ACT_SIG_INT;
  }
}  

void atr_advance_time (void *handle, int steps)
{
  atrace *a = (atrace *)handle;
  atrace_advance_time (a, steps);
}


void atr_advance_time_by (void *handle, float dt)
{
  atrace *a = (atrace *)handle;
  atrace_advance_time (a, (int)((dt+0.9*a->vdt)/(a->vdt)));
}

int atr_has_more_data (void *handle)
{
  return atrace_more_data ((atrace *)handle);
}

act_signal_val_t atr_get_signal (void *handle, void *sig)
{
  name_t *n = (name_t *)sig;
  act_signal_val_t ret;
  atrace_val_t v = ATRACE_GET_VAL (n);
  if (atrace_is_analog (n)) {
    ret.v = ATRACE_FLOATVAL (&v);
  }
  else if (atrace_is_digital (n) || atrace_is_channel (n)) {
    if (atrace_bitwidth (n) <= ATRACE_SHORT_WIDTH) {
      ret.val = ATRACE_SMALLVAL (&v);
    }
    else {
      ret.valp = ATRACE_BIGVAL (&v);
    }
  }
  return ret;
}
  
