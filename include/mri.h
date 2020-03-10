#ifndef _MRI_H_
#define _MRI_H_

#include <stdint.h>
#include <stddef.h>

/* defines */
#define MRI_MAX_SLOT_STR_SIZE	(256)
#define MRI_USERDATA_SIZE		(32)
 
#define MRI_SLOT_FLAG_CONST		(1 << 0)	/* TODO(omer): Unused for now	*/
#define MRI_SLOT_FLAG_RATE		(1 << 1)	/* TODO(omer): Unused for now	*/
#define MRI_SLOT_FLAG_PK		(1 << 2)
#define MRI_SLOT_FLAG_HIDDEN	(1 << 3)

typedef enum mri_iter_action {
	MRI_ITER_STOP,
	MRI_ITER_CONTINUE
} mri_iter_action_t;
 
/* typedefs */
typedef uint8_t		mri_byte_t;
typedef const char	*cstring_t;
 
typedef struct mri_iter_state {
	size_t		iteration_count;				/* Iteration count (incremented by MRI) */
	mri_byte_t	userdata[MRI_USERDATA_SIZE];	/* User provided data to help iteration	*/
} mri_iter_state_t;

/* Reflects result of capturing a slot */
typedef struct mri_capture_sample {
	uint32_t	ns_timestamp;					/* Data timestamp (monotonic nanoseconds)	*/
	mri_byte_t	sample[MRI_MAX_SLOT_STR_SIZE];	/* Data captured via calls to the formatter	*/
} mri_capture_sample_t;

/* Provided formatters (multi-formatters) */
typedef struct {} mri_hex_t;			/* Hexadecimal representation	*/
typedef struct {} mri_number_t;			/* Plain normal numeric data	*/
typedef struct {} mri_buffer_t;			/* Memory address dump in bytes	*/
typedef struct {} mri_string_t;			/* ASCII encoded human string	*/
typedef struct {} mri_timespec_t;		/* timespec (tv_sec, tv_usec)	*/
typedef struct {} mri_epoch_time_t;		/* uint64_t seconds since epoch	*/
typedef struct {} mri_elapsed_time_t;	/* uint64_t monotonic diff		*/

/* Provided shapers (multi-shapers) */
typedef struct {} mri_rate_shaper_t;	/* Numeric slot rate per second	*/
 
/* Callbacks (TBD) */

/* Input: slot-data, size-of-slot, output-string			*/
typedef int (*mri_formatter_cb)(void *, size_t, char *);
/* Input: type-data, iteration-state-data, output-memory	*/
typedef mri_iter_action_t (*mri_iterator_cb)(void *, mri_iter_state_t *, void **);
/* Input: registered-context, cli-input, size-of-input		*/
typedef int (*mri_config_change_cb)(void *, cstring_t, size_t);
/* Input: list-of-samples, amount-of-samples, output-string	*/
typedef int (*mri_shaper_cb)(mri_capture_sample_t *, size_t, char *);
 
/* prototypes */
 
/**
 * mri_init()
 *
 * Notes:
 *	Each instance can only be linked to one domain (! consider !)
 *	Instance is global to process having thread-oriented CB support
 *
 *	@param domain - root-path, shared, kind of like catagory (routing, system-events, em, etc...)
 *	@returns C-Style boolean (0 = OK, <0 = ERROR) (-ERRNO)
 */
int mri_init(const char *domain);
 
/* Formatting apart from provided-formatters section */
int _mri_create_formatter(cstring_t name, mri_formatter_cb callback);

/* TODO(omer): Add shaper configuration (amount of samples, scope and more */

/* Shaping apart from provided-shapers section */
int _mri_create_shaper(cstring_t name, mri_shaper_cb callback);
 
/* Registering types and data to MRI */
int _mri_create_type(cstring_t name, size_t size);
int _mri_type_add_slot(cstring_t type, cstring_t name, cstring_t formatter, size_t offset, size_t size, int slot_flags);
int _mri_type_add_vslot(cstring_t type, cstring_t name, mri_formatter_cb callback);
int _mri_type_add_shaper(cstring_t type, cstring_t name, cstring_t formatter, size_t offset, size_t size, cstring_t shaper);
 
/* Current suggestion:
 *	when someone registers data on path, we capture his thread-id and
 *	allow him to poll on an event object that is related to his thread-id
 *
 *	mri_get_eventloop_fd() will allow user to allocate time for MRI in any bitch-thread
 *	mri_get_notification_fd() will allow to get the thread-bound event-fd to poll in relevant thread
 */
int mri_get_eventloop_fd();
int mri_get_notification_fd();
 
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

#endif /* _MRI_H_ */
