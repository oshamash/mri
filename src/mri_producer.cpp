#include <map>
#include <memory>
#include <string>
#include <vector>
#include <iterator>
#include <functional>
#include <unordered_map>

#include <errno.h>
#include <string.h>
#include <sys/socket.h>

extern "C" {
	#include "mri_producer.h"
//	#include "dn_common/general.h"
}

#include "mri_data.hpp"
#include "mri_sched.hpp"
#include "mri_general.hpp"
//#include "../deps/ordered_map.h"

/* gettid() is undefined in glibc, use this instead */
#include <sys/syscall.h>
static tid_t _gettid() { return syscall(SYS_gettid); }

/* Global variables */
static mri_xtype_map_t			mri_xtypes;
static mri_shaper_map_t			mri_shapers;
static mri_formatter_map_t		mri_formatters;

/* Root-Node externalized for all */
xpath_node mri_root ("/", nullptr);

#define MRI_MAP_INSERT(map, key, ...)																	\
	/* Note: ignore potential copy, even by using std::move, compiler optimization will fix this */		\
	/* Note: usage of m_name.c_str() as the map-key allows us to avoid another string allocation */		\
	auto __temp_cls = std::make_unique<decltype(map)::mapped_type::element_type>(key, ##__VA_ARGS__);	\
	auto&& [iter, did_insert] = map.try_emplace(__temp_cls->m_name.c_str(), std::move(__temp_cls))

#define MRI_MAP_INSERT_CHECK_RES(exists_err, map_data_t, map, key)										\
	do {																								\
		if (unlikely(! did_insert)) {																	\
			if (map.end() == iter) {																	\
				MRI_LOG(MRI_ERROR, "Failed to insert " #key " %s " #map_data_t " (OOM) (ˆ⺫ˆ๑)<3",key);	\
				return -ENOMEM;																			\
			}																							\
																										\
			MRI_LOG(MRI_WARNING, #map_data_t " %s already exists", key);								\
			return exists_err;																			\
		}																								\
	} while (0)

#define MRI_SAFE_MAP_INSERTION(exists_err, map_data_t, map, key, ...)									\
	MRI_MAP_INSERT(map, key, ##__VA_ARGS__);															\
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
	return 0;
}

extern "C" {
	/* ---------------------------------------------------------------------------------------------
	 * 							Delegators to map user-defined format/shaper
	 * ---------------------------------------------------------------------------------------------
	 */

	int _mri_create_formatter(cstring_t formatter, mri_formatter_cb callback) {
		/* Sanity check */
		if (! formatter || ! callback) return -EINVAL;

		MRI_SAFE_MAP_INSERTION(	-EEXIST,
								xformat,
								mri_formatters,
								formatter,
								callback);
		return 0;
	}

	int _mri_create_shaper(cstring_t shaper, mri_shaper_cb callback) {
		/* Sanity check */
		if (! shaper || ! callback) return -EINVAL;

		MRI_SAFE_MAP_INSERTION(	-EEXIST,
								xshaper,
								mri_shapers,
								shaper,
								callback);
		return 0;
	}

	/* ---------------------------------------------------------------------------------------------
	 * 									Custom type creation functions
	 * ---------------------------------------------------------------------------------------------
	 */

	int _mri_create_type(cstring_t type, size_t size) {
		/* Sanity check */
		if (! type) return -EINVAL;

		MRI_SAFE_MAP_INSERTION(	0,
								xtype,
								mri_xtypes,
								type,
								size);
		return 0;
	}


	int _mri_type_add_slot(	cstring_t	type,
							cstring_t	name,
							cstring_t	formatter,
							size_t		offset,
							size_t		size,
							int			slot_flags) {
		/* Sanity check */
		if (! type || ! name || ! formatter) return -EINVAL;

		auto xtype_iter = mri_xtypes.find(type);
		if (unlikely(mri_xtypes.end() == xtype_iter)) {
			MRI_LOG(MRI_ERROR, "xtype %s does not exist, better luck next time (҂◡_◡) ᕤ", type);
			return -EINVAL;
		}

		auto xformat_iter = mri_formatters.find(formatter);
		if (unlikely(mri_formatters.end() == xformat_iter)) {
			MRI_LOG(MRI_ERROR, "xformat %s does not exist, better luck next time (҂◡_◡) ᕤ", formatter);
			return -EINVAL;
		}

		auto &mri_type = xtype_iter->second;
		auto const &format = xformat_iter->second;
		return mri_type->add_slot(name, offset, size, format->m_formatter, slot_flags);
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
		/* Sanity check */
		if (! type || ! name || ! callback) return -EINVAL;

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
		return mri_type->add_slot(name, 0, mri_type->m_size, format->m_formatter, 0);
	}

	int _mri_type_add_shaper(	cstring_t	type,
								cstring_t	slot,
								cstring_t	shaper,
								size_t		offset,
								size_t		size) {
		/* Sanity check */
		if (! type || ! slot || ! shaper) return -EINVAL;

		auto xtype_iter = mri_xtypes.find(type);
		if (unlikely(mri_xtypes.end() == xtype_iter)) {
			MRI_LOG(MRI_ERROR, "xtype %s does not exist, better luck next time (҂◡_◡) ᕤ", type);
			return -EINVAL;
		}

		auto shaper_iter = mri_shapers.find(shaper);
		if (mri_shapers.end() == shaper_iter) {
			MRI_LOG(MRI_ERROR, "shaper %s does not exist, better luck next time (҂◡_◡) ᕤ", shaper);
			return -EINVAL;
		}

		auto &mri_type = xtype_iter->second;

		/* Add hidden slot for shaping (allow for user to add it as not-hidden before) */
		int retval = mri_type->add_slot(slot, offset, size, nullptr, MRI_SLOT_FLAG_HIDDEN);
		if (unlikely(0 != retval && -EEXIST != retval)) return retval;

		/* Save shaper to use later in xdump */
		auto const &xshaper = shaper_iter->second;
		auto &xslot = mri_type->m_slots.find(slot)->second;
		xslot->m_shaper = xshaper->m_shaper;
		auto __temp_cls = std::make_unique<slot_capture_data>();
		auto&& [iter, did_insert] = mri_type->m_slot_capture_map.try_emplace(slot, std::move(__temp_cls));
		MRI_MAP_INSERT_CHECK_RES(-EEXIST, slot_capture_data, mri_type->m_slot_capture_map, slot);
		return 0;
	}

	/* ---------------------------------------------------------------------------------------------
	 * 								Event-loop and timing related functions
	 * ---------------------------------------------------------------------------------------------
	 */
	int mri_set_current_thread_sched(mri_sched_info_t *sched) {
		/* Sanity check */
		if (! sched || ! sched->register_event || ! sched->unregister_event) return -EINVAL;

		/* Register sub-scheduler for current thread's ID */
		return mri_main_sched::instance().register_subsched(_gettid(), *sched) ? 0 : -1;
	}

	/* ---------------------------------------------------------------------------------------------
	 * 							Custom user-data registration related functions
	 * ---------------------------------------------------------------------------------------------
	 */

	xpath_node *mri_create_path_from_root(cstring_t path) {
		/* Sanity */
		if ('/' != *path) {
			MRI_LOG(MRI_ERROR, "Path (%s) does not start at root ୧༼ಠ益ಠ༽୨", path);
			return nullptr;
		}

		xpath_node *node = &mri_root;
		cstring_t iter = path + 1, last_node = path;
		for (; *iter; ++iter) {
			/* Note:
			 *	Node starts after last '/' (<last_node> + 1)
			 *	and ends before following '/' (<iter> - 1) 
			 */
			if ('/' == *iter) {
				/* Calculate length of node */
				intptr_t str_len = (intptr_t) iter - (intptr_t) last_node - 1;
				
				/* Skip empty nodes (non-strict format) */
				if (0 >= str_len) {
					++last_node;
					continue;
				}

				/* Actual name of the node */
				std::string_view node_name { last_node + 1, (size_t) str_len };

				/* Update current node indicator to new subnode */
				node = (xpath_node *) node->fetch_subnode(node_name);
				if (! node) {
					/* Error already logged by fetch_subnode() */
					/* TODO(omer): Support rollback ? */
					return nullptr;
				}

				/* Update last */
				last_node = iter;
			}
		}

		/* Fetch the leaf - actual work needed */

		/* Calculate length of node */
		intptr_t str_len = (intptr_t) iter - (intptr_t) last_node - 1;
		
		/* Last node is empty -> return last known (non-strict) */
		if (0 >= str_len) return node;

		std::string_view node_name { last_node + 1, (size_t) str_len };
		node = (xpath_node *) node->fetch_subnode(node_name);
		if (! node) {
			/* Error already logged by fetch_subnode() */
			/* TODO(omer): Support rollback ? */
			return nullptr;
		}

		return node;
	}

	int _mri_register(cstring_t path, cstring_t type, void *object, mri_iterator_cb callback) {
		if (! path || ! type || ! callback) return -EINVAL;

		/* Find relevant type */
		auto xtype_iter = mri_xtypes.find(type);
		if (unlikely(mri_xtypes.end() == xtype_iter)) {
			MRI_LOG(MRI_ERROR, "xtype %s does not exist, better luck next time (҂◡_◡) ᕤ", type);
			return -EINVAL;
		}

		/* Get underlying node (creating path in the process) */
		xpath_node *leaf_node = mri_create_path_from_root(path);
		if (! leaf_node) {
			/* Error already logged by internal call to fetch_subnode() */
			return -ENOMEM;
		}

		/* Fill in leaf data (get-format) */
		leaf_node->m_get_data = xnode_get {
			.m_tid				= _gettid(),
			.m_type				= xtype_iter->second.get(),
			.m_xdata			= object,
			.m_xdata_iterator	= callback
		};

		return 0;
	}
	int mri_register_config(cstring_t path, void *context, mri_config_change_cb callback);
	int mri_unregister(cstring_t path);
}
