# tracelib

This provides an implementation of a simulation trace file writer interface.
Rather than providing support for a set of formats, the interface instead
assumes the existennce of a shared library that can be loaded in at run-time
that provides the appropriate file-format-specific functions for creating
a trace file.

Sample implementations for generating ACT trace files, VCD files, and LXT2
files are provided. A blank template file (`template.c`) is provided that
summarizes the functions that must be provided. Header file `tracelib.h`
provides details of the interface.

## Building

There are two ways to build the library.

* As an ACT support library: follow the instructions for building any ACT tool.
* As a standalone library without any dependencies on ACT: use `cmake` to build and install the library (`mkdir build; cd build; cmake -DCMAKE_INSTALL_PREFIX=<install-dir> ..; make && make install`)

## Usage

To begin using the interface, a shared object library must be loaded.

* `act_extern_trace_func_t *act_trace_load_format (char *prefix, const char *dl)`
  * The `prefix` string is normally the name of the format. Functions in the shared object library that implement the necessary API start with `<prefix>_`.
  * `dl` is the path to the shared object library. If omitted, the default name is used, which is `libtrace_<prefix>.so`. If the library was built as an ACT support library, the `$ACT_HOME/lib/` directory is checked for the library as well.
  * If successful, a pointer to the trace file API is returned that is used to create a trace file.

* `act_trace_t *act_trace_create (act_extern_trace_func_t *, const char *name, float stop_time, float ts, int mode)`
  * This creates the trace file with the specified name. It takes the trace file API as an argument, as well as the end time for the simulation trace and the time resolution.
  * The mode argument can be zero or one; zero means that the trace file created uses the API where the time is specified as a floating-point number. If mode is one, the time is specified as an unsigned integer, where the time in SI units is obtained by multiplying the integer by `ts`. Note that a trace file API can support both interfaces, but a specific trace file can only use one of the two options.

* `void *act_trace_add_signal (act_trace_t *,  act_signal_type_t type, const char *s, int width)`
  * This returns a signal handle that to be used when recording signal changes. It returns `NULL` on failure.
  * `nm` is the name of the signal, `type` (one of `ACT_SIG_BOOL`, `ACT_SIG_INT`, `ACT_SIG_CHAN`, `ACT_SIG_ANALOG`) specifies the signal type, and for channel and integer arguments the `width` is the bit-width of the data.

* `int act_trace_init_start (act_trace_t *)` and `int act_trace_init_end (act_trace_t *)`
  * This indicates the start of the block of initial values for signals. Signal initial values are recorded with time set to zero and the signal change API (below). 
  * The functions return 1 on success, 0 on failure.

* Signal changes for mode zero are recorded with the following API calls
  * `int act_trace_analog_change (act_trace_t *, void *node, float t, float v)`
  * `int act_trace_digital_change (act_trace_t *, void *node, float t, unsigned long v)`
  * `int act_trace_wide_digital_change (act_trace_t *, void *node, float t, int len, unsigned long *v)`
    * This call is used to support wider than 64-bit values recorded in the trace. `len` (more than one) is the size of the `v` array, where the least significant 64-bit chunk is in `v[0]`.
  * `int act_trace_chan_change (act_trace_t *, void *node, float t, act_chan_state_t s, unsigned long v)`
  * `int act_trace_wide_chan_change (act_trace_t *, void *node, float t, act_chan_state_t s, unsigned long v)`
    * These calls are similar to the digital change calls, except they also include a channel state parameter `s`. The state can be `ACT_CHAN_IDLE`, `ACT_CHAN_SEND_BLOCKED`, `ACT_CHAN_RECV_BLOCKED`, or `ACT_CHAN_VALUE`. The `v` field is only used when a `ACT_CHAN_VALUE` is specified.
  * All functions return 1 on success, 0 on failure. The `node` pointer is the handle returned when the signal was added.

* Signal changes for mode one are recoded with a similar API, except that the time is specified as an array of unsigned long values (similar to the wide integer/channel).
  * `int act_trace_analog_change_alt (act_trace_t *, void *node, int len, unsigned long *tm, float v)`
  * `int act_trace_digital_change_alt (act_trace_t *, void *node, int len, unsigned long *tm, unsigned long v)`
  * `int act_trace_wide_digital_change_alt (act_trace_t *, void *node, int len, unsigned long *tm, int lenv, unsigned long *v)`
  * `int act_trace_chan_change_alt (act_trace_t *, void *node, int len, unsigned long *tm, act_chan_state_t s, unsigned long v)`
  * `int act_trace_wide_chan_change_alt (act_trace_t *, void *node, int len, unsigned long *tm, act_chan_state_t s, int lenv, unsigned long *v)`
  * All functions return 1 on success, 0 on failure.

* `int act_trace_close (act_trace_t *)`
  * Closes the trace file and releases storage.

Finally, the API enforces a simple state machine in terms of the order in which these functions are to be called. The order must be:
1. Create trace file
2. Add signals
3. Start initial block
4. Add initial values
5. End initial block
6. Any signal changes for the rest of the simulation
7. Close trace file
