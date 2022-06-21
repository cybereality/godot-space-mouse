/*
Copyright (c) 2022 Andres Hernandez

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <gdnative_api_struct.gen.h>
#include <string.h>
#include <hidapi.h>

#ifndef HID_API_MAKE_VERSION
#define HID_API_MAKE_VERSION(mj, mn, p) (((mj) << 24) | ((mn) << 8) | (p))
#endif
#ifndef HID_API_VERSION
#define HID_API_VERSION HID_API_MAKE_VERSION(HID_API_VERSION_MAJOR, HID_API_VERSION_MINOR, HID_API_VERSION_PATCH)
#endif

#if defined(__APPLE__) && HID_API_VERSION >= HID_API_MAKE_VERSION(0, 12, 0)
#include <hidapi_darwin.h>
#endif

#if defined(_WIN32) && HID_API_VERSION >= HID_API_MAKE_VERSION(0, 12, 0)
#include <hidapi_winapi.h>
#endif

#if defined(USING_HIDAPI_LIBUSB) && HID_API_VERSION >= HID_API_MAKE_VERSION(0, 12, 0)
#include <hidapi_libusb.h>
#endif

#define MODELS 7
#define IDS 3

int model_ids[MODELS][IDS] = {
	// space navigator
	{ 0x046d, 0xc626, 0x00 },
	// space mouse compact
	{ 0x256f, 0xc635, 0x00 },
	// space mouse pro wireless
	{ 0x256f, 0xc632, 0x01 },
	// space mouse pro
	{ 0x046d, 0xc62b, 0x00 },
	// space mouse wireless
	{ 0x256f, 0xc62e, 0x01 },
	// universal receiver
	{ 0x256f, 0xc652, 0x01 },
	// space pilot pro
	{ 0x046d, 0xc629, 0x00 }
};

enum Format {
	Original = 0,
	Current = 1
};

int current_model = -1;

typedef struct SpaceData {
	int px, py, pz, rx, ry, rz;
} SpaceData;

typedef struct SpaceMotion {
    godot_vector3 translation;
    godot_vector3 rotation;
} SpaceMotion;

int to_int(unsigned char* buffer)
{
	int val = (buffer[1] << 8) + buffer[0];
	if (val >= 32768)
		val = -(65536 - val);
    return val;
}

hid_device *space_device;
unsigned char space_buffer[256];
SpaceData space_data = {0, 0, 0, 0, 0, 0};
int space_connected = 0;

const godot_gdnative_core_api_struct *api = NULL;
const godot_gdnative_ext_nativescript_api_struct *nativescript_api = NULL;

void *spacemouse_constructor(godot_object *p_instance, void *p_method_data);
void spacemouse_destructor(godot_object *p_instance, void *p_method_data, void *p_user_data);
godot_variant spacemouse_translation(godot_object *p_instance, void *p_method_data,
        void *p_user_data, int p_num_args, godot_variant **p_args);
godot_variant spacemouse_rotation(godot_object *p_instance, void *p_method_data,
        void *p_user_data, int p_num_args, godot_variant **p_args);
godot_variant spacemouse_connect(godot_object *p_instance, void *p_method_data,
        void *p_user_data, int p_num_args, godot_variant **p_args);
godot_variant spacemouse_poll(godot_object *p_instance, void *p_method_data,
        void *p_user_data, int p_num_args, godot_variant **p_args);
        
void GDN_EXPORT godot_gdnative_init(godot_gdnative_init_options *p_options) {
    api = p_options->api_struct;

    for (int i = 0; i < api->num_extensions; i++) {
        switch (api->extensions[i]->type) {
            case GDNATIVE_EXT_NATIVESCRIPT: {
                nativescript_api = (godot_gdnative_ext_nativescript_api_struct *)api->extensions[i];
            }; break;
            default: break;
        }
    }
}

void GDN_EXPORT godot_gdnative_terminate(godot_gdnative_terminate_options *p_options) {
    api = NULL;
    nativescript_api = NULL;
}

void GDN_EXPORT godot_nativescript_init(void *p_handle) {
    godot_instance_create_func create = { NULL, NULL, NULL };
    create.create_func = &spacemouse_constructor;

    godot_instance_destroy_func destroy = { NULL, NULL, NULL };
    destroy.destroy_func = &spacemouse_destructor;

    nativescript_api->godot_nativescript_register_class(p_handle, "SPACEMOUSE", "Reference",
            create, destroy);
    
    godot_instance_method connect = { NULL, NULL, NULL };
    connect.method = &spacemouse_connect;
    
    godot_instance_method poll = { NULL, NULL, NULL };
    poll.method = &spacemouse_poll;
    
    godot_instance_method translation = { NULL, NULL, NULL };
    translation.method = &spacemouse_translation;
    
    godot_instance_method rotation = { NULL, NULL, NULL };
    rotation.method = &spacemouse_rotation;

    godot_method_attributes attributes = { GODOT_METHOD_RPC_MODE_DISABLED };

	nativescript_api->godot_nativescript_register_method(p_handle, "SPACEMOUSE", "connect",
            attributes, connect);
            
    nativescript_api->godot_nativescript_register_method(p_handle, "SPACEMOUSE", "poll",
            attributes, poll);
            
    nativescript_api->godot_nativescript_register_method(p_handle, "SPACEMOUSE", "translation",
            attributes, translation);
            
    nativescript_api->godot_nativescript_register_method(p_handle, "SPACEMOUSE", "rotation",
            attributes, rotation);
}

void *spacemouse_constructor(godot_object *p_instance, void *p_method_data) {
    SpaceMotion *motion_data = api->godot_alloc(sizeof(SpaceMotion));
    
    api->godot_vector3_new(&motion_data->translation, 0.0, 0.0, 0.0);
    api->godot_vector3_new(&motion_data->rotation, 0.0, 0.0, 0.0);
    
    return motion_data;
}

void spacemouse_destructor(godot_object *p_instance, void *p_method_data, void *p_user_data) {
	hid_close(space_device);
	hid_exit();
    api->godot_free(p_user_data);
}

godot_variant spacemouse_connect(godot_object *p_instance, void *p_method_data,
        void *p_user_data, int p_num_args, godot_variant **p_args) {
	godot_variant failure;
    api->godot_variant_new_bool(&failure, false);
    
    godot_variant success;
    api->godot_variant_new_bool(&success, true);
    
	if (hid_init()) {
		space_connected = 0;
		return failure;
	}

#if defined(__APPLE__) && HID_API_VERSION >= HID_API_MAKE_VERSION(0, 12, 0)
	hid_darwin_set_open_exclusive(1);
#endif

	struct hid_device_info *hid_devices, *current_hid;	
	hid_devices = hid_enumerate(0x0, 0x0);
	current_hid = hid_devices;
	int not_found = 1;
	
	while (current_hid && not_found) {
		for (int model = 0; model < MODELS; model++) {
			if (model_ids[model][0] == current_hid->vendor_id &&
					model_ids[model][1] == current_hid->product_id) {
				current_model = model;
				not_found = 0;
			}
		}
		current_hid = current_hid->next;
	}
	
	hid_free_enumeration(hid_devices);

	space_device = hid_open(model_ids[current_model][0], model_ids[current_model][1], NULL);
	
	if (!space_device) {
 		space_connected = 0;
		return failure;
	}
    
    hid_set_nonblocking(space_device, 1);
    
	memset(space_buffer, 0x00, sizeof(space_buffer));
	space_buffer[0] = 0x01;
	space_buffer[1] = 0x81;
	
	space_connected = 1;
	
	return success;
}

godot_variant spacemouse_poll(godot_object *p_instance, void *p_method_data,
        void *p_user_data, int p_num_args, godot_variant **p_args) {
	godot_variant failure;
    api->godot_variant_new_bool(&failure, false);
    
    godot_variant success;
    api->godot_variant_new_bool(&success, true);

    godot_variant result = failure;
	
    SpaceMotion *motion_data = (SpaceMotion *)p_user_data;
    
	godot_vector3 translation;
	api->godot_vector3_new(&translation, 0.0, 0.0, 0.0);
	godot_vector3 rotation;
	api->godot_vector3_new(&rotation, 0.0, 0.0, 0.0);
	
	godot_vector3 space_trans;
	godot_vector3 space_rot;
	
	int poll_count = 2;

	if (model_ids[current_model][2] == 0)
		poll_count *= 2;
		
	space_data.px = 0;
	space_data.py = 0;
	space_data.pz = 0;
	space_data.rx = 0;
	space_data.ry = 0;
	space_data.rz = 0;

	while(space_connected && poll_count-- > 0) {
		int read_len = hid_read(space_device, space_buffer, sizeof(space_buffer));
		if (read_len > 0) {
			switch(model_ids[current_model][2]) {
				case Original:
					if (space_buffer[0] == 1) {
						space_data.px += to_int(&space_buffer[1]);
						space_data.py += to_int(&space_buffer[5]);
						space_data.pz += to_int(&space_buffer[3]);
					} else if (space_buffer[0] == 2) {
						space_data.rx += to_int(&space_buffer[1]);
						space_data.ry += to_int(&space_buffer[5]);
						space_data.rz += to_int(&space_buffer[3]);
					}
					break;
				case Current:
					if (space_buffer[0] == 1) {
						space_data.px += to_int(&space_buffer[1]);
						space_data.py += to_int(&space_buffer[5]);
						space_data.pz += to_int(&space_buffer[3]);
						space_data.rx += to_int(&space_buffer[7]);
						space_data.ry += to_int(&space_buffer[11]);
						space_data.rz += to_int(&space_buffer[9]);
					}
					break;
			}
		}
	}
	
	if (space_connected) {
		api->godot_vector3_new(&space_trans, space_data.px, space_data.py, space_data.pz);
		translation = api->godot_vector3_operator_add(&translation, &space_trans);
		
		api->godot_vector3_new(&space_rot, space_data.rx, space_data.ry, space_data.rz);
		rotation = api->godot_vector3_operator_add(&rotation, &space_rot);
		
		result = success;
	}
	motion_data->translation = translation;
	motion_data->rotation = rotation;
	
    return result;
}

godot_variant spacemouse_translation(godot_object *p_instance, void *p_method_data,
        void *p_user_data, int p_num_args, godot_variant **p_args) {
    godot_variant result;
    
    SpaceMotion *motion_data = (SpaceMotion *)p_user_data;
    api->godot_variant_new_vector3(&result, &motion_data->translation);
    
    return result;
}

godot_variant spacemouse_rotation(godot_object *p_instance, void *p_method_data,
        void *p_user_data, int p_num_args, godot_variant **p_args) {
    godot_variant result;
    
    SpaceMotion *motion_data = (SpaceMotion *)p_user_data;
    api->godot_variant_new_vector3(&result, &motion_data->rotation);
    
    return result;
}
