#include <map>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

/* TODO(omer): Fix include paths */
extern "C" {
	#include "mri_producer.h"
//	#include "dn_common/general.h"
}

#include "mri_general.hpp"
//#include "../deps/ordered_map.h"

#include "mri_data.hpp"

/* Global variables */
static mri_xtype_map_t		mri_xtypes;
static mri_shaper_map_t		mri_shapers;
static mri_formatter_map_t	mri_formatters;

/* Root-Node externalized for all */
xpath_node mri_root ("/", nullptr, nullptr, nullptr, nullptr);

#define MRI_MAP_INSERT(map, key, ...)																\
	/* Note: ignore potential copy, even by using std::move, compiler optimization will fix this */	\
	/* Note: usage of m_name.c_str() as the map-key allows us to avoid another string allocation */	\
	decltype(map)::mapped_type __temp_cls (key, ##__VA_ARGS__);										\
	auto&& [iter, did_insert] = map.try_emplace(__temp_cls.m_name.c_str(), std::move(__temp_cls))

#define MRI_MAP_INSERT_CHECK_RES(exists_err, map_data_t, map, key)									\
	if (likely(did_insert)) return 0; /* Bailout if all worked */									\
																									\
	if (map.end() == iter) {																		\
		MRI_LOG(MRI_ERROR, "Failed to insert " #key " %s " #map_data_t " (OOM)", key);				\
		return -ENOMEM;																				\
	}																								\
																									\
	MRI_LOG(MRI_WARNING, #map_data_t " %s already exists", key);									\
	return exists_err

#define MRI_SAFE_MAP_INSERTION(exists_err, map_data_t, map, key, ...)								\
	MRI_MAP_INSERT(map, key, ##__VA_ARGS__);														\
	MRI_MAP_INSERT_CHECK_RES(exists_err, map_data_t, map, key)

/* Easier to implement here due to macros, consider moving */
int xtype::add_slot(cstring_t slot, size_t offset, size_t size, mri_formatter_cb formatter, int flags) {
	MRI_SAFE_MAP_INSERTION(	-EEXIST,
							mri_slot,
							m_slots,
							/* mri_slot construction */
							slot,
							offset,
							size,
							formatter,
							(flags & MRI_SLOT_FLAG_HIDDEN));
}

extern "C" {
	/* ---------------------------------------------------------------------------------------------
	 * 							Delegators to map user-defined format/shaper
	 * ---------------------------------------------------------------------------------------------
	 */

	int _mri_create_formatter(cstring_t formatter, mri_formatter_cb callback) {
		MRI_SAFE_MAP_INSERTION(	-EEXIST,
								xformat,
								mri_formatters,
								formatter,
								callback);
	}

	int _mri_create_shaper(cstring_t shaper, mri_shaper_cb callback) {
		MRI_SAFE_MAP_INSERTION(	-EEXIST,
								xshaper,
								mri_shapers,
								shaper,
								callback);
	}

	/* ---------------------------------------------------------------------------------------------
	 * 									Custom type creation functions
	 * ---------------------------------------------------------------------------------------------
	 */

	int _mri_create_type(cstring_t type, size_t size) {
		MRI_SAFE_MAP_INSERTION(	0,
								xtype,
								mri_xtypes,
								type,
								size);
	}


	int _mri_type_add_slot(	cstring_t	type,
							cstring_t	name,
							cstring_t	formatter,
							size_t		offset,
							size_t		size,
							int			slot_flags) {
		auto xtype_iter = mri_xtypes.find(type);
		if (unlikely(mri_xtypes.end() == xtype_iter)) {
			MRI_LOG(MRI_ERROR, "xtype %s does not exist, better luck next time", type);
			return -EINVAL;
		}

		auto xformat_iter = mri_formatters.find(formatter);
		if (unlikely(mri_formatters.end() == xformat_iter)) {
			MRI_LOG(MRI_ERROR, "xformat %s does not exist, better luck next time", formatter);
			return -EINVAL;
		}

		auto &mri_type = xtype_iter->second;
		auto const &format = xformat_iter->second;
		return mri_type.add_slot(name, offset, size, format.m_formatter, slot_flags);
	}

	/**
	 * Create a virtual slot ->
	 *		1. Creates new xformat using the custom-callback
	 *		2. Adds a new slot with type of the newly created xformat
	 *
	 *	@param name - name of the new virtual slot
	 *	@param callback - user-defined function to format vslot
	 */
	int _mri_type_add_vslot(cstring_t type, cstring_t name, mri_formatter_cb callback) {
		auto xtype_iter = mri_xtypes.find(type);
		if (unlikely(mri_xtypes.end() == xtype_iter)) {
			MRI_LOG(MRI_ERROR, "xtype %s does not exist, better luck next time", type);
			return -EINVAL;
		}

		/* TODO(omer): define this somewhere and do something about snprintf-return */
		char virt_formatter[256];
		(void) snprintf(virt_formatter, sizeof(virt_formatter), "__%s_%s", type, name);

		/* Creates 2 variables: iter, did_insert (emplace result) */
		MRI_MAP_INSERT(mri_formatters, virt_formatter, callback);
		if (unlikely(! did_insert)) {
			if (mri_formatters.end() == iter) {
				MRI_LOG(MRI_ERROR, "Failed to insert virtual-format for %s::%s (OOM)", type, name);
				return -ENOMEM;
			}

			MRI_LOG(MRI_ERROR, "Seems like you already configured vslot (%s::%s), naughty developer ..", type, name);
			return -EEXIST;
		}

		auto const &format = iter->second;
		auto &mri_type = xtype_iter->second;
		return mri_type.add_slot(name, 0, mri_type.m_size, format.m_formatter, 0);
	}

	int _mri_type_add_shaper(	cstring_t	type,
								cstring_t	slot,
								cstring_t	shaper,
								size_t		offset,
								size_t		size) {
		auto xtype_iter = mri_xtypes.find(type);
		if (unlikely(mri_xtypes.end() == xtype_iter)) {
			MRI_LOG(MRI_ERROR, "xtype %s does not exist, better luck next time", type);
			return -EINVAL;
		}

		auto shaper_iter = mri_shapers.find(shaper);
		if (mri_shapers.end() == shaper_iter) {
			MRI_LOG(MRI_ERROR, "shaper %s does not exist, better luck next time", shaper);
			return -EINVAL;
		}

		auto &mri_type = xtype_iter->second;

		/* Add hidden slot for shaping (allow for user to add it as not-hidden before) */
		int retval = mri_type.add_slot(slot, offset, size, nullptr, MRI_SLOT_FLAG_HIDDEN);
		if (unlikely(0 != retval && -EEXIST != retval)) return retval;

		/* Save shaper to use later in xdump */
		auto const &xshaper = shaper_iter->second;
		auto &xslot = mri_type.m_slots.find(slot)->second;
		xslot.m_shaper = xshaper.m_shaper;
		auto&& [iter, did_insert] = mri_type.m_slot_capture_map.try_emplace(slot, slot_capture_data {});
		MRI_MAP_INSERT_CHECK_RES(-EEXIST, slot_capture_data, mri_type.m_slot_capture_map, slot);
	}

	/* ---------------------------------------------------------------------------------------------
	 * 								Event-loop and timing related functions
	 * ---------------------------------------------------------------------------------------------
	 */

	int mri_get_eventloop_fd();
	int mri_get_notification_fd();

	/* ---------------------------------------------------------------------------------------------
	 * 							Custom user-data registration related functions
	 * ---------------------------------------------------------------------------------------------
	 */

	int _mri_register(cstring_t path, cstring_t type, void *object, mri_iterator_cb callback);
	int mri_register_config(cstring_t path, void *context, mri_config_change_cb callback);
	int mri_unregister(cstring_t path);
}
