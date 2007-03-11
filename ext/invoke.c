#include "ruby_orbit.h"

const int ARG_POOL_SIZE = 256;

static gpointer allocate_retval_in_pool(ORBit_IMethod* method, char *pool, int* pool_pos) {
	CORBA_TypeCode tc;
	tc = method->ret;
	if(!tc || tc->kind == CORBA_tk_void || tc->kind == CORBA_tk_null) {
		return NULL;	
	}
	*pool_pos += tc->c_length;
	return pool;
}

VALUE corba_object_invoke_method(int argc, VALUE *argv, VALUE self) {
	if(argc < 1) {
		rb_raise(eCorbaError, "Method name must be first argument");
	}
	VALUE method_name = rb_funcall(argv[0], rb_intern("to_s"), 0);
	argc--;
	argv++;
	ORBit_IMethod* method = object_get_method(self, STR(method_name));
	if(!method) {
		rb_raise(rb_eNoMethodError, "undefined method `%s` for %s class", STR(method_name), get_object_type_id(self));
	}
	if(argc < method->arguments._length) {
		rb_raise(rb_eArgError, "wrong number of arguments (%d for %d)", argc, method->arguments._length);
	}
	
	gpointer _args[method->arguments._length];
	char pool[ARG_POOL_SIZE];
	int pool_pos = 0;
	gpointer retval = allocate_retval_in_pool(method, pool, &pool_pos);
	char* pool_of_args = pool+pool_pos;
	object_marshall_arguments(method, argc, argv, _args, pool, &pool_pos);
	ORBit_small_invoke_stub (DATA_PTR(self), method, retval, _args, NULL,	&ruby_orbit2_ev);

	object_unmarshall_outvalues(method, argc, argv, _args, pool_of_args);
	return object_unmarshall(method->ret, retval);
}
