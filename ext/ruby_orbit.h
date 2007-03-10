#ifndef _RUBY_ORBIT_H_
#define _RUBY_ORBIT_H_

#define ORBIT2_STUBS_API
#define ORBIT_IDL_C_COMMON
#define corba_defs_COMMON

#include <orbit/orbit.h>
#include <orbit/orbit-types.h>
#include <orbit/orb-core/orbit-small.h>
#include "ruby.h"
#include "rubyio.h"

extern VALUE mORBit2, cCorbaObject, eCorbaError;

#define STR(x) (RSTRING(x)->ptr)

#define CHECK_EXCEPTION(message, ...) do {\
	if (ruby_orbit2_ev._major != CORBA_NO_EXCEPTION) {\
		CORBA_exception_free (&ruby_orbit2_ev); \
		rb_raise(eCorbaError, message, ##__VA_ARGS__); \
	} \
}while(0)\

#endif /* _RUBY_ORBIT_H_ */