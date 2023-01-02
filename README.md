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
** The `prefix` string is normally the name of the format. Functions in the shared object library that implement the necessary API start with `<prefix>_`.
** `dl` is the path to the shared object library. If omitted, the default name is used, which is `libtrace_<prefix>.so`. If the library was built as an ACT support library, the `$ACT_HOME/lib/` directory is checked for the library as well.
** If successful, a pointer to the trace file API is returned that is used to create a trace file.

* `act_trace_t *act_trace_create (act_extern_trace_func_t *, const char *name, float stop_time, float ts, int mode)`
** This creates the trace file with the specified name. It takes the trace file API as an argument, as well as the end time for the simulation trace and the time resolution. The mode argument can be zero or one; zero means that the trace file created uses the API where the time is specified as a floating-point number. If mode is one, the time is specified as an unsigned integer, where the time in SI units is obtained by multiplying the integer by `ts`. Note that a trace file API can support both interfaces, but a specific trace file can only use one of the two options.



