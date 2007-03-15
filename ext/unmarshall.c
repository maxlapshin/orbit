#include "ruby_orbit.h"

static void unmarshall_struct(VALUE struc, CORBA_TypeCode tc, gpointer struct_raw) {
	int i;
	int offset = 0;;
	for(i = 0; i < tc->sub_parts; i++) {
		offset = ALIGN_VALUE (offset, tc->subtypes[i]->c_align);
		gpointer c_field = struct_raw + offset;
		//if(CORBA_tk_string == tc->subtypes[i]->kind) c_field = &c_field;
		VALUE field = object_unmarshall(tc->subtypes[i], c_field);
		char method_name[100];
		snprintf(method_name, sizeof(method_name), "%s=", tc->subnames[i]);
		rb_funcall(struc, rb_intern(method_name), 1, field);
		offset += ORBit_gather_alloc_info (tc->subtypes[i]);;
	}
}

void object_unmarshall_outvalues(ORBit_IMethod* method, int argc, VALUE *argv, gpointer *args, char *pool) {
	int i;
	for(i = 0; i < method->arguments._length; i++) {
		ORBit_IArg *a = &method->arguments._buffer [i];
		gpointer arg;
		
		if (!(a->flags & (ORBit_I_ARG_OUT | ORBit_I_ARG_INOUT))) {
			continue;
		}
		
		
		CORBA_TypeCode  tc;
		tc = a->tc;

		while (tc->kind == CORBA_tk_alias) {
			tc = tc->subtypes [0];
		}

		if (a->flags & ORBit_I_ARG_OUT)
			/* this may read (&discard) uninitialized memory,
			 * see 'foo' below. This is for simplicity. */
			arg = *(gpointer *)args [i];
		else
			arg = args [i];
			
		switch (tc->kind) {
			case CORBA_tk_wchar:
			case CORBA_tk_wstring: {
				rb_raise(eCorbaError, "Wide char demarshalling unsupported");
			}
			case CORBA_tk_string: {
				if(T_STRING == TYPE(argv[i])) {
					VALUE retval = rb_str_new2(*(char **)arg);
					rb_funcall(argv[i], rb_intern("replace"), 1, retval);
				}
				break;
			}
			case CORBA_tk_char:
			case CORBA_tk_octet: {
				if(cLong == rb_class_of(argv[i])) {
					DATA_PTR(argv[i]) = (void *)(long)*(char *)arg;
				}
				break;
			}
			case CORBA_tk_ushort: {
				if(cLong == rb_class_of(argv[i])) {
					DATA_PTR(argv[i]) = (void *)(long)*(unsigned short *)arg;
				}
				break;
			}
			case CORBA_tk_short: {
				if(cLong == rb_class_of(argv[i])) {
					DATA_PTR(argv[i]) = (void *)(long)*(short *)arg;
				}
				break;
			}
			case CORBA_tk_enum:
			case CORBA_tk_long:
			case CORBA_tk_ulong: {
				if(cLong == rb_class_of(argv[i])) {
					DATA_PTR(argv[i]) = (void *)*(long *)arg;
				}
				break;
			}
			case CORBA_tk_longlong:
			case CORBA_tk_ulonglong: {
				rb_raise(eCorbaError, "Long Long demarshalling unsupported");
			}
			case CORBA_tk_double: {
				RFLOAT(argv[i])->value = *(double *)arg;
				break;
			}
			case CORBA_tk_longdouble: {
				rb_raise(eCorbaError, "Long double demarshalling unsupported");
			}
			case CORBA_tk_float: {
				float val = *(float *)arg;
				RFLOAT(argv[i])->value = (double)val;
				break;
			}
			case CORBA_tk_null:
			case CORBA_tk_void: {
				break;
			}
			case CORBA_tk_any: {
				CORBA_any *encoded = arg;
				switch(encoded->_type->kind) {
					case CORBA_tk_long: {
						if(cLong == rb_class_of(argv[i])) {
							DATA_PTR(argv[i]) = (void *)*(long *)encoded->_value;
						}
						break;
					}
					case CORBA_tk_string: {
						VALUE newstr = rb_str_new2(*(char **)encoded->_value);
						rb_funcall(argv[i], rb_intern("replace"), 1, newstr);
						break;
					}
				}
				break;
			}
			case CORBA_tk_struct: {
				unmarshall_struct(argv[i], tc, arg);
				break;
			}
			case CORBA_tk_sequence: {
				Check_Type(argv[i], T_ARRAY);
				CORBA_sequence_CORBA_octet *sval = arg;
				int j;
				for(j = 0; j < sval->_length; j++) {
					rb_ary_store(argv[i], j, object_unmarshall(tc->subtypes[0], ((gpointer *)sval->_buffer)[j]));
				}
				break;
			}
/*			
			case CORBA_tk_Principal:
			case CORBA_tk_objref:
			case CORBA_tk_TypeCode:
			case CORBA_tk_except:
			case CORBA_tk_union:
			case CORBA_tk_boolean:
			case CORBA_tk_array:
			case CORBA_tk_fixed:
		*/
			default: {
				rb_warn("Don't know, how to demarshall %s. Marshalling nil", tc->name);
				argv[i] = Qnil;
				break;
			}
		}
	}
}

