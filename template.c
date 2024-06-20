/*
 *
 * Functions that have to be provided
 *
 */


/** writer API functions **/

void *prefix_create (const  char *nm, float stop_time, float ts)
{
  return 0;
}

void *prefix_create_alt (const  char *nm, float stop_time, float ts)
{
  return 0;
}


int prefix_signal_start (void *handle)
{

  return 0;
}

void *prefix_add_analog_signal (void *handle, const char *s)
{
  return 0;
}

void *prefix_add_digital_signal (void *handle, const char *s)
{
  return 0;

}

void *prefix_add_int_signal (void *handle, const char *s, int width)
{
  return 0;

}

void *prefix_add_chan_signal (void *handle, const char *s, int width)
{
  return 0;

}

int prefix_signal_end (void *handle)
{
  return 0;
}


int prefix_init_start (void *handle)
{
  return 0;
}

int prefix_init_end (void *handle)
{
  return 0;
}

int prefix_change_digital (void *handle, void *node, float t, unsigned long v)
{
  return 0;
}

int prefix_change_analog (void *handle, void *node, float t, float v)
{
  return 0;
}

int prefix_change_wide_digital (void *handle, void *node, float t, int len, unsigned long *v)
{
  return 0;
}

int prefix_change_chan (void *handle, void *node, float t,
			act_chan_state_t s, unsigned long v)
{
  return 0;
}

int prefix_change_wide_chan (void *handle, void *node, float t,
			     act_chan_state_t s, int len, unsigned long *v)
{
  return 0;
}

int prefix_change_digital_alt (void *handle, void *node, int len, unsigned long *t, unsigned long v)
{
  return 0;
}

int prefix_change_analog_alt (void *handle, void *node, int len, unsigned long *t, float v)
{
  return 0;
}

int prefix_change_wide_digital_alt (void *handle, void *node, int len, unsigned long *t, int lenv, unsigned long *v)
{
  return 0;
}

int prefix_change_chan_alt (void *handle, void *node, int len, unsigned long *t,
			    act_chan_state_t s, unsigned long v)
{
  return 0;
}

int prefix_change_wide_chan_alt (void *handle, void *node,
				    int len, unsigned long *t,
				    act_chan_state_t s, int lenv, unsigned long *v)
{
  return 0;
}


/** reader API functions **/

void *prefix_open (const  char *nm)
{
  return NULL;
}

void *prefix_open_alt (const  char *nm)
{
  return NULL;
}


void prefix_header (void *handle, float *stop_time, float *dt)
{
  *stop_time = -1;
  *dt = -1;
  return;
}

void *prefix_signal_lookup (void *handle, const char *name)
{
  return NULL;
}

act_signal_type_t prefix_type (void *handle, void *signal)
{
  return ACT_SIG_BOOL;
}

act_signal_val_t prefix_get_signal (void *handle, void *signal)
{
  return ACT_SIG_BOOL_X;
}

void prefix_advance_time (void *handle, int steps)
{
  return;
}

void prefix_advance_time_by (void *handle, float dt)
{
  return;
}

int prefix_has_more_data (void *handle)
{
  return 0;
}

int prefix_close (void *handle)
{
  return 0;
}
