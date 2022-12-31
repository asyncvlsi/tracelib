/*
 *
 * Functions that have to be provided
 *
 */

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

int prefix_close (void *handle)
{
  return 0;
}
