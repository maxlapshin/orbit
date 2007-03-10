#include "ruby_orbit.h"

VALUE mORBit2, cCorbaObject, eCorbaError;
CORBA_Environment ruby_orbit2_ev;
CORBA_ORB ruby_orbit2_orb;

static ID to_s, to_i;

const int ARG_POOL_SIZE = 256;

static void corba_object_free(CORBA_Object* obj) {
	if(obj) {
		CORBA_Object_release(*obj, &ruby_orbit2_ev);
	}
}

static VALUE corba_object_allocate(VALUE klass) {
	return Data_Wrap_Struct(klass, 0, corba_object_free, 0);
}

static VALUE corba_object_initialize(VALUE self, VALUE ior) {
	Check_Type(ior, T_STRING);
	Check_Type(self, T_DATA);
	CORBA_Object obj_local = CORBA_ORB_string_to_object(ruby_orbit2_orb, STR(ior), &ruby_orbit2_ev);
	CHECK_EXCEPTION("cannot bind to object: %s", STR(ior));
	DATA_PTR(self) = malloc(sizeof(CORBA_Object));
	memcpy(DATA_PTR(self), &obj_local, sizeof(CORBA_Object));
	return self;
}

static VALUE corba_object_ior(VALUE self) {
	CORBA_Object *obj;
	Data_Get_Struct(self, CORBA_Object, obj);
	char* ior = CORBA_ORB_object_to_string(ruby_orbit2_orb, *obj, &ruby_orbit2_ev);
	CHECK_EXCEPTION("Couldn't get IOR for object");
	VALUE retval = rb_str_new2(ior);
	CORBA_free(ior);
	return retval;
}

static char* get_object_type_id(VALUE self) {
	VALUE retval;
	retval = rb_iv_get(self, "@type_id");
	if(retval == Qnil) {
		CORBA_Object *obj;
		Data_Get_Struct(self, CORBA_Object, obj);
		char* type_id = ORBit_small_get_type_id(*obj, &ruby_orbit2_ev);
		CHECK_EXCEPTION("Couldn't get type id of object");
		retval = rb_str_new2(type_id);
		CORBA_free(type_id);
		rb_iv_set(self, "@type_id", retval);
	}
	return STR(retval);
}

static VALUE corba_object_type_id(VALUE self) {
	get_object_type_id(self);
	return rb_iv_get(self, "@type_id");
}

static VALUE corba_object_interface(VALUE self) {
	CORBA_Object *obj;
	Data_Get_Struct(self, CORBA_Object, obj);
	ORBit_IInterface* interface = ORBit_small_get_iinterface(*obj, get_object_type_id(self), &ruby_orbit2_ev);
	int i;
	for(i = 0; i < interface->methods._length; i++) {
		ORBit_IMethod method = interface->methods._buffer[i];
		printf("Method: %s\n", method.name);
	}
	return Qnil;
}

static VALUE corba_object_methods(VALUE self) {
	CORBA_Object *obj;
	Data_Get_Struct(self, CORBA_Object, obj);
	ORBit_IInterface* interface = ORBit_small_get_iinterface(*obj, get_object_type_id(self), &ruby_orbit2_ev);
	int i;
	VALUE methods = rb_ary_new2(interface->methods._length);
	for(i = 0; i < interface->methods._length; i++) {
		ORBit_IMethod method = interface->methods._buffer[i];
		rb_ary_push(methods, rb_str_new2(method.name));
	}
	return methods;
}


static ORBit_IMethod* object_get_method(VALUE self, char *method_name) {
	CORBA_Object *obj;
	Data_Get_Struct(self, CORBA_Object, obj);
	ORBit_IInterface* interface = ORBit_small_get_iinterface(*obj, get_object_type_id(self), &ruby_orbit2_ev);
	int i = 0;
	int method_index = -1;
	for(i = 0; i < interface->methods._length; i++) {
		if(!strcmp(interface->methods._buffer[i].name, method_name)) {
			method_index = i;
			break;
		}
	}
	if(method_index == -1) {
		return NULL;
	}
	return &(interface->methods._buffer[i]);
}

