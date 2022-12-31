/*
 *
 * Functions that have to be provided
 *
 */
#include <common/atrace.h>

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

int atr_close (void *handle)
{
  atrace_close ((atrace *)handle);
  return 1;
}
