#ifndef _MRI_SERVICE_DISCOVERY_HPP_
#define _MRI_SERVICE_DISCOVERY_HPP_

extern "C" {
	#include "mri_config.h"
}
#include "mri_typing.hpp"

/* This will change everytime we change the mri_service_data structure */
#define MRI_SERVICE_DISCOERY_PROTOCOL_VERSION 1
struct mri_service_data {
	uint32_t			version;					/*!< service data versioning first !	*/
	char				host[MRI_MAX_HOST_SIZE];	/*!< host containing the MRI instance	*/
	mri_service_mode_t	mode;						/*!< MRI usage mode (producer/consumer)	*/
};

/* Data describing current service */
extern mri_service_data self;

using mri_service_map = mri_ordered_map<cstring_t, mri_service_data>;

class IServiceDiscovery {
protected:
	IServiceDiscovery() = default;
public:
	IServiceDiscovery(IServiceDiscovery&)	= delete;
	IServiceDiscovery(IServiceDiscovery&&)	= delete;

	/* Responsible add `self` if it decides that it is needed */
	virtual bool initialize(mri_configuration &mri_config) = 0;
	virtual mri_service_map &fetch_services() = 0;
};

#endif