static void object_marshall_arguments(ORBit_IMethod* method, int argc, VALUE *argv, gpointer *args, char **pool) {
	int i;
	int pool_size = ARG_POOL_SIZE;
	int pool_pos = 0;
	for(i = 0; i < method->arguments._length; i++) {
		ORBit_IArg *a = &method->arguments._buffer [i];
		CORBA_TypeCode  tc;
		tc = a->tc;

		while (tc->kind == CORBA_tk_alias)
			tc = tc->subtypes [0];
		
		switch (tc->kind) {
			case CORBA_tk_wchar:
			case CORBA_tk_wstring: {
				rb_raise(eCorbaError, "Wide char unsupported");
				break;
			}
			case CORBA_tk_char:
			case CORBA_tk_string: {
				args[i] = &STR(argv[i]);
				break;
			}
			case CORBA_tk_ushort:
			case CORBA_tk_short: {
				rb_raise(eCorbaError, "Short unsupported");
			}
			case CORBA_tk_long:
			case CORBA_tk_ulong: {
				rb_raise(eCorbaError, "Long unsupported");
			}
			case CORBA_tk_longlong:
			case CORBA_tk_ulonglong: {
				rb_raise(eCorbaError, "Long Long unsupported");
			}
			case CORBA_tk_double: {
				double **val = (double **)(*pool + pool_pos);
				pool_pos += sizeof(double *);
				*val = &(RFLOAT(argv[i])->value);
				args[i] = val;
				break;
			}
			case CORBA_tk_longdouble: {
				rb_raise(eCorbaError, "Long double unsupported");
			}
			case CORBA_tk_float: {
				rb_raise(eCorbaError, "Float unsupported");
			}
			
			
/*			
			

			case CORBA_tk_enum:
			giop_send_buffer_append_aligned (buf, *val, sizeof (CORBA_long));
			*val = ((guchar *)*val) + sizeof (CORBA_long);
			break;
		case CORBA_tk_boolean:
		case CORBA_tk_octet:
			giop_send_buffer_append (buf, *val, sizeof (CORBA_octet));
			*val = ((guchar *)*val) + sizeof (CORBA_octet);
			break;
		case CORBA_tk_any:
			ORBit_marshal_any (buf, *val);
			*val = ((guchar *)*val) + sizeof (CORBA_any);
			break;
		case CORBA_tk_Principal:
			ulval = *(CORBA_unsigned_long *) (*val);
			giop_send_buffer_append (buf, *val, sizeof (CORBA_unsigned_long));

			giop_send_buffer_append (buf,
						*(char **) ((char *)*val + sizeof (CORBA_unsigned_long)),
						ulval);
			*val = ((guchar *)*val) + sizeof (CORBA_Principal);
			break;
		case CORBA_tk_objref:
			ORBit_marshal_object (buf, *(CORBA_Object *)*val);
			*val = ((guchar *)*val) + sizeof (CORBA_Object);
			break;
		case CORBA_tk_TypeCode:
			ORBit_encode_CORBA_TypeCode (*(CORBA_TypeCode *)*val, buf);
			*val = ((guchar *)*val) + sizeof (CORBA_TypeCode);
			break;
		case CORBA_tk_except:
		case CORBA_tk_struct: {
			gconstpointer val0 = *val;
			int offset;
			for (i = offset = 0; i < tc->sub_parts; i++) {
				offset = ALIGN_VALUE (offset, tc->subtypes[i]->c_align);
				*val = val0 + offset;
				ORBit_marshal_value (buf, val, tc->subtypes[i]);
				offset += ORBit_gather_alloc_info (tc->subtypes[i]);
			}
			offset = ALIGN_VALUE (offset, tc->c_align);
			*val = val0 + offset;
			break;
		}
		case CORBA_tk_union: {
			gconstpointer   val0 = *val;
			gconstpointer	discrim, body;
			CORBA_TypeCode 	subtc;
			int             sz = 0;

			discrim = *val;
			ORBit_marshal_value (buf, val, tc->discriminator);

			subtc = ORBit_get_union_tag (tc, &discrim, FALSE);
			for (i = 0; i < tc->sub_parts; i++)
				sz = MAX (sz, ORBit_gather_alloc_info (tc->subtypes[i]));

			*val = val0 + ALIGN_VALUE (ORBit_gather_alloc_info (tc->discriminator),
						   tc->c_align);
			body = *val;
			ORBit_marshal_value (buf, &body, subtc);
			*val = *val + ALIGN_VALUE (sz, tc->c_align);
			break;
		}
		case CORBA_tk_sequence: {
			const CORBA_sequence_CORBA_octet *sval;
			sval = *val;
			giop_send_buffer_align (buf, sizeof (CORBA_unsigned_long));
			giop_send_buffer_append (buf, &sval->_length,
						 sizeof (CORBA_unsigned_long));

			subval = sval->_buffer;

			switch (tc->subtypes[0]->kind) {
			case CORBA_tk_boolean:
			case CORBA_tk_char:
			case CORBA_tk_octet:
				giop_send_buffer_append (buf, subval, sval->_length);
				break;
			default:
				for (i = 0; i < sval->_length; i++)
					ORBit_marshal_value (buf, &subval, tc->subtypes[0]);
				break;
			}
			*val = ((guchar *)*val) + sizeof (CORBA_sequence_CORBA_octet);
			break;
		}
		case CORBA_tk_array:
			switch (tc->subtypes[0]->kind) {
			case CORBA_tk_boolean:
			case CORBA_tk_char:
			case CORBA_tk_octet:
				giop_send_buffer_append (buf, *val, tc->length);
				*val = ((guchar *)*val) + tc->length;
				break;
			default:
		  		for (i = 0; i < tc->length; i++)
					ORBit_marshal_value (buf, val, tc->subtypes[0]);
				break;
			}
			break;
			giop_send_buffer_append_aligned (buf, *val, sizeof (CORBA_long_long));
			*val = ((guchar *)*val) + sizeof (CORBA_long_long);
			break;
		case CORBA_tk_fixed:
			g_error ("CORBA_fixed NYI");
			break;
		case CORBA_tk_null:
		case CORBA_tk_void:
			break;
		default:
			g_error ("Can't encode unknown type %d", tc->kind);
			break;
		*/
		}
	}
}

