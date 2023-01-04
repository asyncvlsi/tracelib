/*
 *
 * Functions that have to be provided
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "ext/lxt2_write.h"
#include "tracelib.h"


struct local_lxt2_state {
  float _last_time;
  float _ts;
  struct lxt2_wr_trace *f;
};
  

void *lxt2_create (const  char *nm, float stop_time, float ts)
{
  struct local_lxt2_state *st;
  struct lxt2_wr_trace *f;
  int i10;
  double l10;
  
  f = lxt2_wr_init (nm);
  l10 = log10 (ts);
  int il10 = (int)l10;
  if (il10 != l10) {
    il10--;
  }
  lxt2_wr_set_compression_depth (f, 4);
  lxt2_wr_set_break_size (f, 0);
  lxt2_wr_set_maxgranule (f, 8);
  lxt2_wr_set_timescale (f, il10);

  st = (struct local_lxt2_state *) malloc (sizeof (struct local_lxt2_state));
  if (!st) {
    fprintf (stderr, "Failed to allocate %lu bytes\n", sizeof (struct local_lxt2_state));
    exit (1);
  }
  st->f = f;
  st->_last_time = -1;
  st->_ts = 1;
  if (il10 >= 0) {
    while (il10 > 0) {
      st->_ts *= 10;
      il10--;
    }
  }
  else {
    while (il10 < 0) {
      st->_ts /= 10;
      il10++;
    }
  }
  return st;
}

int lxt2_signal_start (void *handle)
{
  return 1;
}

void *lxt2_add_analog_signal (void *handle, const char *s)
{
  struct local_lxt2_state *st = (struct local_lxt2_state *)handle;
  struct lxt2_wr_symbol *sym;

  sym = lxt2_wr_symbol_add (st->f, s, 0, 0, 0, LXT2_WR_SYM_F_DOUBLE);
    
  return sym;
}

void *lxt2_add_digital_signal (void *handle, const char *s)
{
  struct local_lxt2_state *st = (struct local_lxt2_state *)handle;
  struct lxt2_wr_symbol *sym;

  sym = lxt2_wr_symbol_add (st->f, s, 0, 0, 0, LXT2_WR_SYM_F_BITS);
  
  return sym;
}

void *lxt2_add_int_signal (void *handle, const char *s, int width)
{
  struct local_lxt2_state *st = (struct local_lxt2_state *)handle;
  struct lxt2_wr_symbol *sym;

  sym = lxt2_wr_symbol_add (st->f, s, 0, 0, width-1, LXT2_WR_SYM_F_BITS);
  
  return sym;

}

void *lxt2_add_chan_signal (void *handle, const char *s, int width)
{
  struct local_lxt2_state *st = (struct local_lxt2_state *)handle;
  return lxt2_add_int_signal (st->f, s, width);
}

int lxt2_signal_end (void *handle)
{
  return 1;
}


int lxt2_init_start (void *handle)
{
  return 1;
}

int lxt2_init_end (void *handle)
{
  return 1;
}



static char *_local_bits = NULL;
static int _iwidth = 0;
static char *_getbits (int width, unsigned long v)
{
  if (width < 0) {
    width = -width;
  }
  
  if (_iwidth <= width+1) {
    if (_iwidth == 0) {
      _iwidth = width + 1;
      if (_iwidth < 65) {
	_iwidth = 65;
      }
      _local_bits = (char *) malloc (_iwidth);
    }
    else {
      _iwidth = width + 32;
      _local_bits = (char *) realloc (_local_bits, _iwidth);
    }
    if (!_local_bits) {
      fprintf (stderr, "FATAL: could not allocate %d bytes\n", _iwidth);
      exit (1);
    }
  }

  if (width == 1) {
    if (v == ACT_SIG_BOOL_FALSE) {
      _local_bits[0] = '0';
    }
    else if (v == ACT_SIG_BOOL_TRUE) {
      _local_bits[0] = '1';
    }
    else if (v == ACT_SIG_BOOL_X) {
      _local_bits[0] = 'x';
    }
    else if (v == ACT_SIG_BOOL_Z) {
      _local_bits[0] = 'z';
    }
    _local_bits[1] = '\0';
  }
  else {
    int i;
    for (i = width-1; i >= 0; i--) {
      _local_bits[i] = '0' + (v & 1);
      v >>= 1;
    }
    _local_bits[width] = '\0';
  }
  return _local_bits;
}

static char *_getlongbits (int width, int len, unsigned long *v)
{
  int i;
  unsigned long val;
  int pos;

  if (width < 0) {
    width = -width;
  }
  
  if (_iwidth <= width+1) {
    if (_iwidth == 0) {
      _iwidth = width + 1;
      if (_iwidth < 65) {
	_iwidth = 65;
      }
      _local_bits = (char *) malloc (_iwidth);
    }
    else {
      _iwidth = width + 32;
      _local_bits = (char *) realloc (_local_bits, _iwidth);
    }
    if (!_local_bits) {
      fprintf (stderr, "FATAL: could not allocate %d bytes\n", _iwidth);
      exit (1);
    }
  }
  val = v[0];
  pos = 0;
  for (i = width-1; i >= 0; i--) {
    _local_bits[i] = '0' + (val & 1);
    val >>= 1;
    pos++;
    if (pos % 64 == 0) {
      if (pos/64 >= len) {
	val = 0;
      }
      else {
	val = v[pos/64];
      }
    }
  }
  _local_bits[width] = '\0';
  return _local_bits;
}


int lxt2_change_digital (void *handle, void *node, float t, unsigned long v)
{
  struct local_lxt2_state *st = (struct local_lxt2_state *)handle;
  struct lxt2_wr_symbol *s = (struct lxt2_wr_symbol *)node;

  if (st->_last_time != t) {
    lxt2_wr_set_time64 (st->f, (unsigned long) (t/st->_ts));
    st->_last_time = t;
  }

  lxt2_wr_emit_value_bit_string (st->f, s, 0, _getbits (s->msb - s->lsb + 1,v));

  return 1;
}

int lxt2_change_analog (void *handle, void *node, float t, float v)
{
  struct local_lxt2_state *st = (struct local_lxt2_state *)handle;
  
  if (st->_last_time != t) {
    lxt2_wr_set_time64 (st->f, (unsigned long) (t/st->_ts));
    st->_last_time = t;
  }
  
  lxt2_wr_emit_value_double (st->f, (struct lxt2_wr_symbol *) node, 0, v);
  
  return 1;
}

int lxt2_change_wide_digital (void *handle, void *node, float t, int len, unsigned long *v)
{
  struct local_lxt2_state *st = (struct local_lxt2_state *)handle;
  struct lxt2_wr_symbol *s = (struct lxt2_wr_symbol *)node;
  int i;
  char *bits;
  int pos;
  unsigned long val;

  if (st->_last_time != t) {
    lxt2_wr_set_time64 (st->f, (unsigned long) (t/st->_ts));
    st->_last_time = t;
  }
  
  lxt2_wr_emit_value_bit_string (st->f, s, 0, _getlongbits (s->msb - s->lsb + 1,
							    len, v));
    

  return 1;
}

int lxt2_change_chan (void *handle, void *node, float t,
		      act_chan_state_t state, unsigned long v)
{
  struct local_lxt2_state *st = (struct local_lxt2_state *)handle;
  struct lxt2_wr_symbol *s = (struct lxt2_wr_symbol *)node;

  if (st->_last_time != t) {
    lxt2_wr_set_time64 (st->f, (unsigned long) (t/st->_ts));
    st->_last_time = t;
  }
  if (state == ACT_CHAN_VALUE) {
    lxt2_wr_emit_value_bit_string (st->f, s, 0, _getbits (s->msb - s->lsb + 1,v));
  }
  else {
    char buf[4];
    if (s->msb - s->lsb + 1 >= 3) {
      if (state == ACT_CHAN_IDLE) {
	snprintf (buf, 4, "z00");
      }
      else if (state == ACT_CHAN_RECV_BLOCKED) {
	snprintf (buf, 4, "z01");
      }
      else {
	/* send blocked */
	snprintf (buf, 4, "z10");
      }
    }
    else {
      snprintf (buf, 4, "z");
    }
    lxt2_wr_emit_value_bit_string (st->f, s, 0, buf);
  }
  return 1;
}

