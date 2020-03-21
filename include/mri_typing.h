#ifndef _MRI_TYPING_H_
#define _MRI_TYPING_H_

#include <stddef.h>
#include <stdint.h>

/* defines */
#define MRI_MAX_HOST_SIZE		(128)
#define MRI_MAX_SLOT_STR_SIZE	(256)
#define MRI_USERDATA_SIZE		(32)
 
#define MRI_SLOT_FLAG_CONST		(1 << 0)	/* TODO(omer): Unused for now	*/
#define MRI_SLOT_FLAG_RATE		(1 << 1)	/* TODO(omer): Unused for now	*/
#define MRI_SLOT_FLAG_PK		(1 << 2)
#define MRI_SLOT_FLAG_HIDDEN	(1 << 3)

typedef enum mri_service_mode {
	MRI_PRODUCER = 0b01,
	MRI_CONSUMER = 0b10
} mri_service_mode_t;

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
 
#endif 
