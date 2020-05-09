#ifndef _MRI_PRODUCER_H_
#define _MRI_PRODUCER_H_

#include "mri_sched.h"
#include "mri_typing.h"

/* Formatting apart from provided-formatters section */
int _mri_create_formatter(cstring_t name, mri_formatter_cb callback);

/* TODO(omer): Add shaper configuration (amount of samples, scope and more */

/* Shaping apart from provided-shapers section */
int _mri_create_shaper(cstring_t name, mri_shaper_cb callback);
 
/* Registering types and data to MRI */
int _mri_create_type(cstring_t, size_t);
int _mri_type_add_slot(cstring_t, cstring_t, cstring_t, size_t, size_t, int);
int _mri_type_add_vslot(cstring_t, cstring_t, mri_formatter_cb);
int _mri_type_add_shaper(cstring_t, cstring_t, cstring_t, size_t, size_t);
 
/* Current suggestion:
 *	when someone registers data on path, we capture his thread-id,
 *	this will allow for poll on an event object that is related to his thread-id.
 *	MRI will use data in <sched> to register the event-object for the user to poll on.
 *	Ultimatly this will allow user to synchronise access to registered data without (b)locking.
 *
 *	If none are provided, thread's data will use the default MRI sched (provided in init, or mri-thread)
 */
int mri_set_current_thread_sched(mri_sched_info_t *sched);

/* Registering data of defined types to MRI */
int _mri_register(cstring_t path, cstring_t type, void *object, mri_iterator_cb callback);
int mri_register_config(cstring_t path, void *context, mri_config_change_cb callback);
int mri_unregister(cstring_t path);
 
/* Formalize API using macros */
#define mri_create_type(c_type) \
	_mri_create_type(#c_type, sizeof(c_type))
#define mri_type_add_slot(c_type, slot, format, flags) \
	_mri_type_add_slot(#c_type, #slot, #format, (size_t) &(((c_type *) 0)->slot), sizeof(((c_type *) 0)->slot), flags)
#define mri_type_add_vslot(c_type, slot, vslot_format) \
	_mri_type_add_vslot(#c_type, slot, vslot_foramt)
#define mri_register(path, c_type, object, callback) \
	_mri_register(path, #c_type, object, callback)
#define mri_create_formatter(c_type, callback) \
	_mri_create_fromatter(#c_type, callback)
#define mri_create_shaper(c_type, callback) \
	_mri_create_shaper(#c_type, callback)

#define mri_type_add_shaper_slot(c_type, slot, format, shaper) \
	_mri_type_add_shaper(#c_type, #slot, #format, (size_t) &(((c_type *) 0)->slot), sizeof(((c_type *) 0)->slot), #shaper)

#define mri_type_add_rate(c_type, slot) \
	mri_type_add_shaper(c_type, slot, mri_number_t, mri_rate_shaper_t)

#endif
