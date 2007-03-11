#include "ruby_orbit.h"


void object_unmarshall_outvalues(ORBit_IMethod* method, int argc, VALUE *argv, gpointer *args, char *pool) {
	int i;
	int t = 0;
	int *pool_pos = &t;
	return;
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
				rb_raise(eCorbaError, "Wide char unsupported");
				break;
			}
			case CORBA_tk_char:
			case CORBA_tk_string: {
				Check_Type(argv[i], T_STRING);
				char **val = (char **)(pool + *pool_pos);
				*pool_pos += sizeof(char *);
				*val = STR(argv[i]);
				args[i] = val;
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
				Check_Type(argv[i], T_FLOAT);
				double **val = (double **)(pool + *pool_pos);
				*pool_pos += sizeof(double *);
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
			case CORBA_tk_null:
			case CORBA_tk_void: {
				args[i] = NULL;
				break;
			}
			case CORBA_tk_any: {
				CORBA_any *encoded = (CORBA_any *)(pool + *pool_pos);
				*pool_pos += sizeof(CORBA_any);
				encoded->_type = TC_null;
				encoded->_value = NULL;
				if(T_FIXNUM == TYPE(argv[i]) || cLong == rb_class_of(argv[i])) {
					encoded->_type = TC_CORBA_long;
					encoded->_value = (long *)(pool + *pool_pos);
					*pool_pos += sizeof(long);
					if(T_FIXNUM == TYPE(argv[i])) {
						*(long *)(encoded->_value) = FIX2INT(argv[i]);
					} else {
						*(long *)(encoded->_value) = DATA_PTR(argv[i]);
					}
				} else if (T_STRING == TYPE(argv[i])) {
					encoded->_type = TC_CORBA_string;
					encoded->_value = (char **)(pool + *pool_pos);
					*pool_pos += sizeof(char *);
					*(char **)(encoded->_value) = STR(argv[i]);
				}
				args[i] = encoded;
				break;
			}
/*			
			case CORBA_tk_Principal:
			case CORBA_tk_objref:
			case CORBA_tk_TypeCode:
			case CORBA_tk_except:
			case CORBA_tk_struct:
			case CORBA_tk_union:
			case CORBA_tk_sequence:
			case CORBA_tk_boolean:
			case CORBA_tk_char:
			case CORBA_tk_octet:
			case CORBA_tk_array:
			case CORBA_tk_fixed:
		*/
			default: {
				rb_warn("Don't know, how to marshall kind %d. Marshalling nil", tc->kind);
				char **val = (char **)(pool + *pool_pos);
				*pool_pos += sizeof(char *);
				*val = 0;
				args[i] = val;
				break;
			}
		}
	}
}

VALUE object_unmarshall(CORBA_TypeCode tc, gpointer retval) {
	if(!retval || !tc || tc->kind == CORBA_tk_void || tc->kind == CORBA_tk_null) {
		return Qnil;	
	}
	switch(tc->kind) {
		case CORBA_tk_short:
		case CORBA_tk_ushort: {
			return INT2NUM(*(unsigned short *)retval);
		}
		case CORBA_tk_long: {
			return INT2NUM(*(long *)retval);
		}
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
			return rb_float_new((double)(*(float *)retval));
		}
		case CORBA_tk_double: {
			return rb_float_new(*(double *)retval);
		}
		case CORBA_tk_boolean: {
			return *(char *)retval ? Qtrue : Qfalse;
		}
		case CORBA_tk_char: {
			return rb_str_new(retval, 1);
		}
		case CORBA_tk_octet: {
			return INT2FIX((int)*(char *)retval);
		}
		case CORBA_tk_any: {
			CORBA_any *decoded = *(CORBA_any **)retval;
			return object_unmarshall(decoded->_type, decoded->_value);
		}
		case CORBA_tk_Principal: {
			rb_raise(eCorbaError, "CORBA_Principal is unknown shit. Contact max@maxidoors.ru to include it's support");
		}
		case CORBA_tk_objref: {
			return create_ruby_corba_object(*(CORBA_Object *)retval);
		}
		case CORBA_tk_string: {
			return rb_str_new2(*(char **)retval);
		}
		case CORBA_tk_array:
		case CORBA_tk_sequence: {
			rb_raise(eCorbaError, "Sequences and arrays are not implemented yet");
		}
		case CORBA_tk_void:
		case CORBA_tk_null: {
			return Qnil;
		}
		case CORBA_tk_fixed: {
			rb_raise(eCorbaError, "Nobody knows, how to unmarshall fixed");
		}
		/*
		case CORBA_tk_TypeCode:
		case CORBA_tk_except:
		case CORBA_tk_struct: 
		case CORBA_tk_enum:
		case CORBA_tk_union:
		*/
		case CORBA_tk_wchar: 
		case CORBA_tk_wstring: {
			rb_raise(eCorbaError, "Wide characters are not supported");
		}
		default: {
			rb_raise(eCorbaError, "Some really amazing stuff met in output params");
		}
	}
	return Qtrue;
}

