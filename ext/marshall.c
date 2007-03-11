#include "ruby_orbit.h"

static gpointer marshall_value(CORBA_TypeCode tc, VALUE value, char *pool, int* pool_pos, int out) {
	while (tc->kind == CORBA_tk_alias)
		tc = tc->subtypes[0];

	switch (tc->kind) {
		case CORBA_tk_wchar:
		case CORBA_tk_wstring: {
			rb_raise(eCorbaError, "Wide char unsupported");
			break;
		}
		case CORBA_tk_char:
		case CORBA_tk_string: {
			Check_Type(value, T_STRING);
			char **val = ALLOCATE_FOR(char *);
			*val = STR(value);
			return  val;
		}
		case CORBA_tk_ushort:
		case CORBA_tk_short: {
			short *val = ALLOCATE_FOR(short);
			if(T_FIXNUM == TYPE(value)) {
				*(short *)(val) = (short)FIX2INT(value);
			} else {
				*(short *)(val) = (short)(long)DATA_PTR(value);
			}
			if(!out) return val;
			short **val_ptr = ALLOCATE_FOR(short *);
			*val_ptr = val;
			return val_ptr;
		}
		case CORBA_tk_long:
		case CORBA_tk_ulong: {
			long *val = ALLOCATE_FOR(long);
			if(T_FIXNUM == TYPE(value)) {
				*(long *)(val) = FIX2INT(value);
			} else {
				*(long *)(val) = (long)DATA_PTR(value);
			}
			if(!out) return val;
			long **val_ptr = ALLOCATE_FOR(long *);
			*val_ptr = val;
			return val_ptr;
		}
		case CORBA_tk_longlong:
		case CORBA_tk_ulonglong: {
			rb_raise(eCorbaError, "Long Long unsupported");
		}
		case CORBA_tk_double: {
			Check_Type(value, T_FLOAT);
			double **val = ALLOCATE_FOR(double *);
			*val = &(RFLOAT(value)->value);
			return val;
		}
		case CORBA_tk_longdouble: {
			rb_raise(eCorbaError, "Long double unsupported");
		}
		case CORBA_tk_float: {
			rb_raise(eCorbaError, "Float unsupported");
		}
		case CORBA_tk_null:
		case CORBA_tk_void: {
			return NULL;
			break;
		}
		case CORBA_tk_any: {
			CORBA_any *encoded = ALLOCATE_FOR(CORBA_any);
			encoded->_type = TC_null;
			encoded->_value = NULL;
			if(T_FIXNUM == TYPE(value) || cLong == rb_class_of(value)) {
				encoded->_type = TC_CORBA_long;
				encoded->_value = ALLOCATE_FOR(long);
				if(T_FIXNUM == TYPE(value)) {
					*(long *)(encoded->_value) = FIX2INT(value);
				} else {
					*(long *)(encoded->_value) = (long)DATA_PTR(value);
				}
			} else if (T_STRING == TYPE(value)) {
				encoded->_type = TC_CORBA_string;
				encoded->_value = ALLOCATE_FOR(char *);
				*(char **)(encoded->_value) = STR(value);
			}
			return encoded;
		}
		case CORBA_tk_struct: {
			int struct_size = tc->sub_parts*sizeof(long long);
			char* struct_raw = ALLOCATE(struct_size);
			int i;
			int offset = 0;;
			for(i = 0; i < tc->sub_parts; i++) {
				offset = ALIGN_VALUE (offset, tc->subtypes[i]->c_align);
				VALUE field = rb_funcall(value, rb_intern(tc->subnames[i]), 0);
				gpointer marshalled = marshall_value(tc->subtypes[i], field, pool, pool_pos, 0);
				memcpy(struct_raw + offset, marshalled, tc->subtypes[i]->c_align);
			}
			return struct_raw;
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
			rb_warn("Don't know, how to marshall %s. Marshalling nil", tc->name);
			char **val = ALLOCATE_FOR(char *);
			*val = 0;
			return val;
		}
	}
	return NULL;
}

void object_marshall_arguments(ORBit_IMethod* method, int argc, VALUE *argv, gpointer *args, char *pool, int* pool_pos) {
	int i;
	for(i = 0; i < method->arguments._length; i++) {
		args[i] = marshall_value(method->arguments._buffer[i].tc, argv[i], pool, pool_pos, method->arguments._buffer[i].flags & ORBit_I_ARG_OUT);
	}
}

