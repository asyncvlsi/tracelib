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
#include <time.h>
#include <math.h>
#include <ctype.h>
#include <common/hash.h>
#include <common/misc.h>
#include <common/int.h>
#include "trace.h"


namespace {
// hide this part

static const char **_strings;

static int _vcd_group_signals (char *a, char *b)
{
  int imode;
  const char *as = _strings[*((int *)a)];
  const char *bs = _strings[*((int *)b)];
  long ia, ib;

  /* string compare: normal strcmp, but when you are in integer mode,
     reverse the integers */
  imode = 0;
  ia = 0;
  ib = 0;
  while (*as && *bs) {
    if (imode == 0) {
      if (*as != *bs) {
	return *as - *bs;
      }
      if (*as == '[') {
	imode = 1;
	ia = 0;
	ib = 0;
      }
    }
    else {
      if (*as != *bs) {
	while (*as && isdigit (*as)) {
	  ia = ia*10 + (*as - '0');
	  as++;
	}
	while (*bs && isdigit (*bs)) {
	  ib = ib*10 + (*bs - '0');
	  bs++;
	}
	return ib - ia;
      }
      else {
	if (isdigit (*as)) {
	  ia = ia*10 + (*as - '0');
	  ib = ib*10 + (*bs - '0');
	}
	else {
	  imode = 0;
	}
      }
    }
    as++;
    bs++;
  }
  return *as - *bs;
}


class VCDInfo {
 public:
  VCDInfo (FILE *fp, float ts, int mode = 0) {
    _fp = fp;
    _ts = ts;
    _in_dump = 0;
    _last_time = -1;
    _nm_len = 0;
    _nm_max = 0;
    _idxmap = 0;
    _type = NULL;
    _mode = mode;
    _last_itime = 0;
  }
  
  ~VCDInfo () {
    fclose (_fp);
    _fp = NULL;
    if (_idxmap) {
      FREE (_idxmap);
    }
    if (_nm_max > 0) {
      FREE (_nm)
;
    }
    if (_type) {
      FREE (_type);
    }
  }

  void emitHeader() {
    time_t curtime = time (NULL);
    
    // emit VCD header 
    fprintf (_fp, "$date\n");
    fprintf (_fp, "   %s\n", ctime (&curtime));
    fprintf (_fp, "$end\n");
    fprintf (_fp, "$version\n");
    fprintf (_fp, "   VCD generated by act trace library interface.\n");
    fprintf (_fp, "$end\n");
    fprintf (_fp, "$comment\n");
    fprintf (_fp, "   actual timescale is %g.\n", _ts);
    fprintf (_fp, "$end\n");
    fprintf (_fp, "$timescale ");

    double l10 = log10 (_ts);
    if (l10 < -14) {
      fprintf (_fp, "1 fs ");
      _ts = 1e-15;
    }
    else if (l10 < -13) {
      fprintf (_fp, "10 fs ");
      _ts = 10e-15;
    }
    else if (l10 < -12) {
      fprintf (_fp, "100 fs ");
      _ts = 100e-15;
    }
    else if (l10 < -11) {
      fprintf (_fp, "1 ps ");
      _ts = 1e-12;
    }
    else if (l10 < -10) {
      fprintf (_fp, "10 ps ");
      _ts = 10e-12;
    }
    else if (l10 < -9) {
      fprintf (_fp, "100 ps ");
      _ts = 100e-12;
    }
    else if (l10 < -8) {
      fprintf (_fp, "1 ns ");
      _ts = 1e-9;
    }
    else if (l10 < -7) {
      fprintf (_fp, "10 ns ");
      _ts = 10e-9;
    }
    else if (l10 < -6) {
      fprintf (_fp, "100 ns ");
      _ts = 100e-9;
    }
    else if (l10 < -5) {
      fprintf (_fp, "1 us ");
      _ts = 1e-6;
    }
    else if (l10 < -4) {
      fprintf (_fp, "10 us ");
      _ts = 10e-6;
    }
    else if (l10 < -3) {
      fprintf (_fp, "100 us ");
      _ts = 100e-6;
    }
    else if (l10 < -2) {
      fprintf (_fp, "1 ms ");
      _ts = 1e-3;
    }
    else if (l10 < -1) {
      fprintf (_fp, "10 ms ");
      _ts = 10e-3;
    }
    else if (l10 < 0) {
      fprintf (_fp, "100 ms ");
      _ts = 100e-3;
    }
    else {
      fprintf (_fp, "1 s ");
      _ts = 1;
    }
    fprintf (_fp, " $end\n");
    fprintf (_fp, "$scope module top $end\n");
  }

