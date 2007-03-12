#ifndef _RUBY_ORBIT_H_
#define _RUBY_ORBIT_H_

#define ORBIT2_STUBS_API
#define ORBIT_IDL_C_COMMON
#define corba_defs_COMMON

#include <orbit/orbit.h>
#include <orbit/orbit-types.h>
#include <orbit/orb-core/orbit-small.h>
//#include <glib/ghash.h>
#include "ruby.h"
#include "rubyio.h"

extern VALUE mORBit2, cCorbaObject, eCorbaError, mOrphan, cLong, cBool;
extern CORBA_Environment ruby_orbit2_ev;
extern CORBA_ORB ruby_orbit2_orb;


char* get_object_type_id(VALUE self);
ORBit_IMethod* object_get_method(VALUE self, char *method_name);
VALUE create_ruby_corba_object(CORBA_Object obj);
void corba_object_free(CORBA_Object obj);
gpointer allocate_in_pool(char *pool, int* pool_pos, int size);
#define ALLOCATE(size) allocate_in_pool(pool, pool_pos, size)
#define ALLOCATE_FOR(type) ALLOCATE(sizeof(type))

void object_marshall_arguments(ORBit_IMethod* method, int argc, VALUE *argv, gpointer *args, char *pool, int* pool_pos);
void object_unmarshall_outvalues(ORBit_IMethod* method, int argc, VALUE *argv, gpointer *args, char *pool);
VALUE object_unmarshall(CORBA_TypeCode tc, gpointer retval);
VALUE corba_object_methods(VALUE self);
VALUE corba_object_type_id(VALUE self);
VALUE corba_object_ior(VALUE self);
VALUE corba_object_describe_method(VALUE self, VALUE _method_name);
VALUE corba_object_is_a(VALUE self, VALUE type);
VALUE orphan_create(VALUE self, VALUE klass_name_list);
VALUE corba_object_invoke_method(int argc, VALUE *argv, VALUE self);

#define STR(x) (RSTRING(x)->ptr)

#define CHECK_EXCEPTION(message, ...) do {\
	if (ruby_orbit2_ev._major != CORBA_NO_EXCEPTION) {\
		CORBA_exception_free (&ruby_orbit2_ev); \
		rb_raise(eCorbaError, message, ##__VA_ARGS__); \
	} \
}while(0)\

#endif /* _RUBY_ORBIT_H_ */