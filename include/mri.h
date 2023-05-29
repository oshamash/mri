#ifndef _MRI_H_
#define _MRI_H_

#include "mri_sched.h"
#include "mri_config.h"
#include "mri_typing.h"
#include "mri_logging.h"

/* prototypes */

/* Notes:
 *	Each instance can only be linked to one domain (! consider !)
 *	Instance is global to process having thread-oriented CB support
 */

/**
 * mri_init_threaded()
 * Initialize the MRI instance in threaded-mode (new thread is created)
 * All of the events needed to be handled by MRI will be done in the thread
 *
 *	@param config	- pointer to configuration object to use (ownership transfered)
 *	@return int - C-Style boolean (0 = OK, <0 = ERROR) (-ERRNO)
 */
int mri_init_threaded(struct mri_coniguration **config);

/**
 * mri_init_async()
 * Initialize the MRI instance in async-mode (no threads are created)
 * All of the events needed to be handled by MRI will happen in user-thread
 *
 *	@param config	- pointer to configuration object to use (ownership transfered)
 *	@param sched	- information needed to schedule MRI events into user-scheduling
 *	@return int - C-Style boolean (0 = OK, <0 = ERROR) (-ERRNO)
 */
int mri_init_async(struct mri_coniguration **config, mri_sched_info_t *sched);
#endif /* _MRI_H_ */