  void dumpStart () {
    // now sort and emit the variable names and short cuts
    if (_nm_len > 0) {
      _strings = _nm;
      MALLOC (_idxmap, int, _nm_len);
      for (int i=0; i < _nm_len; i++) {
	_idxmap[i] = i;
      }
      mygenmergesort ((char *)_idxmap, sizeof (int), _nm_len,
		      _vcd_group_signals);

      for (int i=0; i < _nm_len; i++) {
	int ix = _idxmap[i];
	fprintf (_fp, "$var %s %d %s %s $end\n",
		 _type[ix] < 0 ? "real" : "wire",
		 _type[ix] < 0 ? 1 : _type[ix],
		 _nm[ix],
		 _idx_to_char (ix));
      }
    }
    
    fprintf (_fp, "$upscope $end\n");
    fprintf (_fp, "$enddefinitions $end\n");
    fprintf (_fp, "$dumpvars\n");
    _in_dump = 1;
    _nm_max = 0;
    FREE (_nm);
  }

  void dumpEnd() {
    fprintf (_fp, "$end\n");
    _in_dump = 0;
  }

  int isInDump() { return _in_dump; }

  int addAnalog (const char *nm) {
    int idx = _nm_len;

    _appendName (nm, -1); // analog name
    // fprintf (_fp, "$var real 1 %s %s $end\n", _idx_to_char (idx), nm);
    return idx;
  }

  int addDigital (const char *nm, int w = 1) {
    int idx = _nm_len;
    _appendName (nm, w);
    // fprintf (_fp, "$var wire %d %s %s $end\n", w, _idxcount (idx), nm);
    return idx;
  }

  void emitTime (float t) {
    if (_mode == 0) {
      if (t == _last_time) {
	return;
      }
      unsigned long tm = t/_ts;
      fprintf (_fp, "#%lu\n", tm);
      _last_time = t;
    }
  }

  void emitTime (BigInt t) {
    if (_mode != 0) {
      if (t == _last_itime) {
	return;
      }
      fprintf (_fp, "#");
      t.decPrint (_fp);
      fprintf (_fp, "\n");
      _last_itime = t;
    }
  }

  void emitDigital (int idx, unsigned long v) {
    BigInt tmp;
    tmp = v;
    if (_type[idx] > 0) {
      tmp.setWidth (_type[idx]);
    }
    fprintf (_fp, "b");
    tmp.bitPrint (_fp);
    fprintf (_fp, " %s\n", _idx_to_char (idx));
  }

  void emitDigital (int idx, int len, unsigned long *v) {
    BigInt tmp;
    if (_type[idx] > 0) {
      tmp.setWidth (_type[idx]);
    }
    for (int i=0; i < len; i++) {
      if (i < tmp.getLen()) {
	tmp.setVal (i, v[i]);
      }
    }
    fprintf (_fp, "b");
    tmp.bitPrint (_fp);
    fprintf (_fp, " %s\n", _idx_to_char (idx));
  }    

  void emitAnalog (int idx, float v) {
    fprintf (_fp, "r%.16g %s\n", v, _idx_to_char (idx));
  }
  
 private:
  const char **_nm;
  int *_type;			// -1 = analog, otherwise digitai w
  int _nm_len;
  int _nm_max;
  
  int *_idxmap;
  
  FILE *_fp;
  float _ts;
  int _in_dump;
  int _mode;
  float _last_time;
  BigInt _last_itime;

  void _appendName (const char *nm, int t) {
    if (_nm_len == _nm_max) {
      if (_nm_max == 0) {
	_nm_max = 8;
	MALLOC (_nm, const char *, _nm_max);
	MALLOC (_type, int, _nm_max);
      }
      else {
	_nm_max = 2*_nm_max;
	REALLOC (_nm, const char *, _nm_max);
	REALLOC (_type, int, _nm_max);
      }
    }
    _nm[_nm_len] = nm;
    _type[_nm_len] = t;
    _nm_len++;
  }

  const  char *_idx_to_char (int idx) {
    const int start_code = 33;
    const int end_code = 126;
    static char buf[100];
    int pos = 0;

    do {
      Assert (pos < 99, "Shortcut is too long; increase static buffer size!");
      buf[pos] = (idx % (end_code - start_code + 1)) + start_code;
      idx = (idx - (idx % (end_code - start_code + 1)))/(end_code - start_code + 1);
      pos++;
    } while (idx > 0);
    buf[pos] = '\0';
    return buf;
  }

};

}

