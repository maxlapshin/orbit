#include "ruby_orbit.h"

#define OUT_PARAM 1
#define INDIRECT_PARAM 2



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
		case CORBA_tk_octet: {
			char *val = ALLOCATE_FOR(char);
			if(T_FIXNUM == TYPE(value)) {
				*val = (char)NUM2INT(value);
			} else if (cLong == rb_class_of(value)) {
				*val = (char)DATA_PTR(value);
			} else {
				rb_raise(rb_eTypeError, "wrong type argument %s (expected Fixnum or ORBit2::Long)", rb_obj_classname(value));
			}
			return val;
		}
		case CORBA_tk_string: {
			Check_Type(value, T_STRING);
			char **val = ALLOCATE_FOR(char *);
			*val = CORBA_string_dup(STR(value));
			return val;
		}
		case CORBA_tk_ushort:
		case CORBA_tk_short: {
			short *val = ALLOCATE_FOR(short);
			if(T_FIXNUM == TYPE(value)) {
				*(short *)(val) = (short)FIX2INT(value);
			} else if(cLong == rb_class_of(value)){
				*(short *)(val) = (short)(long)DATA_PTR(value);
			} else {
				rb_raise(rb_eTypeError, "wrong type argument %s (expected Fixnum or ORBit2::Long)", rb_obj_classname(value));
			}
			if(!out) return val;
			short **val_ptr = ALLOCATE_FOR(short *);
			*val_ptr = val;
			return val_ptr;
		}
		case CORBA_tk_enum:
		case CORBA_tk_long:
		case CORBA_tk_ulong: {
			long *val = ALLOCATE_FOR(long);
			if(T_FIXNUM == TYPE(value)) {
				*(long *)(val) = FIX2INT(value);
			} else if (cLong == rb_class_of(value)) {
				*(long *)(val) = (long)DATA_PTR(value);
			} else {
				rb_raise(rb_eTypeError, "wrong type argument %s (expected Fixnum or ORBit2::Long)", rb_obj_classname(value));
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
			double *val = ALLOCATE_FOR(double);
			if(T_FLOAT == TYPE(value)) {
				*val = RFLOAT(value)->value;
			} else if(T_FIXNUM == TYPE(value)) {
				*val = (double)NUM2INT(value);
			} else if(cLong = rb_class_of(value)) {
				*val = (double)(long)DATA_PTR(value);
			} else {
				rb_raise(rb_eTypeError, "wrong type argument %s (expected Float, Fixnum or ORBit2::Long)", rb_obj_classname(value));
			}
			if(!out) return val;
			double **val_ptr = ALLOCATE_FOR(double *);
			*val_ptr = val;
			return val_ptr;
		}
		case CORBA_tk_longdouble: {
			rb_raise(eCorbaError, "Long double unsupported");
		}
		case CORBA_tk_float: {
			Check_Type(value, T_FLOAT);
			float *val = ALLOCATE_FOR(float);
			*val = (float)RFLOAT(value)->value;
			if(!out) return val;
			float **val_ptr = ALLOCATE_FOR(float *);
			*val_ptr = val;
			return val_ptr;
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
			encoded->_release = 0;
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
			char *struct_raw = pool + *pool_pos;
			int i;
			int offset = 0;;
			for(i = 0; i < tc->sub_parts; i++) {
				size_t field_size = ORBit_gather_alloc_info (tc->subtypes[i]);
				offset = ALIGN_VALUE (offset, tc->subtypes[i]->c_align);
				VALUE field = rb_funcall(value, rb_intern(tc->subnames[i]), 0);
				marshall_value(tc->subtypes[i], field, pool, pool_pos, 0);
				offset += field_size;
			}
			int struct_size = ORBit_gather_alloc_info(tc);
 			*pool_pos += struct_size - offset;
			if(!out) return struct_raw;
			char **struct_ptr = ALLOCATE_FOR(char *);
			*struct_ptr = struct_raw;
			return struct_ptr;
		}
		case CORBA_tk_objref: {
			if(!rb_obj_is_kind_of(value, cCorbaObject)) {
				rb_raise(rb_eTypeError, "wrong type argument %s (expected ancestor of ORBit2::CorbaObject)", rb_obj_classname(value));
			}
			CORBA_Object* obj = ALLOCATE_FOR(CORBA_Object);
			*obj = (CORBA_Object)DATA_PTR(value);
			if(!out) return obj;
			CORBA_Object **ptr = ALLOCATE_FOR(CORBA_Object *);
			*ptr = obj;
			return ptr;
		}
		case CORBA_tk_sequence: {
			Check_Type(value, T_ARRAY);
			CORBA_sequence_CORBA_octet *sval = ALLOCATE_FOR(CORBA_sequence_CORBA_octet);
			CORBA_sequence_set_release(sval, CORBA_FALSE);
			sval->_length = RARRAY(value)->len;
			int i;
			sval->_buffer = (void *)(pool + *pool_pos);
			for(i = 0; i < RARRAY(value)->len; i++) {
				marshall_value(tc->subtypes[0], RARRAY(value)->ptr[i], pool, pool_pos, 0);
			}
			if(!out) return sval;
			CORBA_sequence_CORBA_octet **ptr = ALLOCATE_FOR(CORBA_sequence_CORBA_octet *);
			*ptr = sval;
			return ptr;
		}
		case CORBA_tk_array: {
			Check_Type(value, T_ARRAY);
			gpointer ptr = pool + *pool_pos;
			CORBA_TypeCode subtype = tc->subtypes[0];
			int i;
			for(i = 0; i < tc->length && i < RARRAY(value)->len; i++) {
				marshall_value(subtype, RARRAY(value)->ptr[i], pool, pool_pos, 0);
			}
			ALLOCATE((tc->length - i)*ORBit_gather_alloc_info(subtype));
			if(!out) return ptr;
			gpointer *ret = ALLOCATE_FOR(gpointer);
			*ret = ptr;
			return ret;
		}
/*		
		case CORBA_tk_Principal:
		case CORBA_tk_TypeCode:
		case CORBA_tk_except:
		case CORBA_tk_union:
		case CORBA_tk_boolean:
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

