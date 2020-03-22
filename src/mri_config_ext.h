#ifndef _MRI_CONFIG_EXT_H_
#define _MRI_CONFIG_EXT_H_

#include <stddef.h>

/* Note: data is never free()'d - stays as long as the program does */
typedef struct mri_config_data {
    void    *data;	/*!< config-item data buffer (allocate if needed)	*/
    size_t  length;	/*!< Length of buffer (if dynamic sizing is needed)	*/
} mri_config_data_t;

/**
 * Signature for config handlers (defined as externals inside mri_config.c)
 *	@param input - input string -> used-data from main()
 *	@param output - where to save configuration after parsing
 *	@return C-style boolean (0 = OK, <0 = ERROR) (-ERRNO)
 */
typedef int (config_handler_cb)(const char *input, mri_config_data_t *output);

#endif
