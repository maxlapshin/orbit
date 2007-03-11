#include "ruby_orbit.h"

VALUE corba_object_methods(VALUE self) {
	ORBit_IInterface* interface = ORBit_small_get_iinterface(DATA_PTR(self), get_object_type_id(self), &ruby_orbit2_ev);
	int i;
	VALUE methods = rb_ary_new2(interface->methods._length);
	for(i = 0; i < interface->methods._length; i++) {
		ORBit_IMethod method = interface->methods._buffer[i];
		rb_ary_push(methods, rb_str_new2(method.name));
	}
	return methods;
}


VALUE corba_object_type_id(VALUE self) {
	get_object_type_id(self);
	return rb_iv_get(self, "@type_id");
}

VALUE corba_object_ior(VALUE self) {
	if(!DATA_PTR(self) || !((ORBit_RootObject)(DATA_PTR(self)))->interface) {
		return Qnil;
	}
	char* ior = CORBA_ORB_object_to_string(ruby_orbit2_orb, DATA_PTR(self), &ruby_orbit2_ev);
	CHECK_EXCEPTION("Couldn't get IOR for object");
	VALUE retval = rb_str_new2(ior);
	CORBA_free(ior);
	return retval;
}

VALUE corba_object_describe_method(VALUE self, VALUE _method_name) {
	VALUE method_name = rb_funcall(_method_name, rb_intern("to_s"), 0);
	ORBit_IMethod* method = object_get_method(self, STR(method_name));
	if(!method) {
		return Qnil;
	}
	VALUE description = rb_str_new("", 0);
	rb_str_cat2(description, method->ret->name);
	rb_str_cat2(description, " ");
	rb_str_cat2(description, method->name);
	rb_str_cat2(description, "(");
	int i;
	for(i = 0; i < method->arguments._length; i++) {
		ORBit_IArg *arg = &method->arguments._buffer [i];
		CORBA_TypeCode  tc = arg->tc;

		while (tc->kind == CORBA_tk_alias)
			tc = tc->subtypes [0];
		if(i > 0) {
			rb_str_cat2(description, ", ");
		}
		rb_str_cat2(description, tc->name);
		rb_str_cat2(description, " ");
		rb_str_cat2(description, arg->name);
	}
	rb_str_cat2(description, ");");
	return description;
}

VALUE corba_object_is_a(VALUE self, VALUE type) {
	Check_Type(type, T_STRING);
	return CORBA_Object_is_a(DATA_PTR(self), STR(type), &ruby_orbit2_ev) ? Qtrue : Qfalse;
}

VALUE orphan_create(VALUE self, VALUE klass_name_list) {
	Check_Type(klass_name_list, T_ARRAY);
	int i;
	VALUE module = mOrphan;
	for(i = 0; i < RARRAY(klass_name_list)->len-2; i++) {
		module = rb_define_module_under(module, STR(RARRAY(klass_name_list)->ptr[i]));
	}
	return rb_define_class_under(module, STR(RARRAY(klass_name_list)->ptr[i]), cCorbaObject);
}

