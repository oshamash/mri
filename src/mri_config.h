#ifndef _MRI_CONFIG_H_
#define _MRI_CONFIG_H_

#include "mri_typing.h"

/* FWD Decleration */
struct mri_configuration;

enum mri_config_value {
	/* Producer configuration */
	MRI_CONFIG_DOMAIN,

	/* Service discovery configuration */
	MRI_CONFIG_SERVICE_DISCOVERY_HOST,
	MRI_CONFIG_SERVICE_DISCOVERY_PORT,
	MRI_CONFIG_SERVICE_DISCOVERY_TO_USE,

	/* Never go beyond this point */
	MRI_CONFIG_MAX_VALUE
};

/**
 * Create configuration object
 * Input is formatted in getopt() fashion
 *	@param argc - main-like input, number of arguments
 *	@param argv - main-like inout, array of argument-strings
 *	@return mri_configuration on success, NULL on failure
 */
struct mri_configuration *mri_config_create(int argc, char **argv);

/**
 * Destroy configuration object
 *	@param config - mri configuration object to destroy
 *	@returns C-style boolean (0 = OK, <0 = ERROR) (-ERRNO)
 */
int mri_config_destroy(struct mri_configuration *config);

/**
 * Get configuration-value from object
 *	@param config - mri configuration object to manage
 *	@param type - type of configuration-value (enum) to manage
 *	@param output - output buffer/object to write value into
 *	@param size - size of output object, on success write-back size written
 *	@returns C-style boolean (0 = OK, <0 = ERROR) (-ERRNO)
 */
int mri_config_get(struct mri_configuration *config, mri_config_value type, void *output, size_t *size);

/**
 * Get configuration-value from object
 *	@param config - mri configuration object to manage
 *	@param type - type of configuration-value (enum) to manage
 *	@param input - input buffer/object to write as config-value
 *	@param size - size of input object
 *	@returns C-style boolean (0 = OK, <0 = ERROR) (-ERRNO)
 */
int mri_config_set(struct mri_configuration *config, mri_config_value type, void *input, size_t size);

#endif
