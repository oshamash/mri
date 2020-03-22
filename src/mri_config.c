#include "mri_config.h"
#include "mri_general.h"

#include <stdlib.h>
#include <getopt.h>
#include <string.h>

#include "mri_config_ext.h"

/* Note: remember to update to have proper debugging later */
const char *config_type_to_string(mri_config_value_t type) {
	switch(type) {
	case MRI_CONFIG_DOMAIN					: return "DOMAIN";
	case MRI_CONFIG_SERVICE_DISCOVERY_HOST	: return "SERVICE_DISCOVERY_HOST";
	case MRI_CONFIG_SERVICE_DISCOVERY_PORT	: return "SERVICE_DISCOVERY_PORT";
	case MRI_CONFIG_SERVICE_DISCOVERY_TO_USE: return "SERVICE_DISCOVERY_TO_USE";
	default									: return "UNKNOWN";
	}
}

struct mri_configuration {
	mri_config_data_t config[MRI_CONFIG_MAX_VALUE];
};

int cstring_ref_handler(const char *input, mri_config_data_t *output) {
	output->data = (void *) input;
	output->length = strnlen(input, MRI_CONFIG_MAX_STRING_SIZE);
	return 0;
}

/* External handlers from project */
extern int config_to_service_discovery(const char *, mri_config_data_t *);

/* Array of typedef int (config_handler_cb)(const char *input, mri_config_data_t *output); */
static int (*handle_config_cb[MRI_CONFIG_MAX_VALUE])(const char *, mri_config_data_t *) = {
	/* Note: we assume input is from main() so no need to allocate */
	[MRI_CONFIG_DOMAIN]						= cstring_ref_handler,
	[MRI_CONFIG_SERVICE_DISCOVERY_HOST]		= cstring_ref_handler,
	[MRI_CONFIG_SERVICE_DISCOVERY_PORT]		= cstring_ref_handler,
	[MRI_CONFIG_SERVICE_DISCOVERY_TO_USE]	= config_to_service_discovery
};

static int parse_opts(struct mri_configuration *conf, int argc, char **argv) {
	int option_index;
	struct option long_options[] = {
		{ "domain",				required_argument,	0, MRI_CONFIG_DOMAIN					},
		{ "sd-host",			required_argument,	0, MRI_CONFIG_SERVICE_DISCOVERY_HOST	},
		{ "sd-port",			required_argument,	0, MRI_CONFIG_SERVICE_DISCOVERY_PORT	},
		{ "service-discovery",	required_argument,	0, MRI_CONFIG_SERVICE_DISCOVERY_TO_USE	},
		{ 0,					0,					0, 0									}
	};

	/* Fetch first option and loop */
	int c = getopt_long(argc, argv, "", long_options, &option_index);
	while (0 == c) {
		struct option *curr_opt = &(long_options[option_index]);
		MRI_LOG(MRI_DEBUG, "Parsing config-item %s", curr_opt->name);

		int config_item = curr_opt->val;
		if (unlikely(config_item < 0 || config_item >= MRI_CONFIG_MAX_VALUE)) {
			MRI_LOG(MRI_ERROR, "Config-item %s (%d) cannot be parsed (out-of-range)", curr_opt->name, config_item);
			return -1;
		}

		__auto_type callback = handle_config_cb[config_item];
		if (unlikely(! callback)) {
			MRI_LOG(MRI_ERROR, "Config-item %s missing callback handler");
			return -1;
		}

		if (0 != callback(optarg, &(conf->config[config_item]))) {
			MRI_LOG(MRI_WARN, "Config-item %s failed parsing (value = %s) | ignoring", curr_opt->name, optarg);
		} else {
			MRI_LOG(MRI_DEBUG, "Config-item %s parsed successfully", curr_opt->name);
		}

		/* Fetch next option */
		c = getopt_long(argc, argv, "", long_options, &option_index);
	}

	return 0;
}

struct mri_configuration *mri_config_create(int argc, char **argv) {
	struct mri_configuration *conf =
		(struct mri_configuration *) calloc(1, sizeof(struct mri_configuration));
	if (NULL == conf) {
		MRI_LOG(MRI_ERROR, "Failed to allocate memory for configuration (OOM)");
		return NULL;
	}

	/* Parse arguments into configuration */
	if (argc && 0 != parse_opts(conf, argc, argv)) {
		mri_config_destroy(conf);
		return NULL;
	}

	return conf;
}

int mri_config_destroy(struct mri_configuration *config) {
	if (config) free(config);
	return 0;
}

int mri_config_get_mutable(struct mri_configuration *config, mri_config_value_t type, void *output, size_t *size) {
	/* Sanity */
	if (! config || type < 0 || type > MRI_CONFIG_MAX_VALUE) return -1;

	mri_config_data_t *config_item = &(config->config[type]);
	if (! config_item->data || ! config_item->length) {
		MRI_LOG(MRI_ERROR, "Config-item (%s) has no proper data, aborting it", config_type_to_string(type));
		return -1;
	}

	if (*size >= config_item->length) {
		memcpy(output, config_item->data, config_item->length);
		*size = config_item->length;
		return 0;
	}

	MRI_LOG(MRI_WARN,
			"Config-item (%s) will truncate (need %zu bytes, got buffer with %zu bytes)",
			config_type_to_string(type),
			config_item->length,
			*size);
	return -1;
}

int mri_config_get(struct mri_configuration *config, mri_config_value_t type, const void **output, size_t *size) {
	/* Sanity */
	if (! config || type < 0 || type > MRI_CONFIG_MAX_VALUE) return -1;

	mri_config_data_t *config_item = &(config->config[type]);
	*output = config_item->data;
	*size = config_item->length;
	return 0;
}

int mri_config_set_mutable(struct mri_configuration *config, mri_config_value_t type, void *input, size_t size) {
	/* Sanity */
	if (! config || type < 0 || type > MRI_CONFIG_MAX_VALUE || ! input || ! size) return -1;

	mri_config_data_t *config_item = &(config->config[type]);

	/* Allocate data */
	if (! (config_item->data = malloc(size))) {
		MRI_LOG(MRI_ERROR, "Config-item (%s) cannot allocate %zu bytes (OOM)", config_type_to_string(type), size);
		return -1;
	}

	memcpy(config_item->data, input, size);
	config_item->length = size;
	return 0;
}


int mri_config_set(struct mri_configuration *config, mri_config_value_t type, void *input, size_t size) {
	/* Sanity */
	if (! config || type < 0 || type > MRI_CONFIG_MAX_VALUE) return -1;

	mri_config_data_t *config_item = &(config->config[type]);
	config_item->data = input;
	config_item->length = size;
	return 0;
}