static VALUE corba_object_invoke_method(int argc, VALUE *argv, VALUE self) {
	CORBA_Object *obj;
	Data_Get_Struct(self, CORBA_Object, obj);
	if(argc < 1) {
		rb_raise(eCorbaError, "Method name must be first argument");
	}
	VALUE method_name = rb_funcall(argv[0], to_s, 0);
	ORBit_IMethod* method = object_get_method(self, STR(method_name));
	if(!method) {
		rb_raise(rb_eNoMethodError, "undefined method `%s` for %s class", STR(method_name), get_object_type_id(self));
	}
	if(argc < method->arguments._length) {
		rb_raise(rb_eArgError, "wrong number of arguments (%d for %d)", argc, method->arguments._length);
	}
	
	gpointer _args[method->arguments._length];
	char *pool = malloc(ARG_POOL_SIZE);
	object_marshall_arguments(method, argc - 1, argv + 1, _args, &pool);
	CORBA_Object retval;

	ORBit_small_invoke_stub (*obj, method, &retval, _args, NULL,	&ruby_orbit2_ev);
	return Qtrue;
}

static VALUE echo_string(VALUE self, VALUE astring, VALUE number) {
	CORBA_Object *obj;
	Check_Type(astring, T_STRING);
	Check_Type(number, T_FLOAT);
	Data_Get_Struct(self, CORBA_Object, obj);
	char method_name[] = "echoString";
	
	ORBit_IInterface* interface = ORBit_small_get_iinterface(*obj, get_object_type_id(self), &ruby_orbit2_ev);
	int i = 0;
	int method_index = -1;
	for(i = 0; i < interface->methods._length; i++) {
		if(!strcmp(interface->methods._buffer[i].name, method_name)) {
			method_index = i;
			break;
		}
	}
	
	gpointer _args[2];
	_args[0] = &STR(astring);
	double *val = &(RFLOAT(number)->value);
	_args[1] = &val;
	CORBA_Object retval;

	ORBit_small_invoke_stub (*obj, &(interface->methods._buffer[method_index]), &retval, _args, NULL,	&ruby_orbit2_ev);
	printf("Result: %f\n", RFLOAT(number)->value);
		
	return Qtrue;
}

void Init_ruby_orbit() {
	int argc = 0;
	char *argv[1];
	argv[0] = 0;
	
	to_s = rb_intern("to_s");
	to_i = rb_intern("to_i");
	mORBit2 = rb_define_module("ORBit2");
	
	eCorbaError = rb_define_class_under(mORBit2, "CorbaError", rb_eStandardError);
	
	cCorbaObject = rb_define_class_under(mORBit2, "CorbaObject", rb_cObject);
	rb_define_alloc_func(cCorbaObject, corba_object_allocate);
	rb_define_method(cCorbaObject, "initialize", corba_object_initialize, 1);
	rb_define_method(cCorbaObject, "ior", corba_object_ior, 0);
	rb_define_method(cCorbaObject, "type_id", corba_object_type_id, 0);
	rb_define_method(cCorbaObject, "interface", corba_object_interface, 0);
	rb_define_method(cCorbaObject, "corba_methods", corba_object_methods, 0);
	rb_define_method(cCorbaObject, "invoke_method", corba_object_invoke_method, -1);
	rb_define_method(cCorbaObject, "method_missing", corba_object_invoke_method, -1);
	rb_define_method(cCorbaObject, "echo", echo_string, 2);
	
	CORBA_exception_init(&ruby_orbit2_ev);
	
	ruby_orbit2_orb = CORBA_ORB_init(&argc, argv, "orbit-local-orb", &ruby_orbit2_ev);
}