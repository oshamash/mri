#include <string_view>

#include "mri_general.hpp"

enum class mock_enum {
	REDIS_SERVICE_DISCOVERY,
	ETCD_SERVICE_DISCOVERY,
};

/* Export code into mri_config.c properly */

extern "C" {
	#include "mri_config_ext.h"

	/* Note: we hack the void* so CPP compiler is angry, fix this later */
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wstrict-aliasing"
	int config_to_service_discovery(const char *input, mri_config_data_t *output) {
		std::string_view cpp_input(input);
		if ("redis" == cpp_input) {
			/* No need to allocate since enum fits generic pointer size */
			*((mock_enum *) &(output->data)) = mock_enum::REDIS_SERVICE_DISCOVERY;
			output->length = sizeof(mock_enum);
		} else if ("etcd" == cpp_input) {
			/* No need to allocate since enum fits generic pointer size */
			*((mock_enum *) &(output->data)) = mock_enum::ETCD_SERVICE_DISCOVERY;
			output->length = sizeof(mock_enum);
		} else {
			MRI_LOG(MRI_ERROR, "Did not find service discovery option for %s", input);
			return -1;
		}

		return 0;
	}
	#pragma GCC diagnostic pop
}
