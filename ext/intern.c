#include "ruby_orbit.h"



ORBit_IMethod* object_get_method(VALUE self, char *method_name) {
	ORBit_IInterface* interface = ORBit_small_get_iinterface(DATA_PTR(self), get_object_type_id(self), &ruby_orbit2_ev);
	int i = 0;
	int method_index = -1;
	char getter_name[strlen(method_name)+6];
	snprintf(getter_name, sizeof(getter_name), "_get_%s", method_name);
	
	char setter_name[strlen(method_name)+6];
	snprintf(setter_name, sizeof(setter_name), "_set_%s", method_name);
	if(setter_name[strlen(setter_name)-1] == '=') {
		setter_name[strlen(setter_name)-1] = 0;
	} else {
		setter_name[0] = 0;
	}
	for(i = 0; i < interface->methods._length; i++) {
		if(!strcmp(interface->methods._buffer[i].name, method_name)) {
			method_index = i;
			break;
		}
		if(!strcmp(interface->methods._buffer[i].name, getter_name)) {
			method_index = i;
			break;
		}
		if(*setter_name && !strcmp(interface->methods._buffer[i].name, setter_name)) {
			method_index = i;
			break;
		}
	}
	if(method_index == -1) {
		return NULL;
	}
	return &(interface->methods._buffer[i]);
}


char* get_object_type_id(VALUE self) {
	VALUE retval;
	retval = rb_iv_get(self, "@type_id");
	if(retval == Qnil) {
		char* type_id = ORBit_small_get_type_id(DATA_PTR(self), &ruby_orbit2_ev);
		CHECK_EXCEPTION("Couldn't get type id of object");
		retval = rb_str_new2(type_id);
		CORBA_free(type_id);
		rb_iv_set(self, "@type_id", retval);
	}
	return STR(retval);
}


void corba_object_free(CORBA_Object obj) {
	CORBA_Object_release(obj, &ruby_orbit2_ev);
}


//static GHashTable *ruby_objects;

VALUE create_ruby_corba_object(CORBA_Object obj) {
//	printf("Loaded objref %d\n", obj);
//	gpointer maybe_object = g_hash_table_lookup(ruby_objects, obj);
//	if(maybe_object) {
//		return maybe_object;
//	}
	
	char* _type_id = ORBit_small_get_type_id(obj, &ruby_orbit2_ev);
	CHECK_EXCEPTION("Couldn't get type id of object");
	VALUE type_id = rb_str_new2(_type_id);
	CORBA_free(_type_id);
	VALUE klass = rb_funcall(cCorbaObject, rb_intern("lookup!"), 1, type_id);
	VALUE result = Data_Wrap_Struct(klass, 0, corba_object_free, obj);
//	g_hash_table_insert(ruby_objects, obj, result);
	return result;
}
