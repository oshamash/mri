#ifndef _MRI_H_
#define _MRI_H_

#include "mri_config.h"
#include "mri_typing.h"
#include "mri_logging.h"

/* prototypes */

/**
 * mri_init()
 *
 * Notes:
 *	Each instance can only be linked to one domain (! consider !)
 *	Instance is global to process having thread-oriented CB support
 *
 *	@param config - pointer to configuration object to use (ownership transfered)
 *	@returns C-Style boolean (0 = OK, <0 = ERROR) (-ERRNO)
 */
int mri_init(struct mri_coniguration **config);
#endif /* _MRI_H_ */
