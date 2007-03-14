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

static void describe_type(VALUE description, CORBA_TypeCode tc) {
	if(!tc) {
		return;
	}
	if(tc->kind == CORBA_tk_struct) {
		int j;
		rb_str_cat2(description, tc->name);
		rb_str_cat2(description, "{");
		for(j = 0; j < tc->sub_parts; j++) {
			if(j > 0) {
				rb_str_cat2(description, ", ");
			}
			describe_type(description, tc->subtypes[j]);
			rb_str_cat2(description, tc->subnames[j]);
		}
		rb_str_cat2(description, "} ");
	} else if (tc->kind == CORBA_tk_sequence) {
		describe_type(description, tc->subtypes[0]);
		rb_str_cat2(description, "[] ");
	} else if(tc->name) {
		rb_str_cat2(description, tc->name);
		rb_str_cat2(description, " ");
	} else if (tc->kind == CORBA_tk_string) {
		rb_str_cat2(description, "string ");
	}
}

VALUE corba_object_describe_method(VALUE self, VALUE _method_name) {
	VALUE method_name = rb_funcall(_method_name, rb_intern("to_s"), 0);
	ORBit_IMethod* method = object_get_method(self, STR(method_name));
	if(!method) {
		return Qnil;
	}
	VALUE description = rb_str_new("", 0);
	describe_type(description, method->ret);
	rb_str_cat2(description, method->name);
	rb_str_cat2(description, "(");
	int i;
	for(i = 0; i < method->arguments._length; i++) {
		ORBit_IArg *arg = &method->arguments._buffer[i];
		CORBA_TypeCode  tc = arg->tc;

		while (tc->kind == CORBA_tk_alias)
			tc = tc->subtypes [0];
			
		if(i > 0) {
			rb_str_cat2(description, ", ");
		}
		if(arg->flags & ORBit_I_ARG_OUT) {
			rb_str_cat2(description, "out ");
		}
		if(arg->flags & ORBit_I_ARG_INOUT) {
			rb_str_cat2(description, "inout ");
		}
		describe_type(description, tc);
		rb_str_cat2(description, arg->name);
	}
	rb_str_cat2(description, ")");
	if(method->exceptions._length > 0) {
		rb_str_cat2(description, " throws ");
	}
	for(i = 0; i < method->exceptions._length; i++) {
		describe_type(description, method->exceptions._buffer[i]);
	}
	rb_str_cat2(description, ";");
	return description;
}

VALUE corba_object_is_a(VALUE self, VALUE type) {
	Check_Type(type, T_STRING);
	return CORBA_Object_is_a(DATA_PTR(self), STR(type), &ruby_orbit2_ev) ? Qtrue : Qfalse;
}


static VALUE orphan_lookup_find(VALUE where, VALUE klass_name_list, VALUE parent, int create) {
	int i;
	ID id;
	VALUE module = where;
	for(i = 0; i < RARRAY(klass_name_list)->len-1; i++) {
		id = rb_intern(STR(RARRAY(klass_name_list)->ptr[i]));
		if(rb_const_defined(module, id)) {
			module = rb_const_get(module, id);
		} else if (create) {
			module = rb_define_module_under(module, STR(RARRAY(klass_name_list)->ptr[i]));
		} else {
			return Qnil;
		}
	}
	id = rb_intern(STR(RARRAY(klass_name_list)->ptr[i]));
	if(rb_const_defined(module, id)) {
		return rb_const_get(module, id);
	}
	if(create) {
		return rb_define_class_under(module, STR(RARRAY(klass_name_list)->ptr[i]), parent);
	} else {
		return Qnil;
	}
}

VALUE orphan_lookup(VALUE self, VALUE klass_name_list, VALUE parent) {
	Check_Type(klass_name_list, T_ARRAY);
	VALUE klass = orphan_lookup_find(rb_cObject, klass_name_list, parent, 0);
	if(klass == Qnil) {
		klass = orphan_lookup_find(mOrphan, klass_name_list, parent, 1);
	}
	return klass;
}