extern "C" {

void *vcd_create (const  char *nm, float stop_time, float ts)
{
  FILE *fp;
  VCDInfo *vi;

  fp = fopen (nm, "w");
  if (!fp) {
    fprintf (stderr, "ERROR: could not open file `%s' for writing\n", nm);
    return NULL;
  }
  vi = new VCDInfo (fp, ts);
  vi->emitHeader ();

  return vi;
}

void *vcd_create_alt (const  char *nm, float stop_time, float ts)
{
  FILE *fp;
  VCDInfo *vi;

  fp = fopen (nm, "w");
  if (!fp) {
    fprintf (stderr, "ERROR: could not open file `%s' for writing\n", nm);
    return NULL;
  }
  vi = new VCDInfo (fp, ts, 1);
  vi->emitHeader ();

  return vi;
}

int vcd_signal_start (void *handle)
{
  // nothing to do
  return 1;
}

void *vcd_add_analog_signal (void *handle, const char *s)
{
  VCDInfo *vi = (VCDInfo *)handle;
  int idx = vi->addAnalog (s);
  return (void *)((long)idx+1);
}

void *vcd_add_digital_signal (void *handle, const char *s)
{
  VCDInfo *vi = (VCDInfo *)handle;
  int idx = vi->addDigital (s);
  return (void *)((long)idx+1);
}

void *vcd_add_int_signal (void *handle, const char *s, int width)
{
  VCDInfo *vi = (VCDInfo *)handle;
  int idx = vi->addDigital (s, width);
  return (void *)((long)idx+1);
}

void *vcd_add_chan_signal (void *handle, const char *s, int width)
{
  VCDInfo *vi = (VCDInfo *)handle;
  int idx = vi->addDigital (s, ACT_TRACE_CHAN_WIDTH (width));
  return (void *)((long)idx+1);
}

int vcd_signal_end (void *handle)
{
  // nothing to do
  return 1;
}


int vcd_init_start (void *handle)
{
  VCDInfo *vi = (VCDInfo *)handle;
  vi->dumpStart ();
  return 1;
}

int vcd_init_end (void *handle)
{
  VCDInfo *vi = (VCDInfo *)handle;
  vi->dumpEnd ();
  
  return 1;
}

int vcd_change_digital (void *handle, void *node, float t, unsigned long v)
{
  VCDInfo *vi = (VCDInfo *)handle;

  if (!vi->isInDump()) {
    vi->emitTime (t);
  }
  vi->emitDigital (((unsigned long)node)-1, v);
  
  return 1;
}

int vcd_change_analog (void *handle, void *node, float t, float v)
{
  VCDInfo *vi = (VCDInfo *)handle;

  if (!vi->isInDump()) {
    vi->emitTime (t);
  }
  vi->emitAnalog (((unsigned long)node)-1, v);
  
  return 1;
}

int vcd_change_wide_digital (void *handle, void *node, float t,
			     int len, unsigned long *v)
{
  VCDInfo *vi = (VCDInfo *)handle;

  if (!vi->isInDump()) {
    vi->emitTime (t);
  }
  vi->emitDigital (((unsigned long)node)-1, len, v);
  
  return 0;
}


int vcd_change_digital_alt (void *handle, void *node, int len,
			    unsigned long *tm, unsigned long v)
{
  VCDInfo *vi = (VCDInfo *)handle;
  if (!vi->isInDump()) {
    BigInt tmp;
    tmp.setWidth (len * 8 * sizeof (unsigned long));
    for (int i=0; i < len; i++) {
      tmp.setVal (i, tm[i]);
    }
    vi->emitTime (tmp);
  }
  vi->emitDigital (((unsigned long)node)-1, v);
  
  return 1;
}

int vcd_change_analog_alt (void *handle, void *node, int len,
			   unsigned long *tm, float v)
{
  VCDInfo *vi = (VCDInfo *)handle;

  if (!vi->isInDump()) {
    BigInt tmp;
    tmp.setWidth (len * 8 * sizeof (unsigned long));
    for (int i=0; i < len; i++) {
      tmp.setVal (i, tm[i]);
    }
    vi->emitTime (tmp);
  }
  vi->emitAnalog (((unsigned long)node)-1, v);
  
  return 1;
}

int vcd_change_wide_digital_alt (void *handle, void *node, int len,
			     unsigned long *tm,
			     int lenv, unsigned long *v)
{
  VCDInfo *vi = (VCDInfo *)handle;

  if (!vi->isInDump()) {
    BigInt tmp;
    tmp.setWidth (len * 8 * sizeof (unsigned long));
    for (int i=0; i < len; i++) {
      tmp.setVal (i, tm[i]);
    }
    vi->emitTime (tmp);
  }
  vi->emitDigital (((unsigned long)node)-1, lenv, v);
  
  return 0;
}


int vcd_close (void *handle)
{
  VCDInfo *vi = (VCDInfo *)handle;
  delete vi;
  return 1;
}

}  