VALUE object_unmarshall(CORBA_TypeCode tc, gpointer retval) {
	if(!retval || !tc) {
		return Qnil;	
	}
	while (tc->kind == CORBA_tk_alias)
		tc = tc->subtypes[0];
	if(tc->kind == CORBA_tk_void || tc->kind == CORBA_tk_null) {
		return Qnil;	
	}
	switch(tc->kind) {
		case CORBA_tk_short:
		case CORBA_tk_ushort: {
			return INT2NUM(*(unsigned short *)retval);
		}
		case CORBA_tk_long: {
			return INT2NUM((long)retval);
		}
		case CORBA_tk_enum:
		case CORBA_tk_ulong: {
			return INT2NUM(*(unsigned long *)retval);
		}
		case CORBA_tk_longlong: {
			return rb_ll2inum(*(long long *)retval);
		}
		case CORBA_tk_ulonglong: {
			return rb_ull2inum(*(unsigned long long *)retval);
		}
		case CORBA_tk_longdouble: {
			return rb_float_new((double)(*(long double *)retval));
		}
		case CORBA_tk_float: {
			return rb_float_new((*(float *)retval));
		}
		case CORBA_tk_double: {
			return rb_float_new(*(double *)retval);
		}
		case CORBA_tk_boolean: {
			return *(char *)retval ? Qtrue : Qfalse;
		}
		case CORBA_tk_octet:
		case CORBA_tk_char: {
			return INT2FIX((unsigned int)*(unsigned char *)retval);
		}
		case CORBA_tk_any: {
			CORBA_any *decoded = (CORBA_any *)retval;
			return object_unmarshall(decoded->_type, *(gpointer *)decoded->_value);
		}
		case CORBA_tk_Principal: {
			rb_raise(eCorbaError, "CORBA_Principal is unknown type. Contact max@maxidoors.ru to include it's support");
		}
		case CORBA_tk_objref: {
			return create_ruby_corba_object(*(CORBA_Object *)retval);
		}
		case CORBA_tk_string: {
			//return rb_str_new2(retval);
			return rb_str_new2(*(char **)retval);
		}
		case CORBA_tk_sequence: {
			CORBA_sequence_CORBA_octet *sval = *(CORBA_sequence_CORBA_octet **)retval;
			VALUE value = rb_ary_new2(sval->_length);
			int i;
			for(i = 0; i < sval->_length; i++) {
				if(tc->subtypes[0]->kind == CORBA_tk_string) {
					rb_ary_push(value, rb_str_new2(((char **)sval->_buffer)[i]));
				} else {
					rb_ary_push(value, object_unmarshall(tc->subtypes[0], ((gpointer *)sval->_buffer)[i]));
				}
			}
			return value;
		}
		case CORBA_tk_void:
		case CORBA_tk_null: {
			return Qnil;
		}
		case CORBA_tk_fixed: {
			rb_raise(eCorbaError, "Nobody knows, how to unmarshall fixed");
		}
		case CORBA_tk_struct: {
			VALUE klass = rb_funcall(cCorbaObject, rb_intern("lookup!"), 1, rb_str_new2(tc->name));
			VALUE struc = rb_funcall(klass, rb_intern("new"), 0);
			unmarshall_struct(struc, tc, retval);
			return struc;
		}
		/*
		case CORBA_tk_array:
		case CORBA_tk_TypeCode:
		case CORBA_tk_except:
		case CORBA_tk_union:
		*/
		case CORBA_tk_wchar: 
		case CORBA_tk_wstring: {
			rb_raise(eCorbaError, "Wide characters are not supported");
		}
		default: {
			rb_raise(eCorbaError, "Some really amazing stuff met in output params: %s (%d)", tc->name, tc->kind);
		}
	}
	return Qtrue;
}

