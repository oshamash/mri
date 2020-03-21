#include "mri_config.h"
#include "mri_general.h"

#include <stdlib.h>
#include <getopt.h>

typedef struct mri_config_data {
	void	*data;
	size_t	length;
} mri_config_data_t;


struct mri_configuration {
	mri_config_data_t config[MRI_CONFIG_MAX_VALUE];
};

struct mri_configuration *mri_config_create(int argc, char **argv) {
	struct mri_configuration *conf =
		(struct mri_configuration *) calloc(1, sizeof(struct mri_configuration));
	if (NULL == conf) {
		MRI_LOG(MRI_ERROR, "Failed to allocate memory for configuration (OOM)");
		return NULL;
	}

	return conf;
}
