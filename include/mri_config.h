#ifndef _MRI_CONFIG_H_
#define _MRI_CONFIG_H_

#include "mri_typing.h"

#define MRI_CONFIG_MAX_STRING_SIZE 128

/* FWD Decleration */
struct mri_configuration;

typedef enum mri_config_value {
	/* Producer configuration */
	MRI_CONFIG_DOMAIN,

	/* Service discovery configuration */
	MRI_CONFIG_SERVICE_DISCOVERY_HOST,
	MRI_CONFIG_SERVICE_DISCOVERY_PORT,
	MRI_CONFIG_SERVICE_DISCOVERY_TO_USE,

	/* Redis specific */
	MRI_CONFIG_REDIS_SD_TIMEOUT,
	MRI_CONFIG_REDIS_SD_USE_SENTINEL,
	MRI_CONFIG_REDIS_SD_SENTINEL_MASTER,

	/* Never go beyond this point */
	MRI_CONFIG_MAX_VALUE
} mri_config_value_t;

/**
 * Convert key to human-readable string (log usage)
 *	@param type - the enumeration value for the key
 *	@return cstring_t - string representing the enum-value
 */
cstring_t config_type_to_string(mri_config_value_t type);

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
 * Get a mutable configuration-value from object (copy of internal)
 *	@param config - mri configuration object to manage
 *	@param type - type of configuration-value (enum) to manage
 *	@param output - output buffer/object to write value into
 *	@param size - size of output object, on success write-back size written
 *	@returns C-style boolean (0 = OK, <0 = ERROR) (-ERRNO)
 */
int mri_config_get_mutable(struct mri_configuration *config, mri_config_value_t type, void *output, size_t *size);

/**
 * Get a non-mutable configuration-value from object (reference to internal)
 *  @param config - mri configuration object to manage
 *  @param type - type of configuration-value (enum) to manage
 *  @param output - pointer to data-pointer which will point to internal-data after call
 *  @param size - pointer to size which will be assigned with internal-data size after call
 *  @returns C-style boolean (0 = OK, <0 = ERROR) (-ERRNO)
 */
int mri_config_get(struct mri_configuration *config, mri_config_value_t type, const void **output, size_t *size);

/**
 * Set a mutable configuration-value from object (copy to internal)
 *	@param config - mri configuration object to manage
 *	@param type - type of configuration-value (enum) to manage
 *	@param input - input buffer/object to write as config-value (deep-copy)
 *	@param size - size of input object
 *	@returns C-style boolean (0 = OK, <0 = ERROR) (-ERRNO)
 */
int mri_config_set_mutable(struct mri_configuration *config, mri_config_value_t type, void *input, size_t size);

/**
 * Set a non-mutable configuration-value from object (internal reference to it)
 *	@param config - mri configuration object to manage
 *	@param type - type of configuration-value (enum) to manage
 *	@param input - buffer to set as config-value (pointer copy)
 *	@param size - size of input object
 *	@returns C-style boolean (0 = OK, <0 = ERROR) (-ERRNO)
 */
int mri_config_set(struct mri_configuration *config, mri_config_value_t type, void *input, size_t size);

#endif