int lxt2_change_wide_chan (void *handle, void *node, float t,
			   act_chan_state_t state,
			   int len, unsigned long *v)
{
  struct local_lxt2_state *st = (struct local_lxt2_state *)handle;
  struct lxt2_wr_symbol *s = (struct lxt2_wr_symbol *)node;
  int i;
  char *bits;
  int pos;
  unsigned long val;

  if (st->_last_time != t) {
    lxt2_wr_set_time64 (st->f, (unsigned long) (t/st->_ts));
    st->_last_time = t;
  }

  if (state == ACT_CHAN_VALUE) {
    lxt2_wr_emit_value_bit_string (st->f, s, 0, _getlongbits (s->msb - s->lsb + 1,
							      len, v));
  }
  else {
    char buf[4];
    if (state == ACT_CHAN_IDLE) {
      snprintf (buf, 4, "z00");
    }
    else if (state == ACT_CHAN_RECV_BLOCKED) {
      snprintf (buf, 4, "z01");
    }
    else {
      /* send blocked */
      snprintf (buf, 4, "z10");
    }
    lxt2_wr_emit_value_bit_string (st->f, s, 0, buf);
  }
  return 1;
}

int lxt2_close (void *handle)
{
  struct local_lxt2_state *st = (struct local_lxt2_state *)handle;
  lxt2_wr_close (st->f);
  free (st);
  return 1;
}
