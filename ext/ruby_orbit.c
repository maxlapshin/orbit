#include "ruby_orbit.h"

VALUE mORBit2, cCorbaObject, eCorbaError, mOrphan, cLong, cBool;
CORBA_Environment ruby_orbit2_ev;
CORBA_ORB ruby_orbit2_orb;



static VALUE corba_object_allocate(VALUE klass) {
	return Data_Wrap_Struct(klass, 0, corba_object_free, 0);
}


static VALUE corba_object_from_ior(VALUE klass, VALUE ior) {
	Check_Type(ior, T_STRING);
	return create_ruby_corba_object(CORBA_ORB_string_to_object(ruby_orbit2_orb, STR(ior), &ruby_orbit2_ev));
}

void long_free(void *l){}
static VALUE long_allocate(VALUE klass) {
	return Data_Wrap_Struct(klass, 0, long_free, 0);
}
static VALUE long_initialize(VALUE self, VALUE i) {
	if(T_FIXNUM == TYPE(i)) {
		DATA_PTR(self) = (void *)NUM2INT(i);
	} else if(T_BIGNUM == TYPE(i)) {
		long long ll = rb_big2ll(i);
		DATA_PTR(self) = (long)ll;
	} else if(cLong == rb_class_of(i)) {
		DATA_PTR(self) = DATA_PTR(i);
	} else {
		rb_raise(rb_eTypeError, "wrong type argument %s (expected Fixnum, Bignum or ORBit2::Long)", rb_obj_classname(i));
	}
	return self;
}
static VALUE long_to_i(VALUE self) {
	return INT2NUM((long)DATA_PTR(self));
}

static VALUE long_equal(VALUE self, VALUE l) {
	if(T_FIXNUM == TYPE(l)) {
		return NUM2INT(l) == (long)DATA_PTR(self) ? Qtrue : Qfalse;
	}
	if(T_BIGNUM == TYPE(l)) {
		long long ll = rb_big2ll(l);
		return ll == (long long)(long)DATA_PTR(self) ? Qtrue : Qfalse;
	}
	if(T_FLOAT == TYPE(l)) {
		return (long)DATA_PTR(self) == (long)RFLOAT(l)->value ? Qtrue : Qfalse;
	}
	if(cLong == rb_class_of(l)) {
		return DATA_PTR(self) == DATA_PTR(l) ? Qtrue : Qfalse;
	}
	return Qfalse;
}

gpointer allocate_in_pool(char *pool, int* pool_pos, int size) {
	*pool_pos += size;
	return pool + *pool_pos - size;
}

void Init_ruby_orbit() {
	int argc = 0;
	char *argv[1];
	argv[0] = 0;
	
	//ruby_objects = g_hash_table_new(g_direct_hash, g_direct_equal);
	mORBit2 = rb_define_module("ORBit2");
	
	eCorbaError = rb_define_class_under(mORBit2, "CorbaError", rb_eStandardError);
	
	cCorbaObject = rb_define_class_under(mORBit2, "CorbaObject", rb_cObject);
	rb_define_alloc_func(cCorbaObject, corba_object_allocate);
	rb_define_singleton_method(cCorbaObject, "from_ior", corba_object_from_ior, 1);
	rb_define_method(cCorbaObject, "ior", corba_object_ior, 0);
	rb_define_method(cCorbaObject, "type_id", corba_object_type_id, 0);
	rb_define_method(cCorbaObject, "corba_methods", corba_object_methods, 0);
	rb_define_method(cCorbaObject, "describe_method", corba_object_describe_method, 1);
	rb_define_method(cCorbaObject, "invoke_method", corba_object_invoke_method, -1);
	rb_define_method(cCorbaObject, "method_missing", corba_object_invoke_method, -1);
	rb_define_method(cCorbaObject, "is_a?", corba_object_is_a, 1);
	
	CORBA_exception_init(&ruby_orbit2_ev);
	
	mOrphan = rb_define_module_under(mORBit2, "Orphan");
	rb_define_singleton_method(mOrphan, "lookup", orphan_lookup, 2);
	
	cLong = rb_define_class_under(mORBit2, "Long", rb_cObject);
	rb_define_alloc_func(cLong, long_allocate);
	rb_define_method(cLong, "initialize", long_initialize, 1);
	rb_define_method(cLong, "to_i", long_to_i, 0);
	rb_define_method(cLong, "to_int", long_to_i, 0);
	rb_define_method(cLong, "=", long_initialize, 1);
	rb_define_method(cLong, "==", long_equal, 1);
	rb_define_method(cLong, "replace", long_initialize, 1);
	
	ruby_orbit2_orb = CORBA_ORB_init(&argc, argv, "orbit-local-orb", &ruby_orbit2_ev);
}

