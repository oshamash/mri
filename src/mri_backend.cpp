#include <map>
#include <string>
#include <functional>
#include <unordered_map>

/* TODO(omer): Fix include paths */
extern "C" {
	#include "../include/mri.h"
//	#include "dn_common/general.h"
}

#include "general.hpp"
//#include "../deps/ordered_map.h"

/* TODO(omer): Change to something real */
#define mri_assert(cond) (void) 0

/* gettid() is undefined in glibc, use this instead */
#include <unistd.h>
#include <sys/syscall.h>
typedef pid_t tid_t;
static tid_t _gettid() { return syscall(SYS_gettid); }

/* Declarations (should move soon) */
struct xtype;
struct xformat;
struct xshaper;
struct mri_slot;
struct xpath_node;

using mri_slot_map_t = std::map<cstring_t, mri_slot>;
using mri_subpath_map_t = std::map<cstring_t, xpath_node>;
using mri_capture_map_t = std::unordered_map<cstring_t, std::vector<mri_capture_sample_t>>;
using mri_formatter_map_t = std::unordered_map<cstring_t, xformat>;
using mri_shaper_map_t = std::unordered_map<cstring_t, xshaper>;
using mri_xtype_map_t = std::unordered_map<cstring_t, xtype>;

struct xformat {
	std::string			m_name;
	mri_formatter_cb	m_formatter;

	template <typename string_t>
	xformat(string_t &&name, mri_formatter_cb formatter)
	: m_name(std::forward<string_t>(name))
	, m_formatter(formatter)
	{}
};

struct xshaper {
	std::string		m_name;
	mri_shaper_cb	m_shaper;

	template <typename string_t>
	xshaper(string_t &&name, mri_shaper_cb shaper)
	: m_name(std::forward<string_t>(name))
	, m_shaper(shaper)
	{}
};

/**
 * MRI_SLOT
 * Member of a registered user-defined data structure
 */
struct mri_slot {
	std::string			m_name;			/*!< Name of the slot (unique per-xtype)	*/
	size_t				m_offset;		/*!< Offset of the slot's data in xtype		*/
	size_t				m_size;			/*!< Size of the slot (used by formatter)	*/
	mri_formatter_cb	m_formatter;	/*!< Reference to formatter (int, str, ...)	*/
	mri_shaper_cb		m_shaper;		/*!< Pointer to slot data-samples shaper_cb	*/
	bool				m_is_hidden;	/*!< Indicator for external usage (to-show)	*/

	/* TODO(omer): Find a better way (m_shaper doubles as is-slot-shaper indicator) */
	/* Note: m_shaper will be filled externally for xpath_node::xtype::dump to use	*/
	template <typename string_t>
	mri_slot(	string_t			&&name,
				size_t				offset,
				size_t				size,
				mri_formatter_cb	formatter,
				bool				hidden)
	: m_name(std::forward<string_t>(name))
	, m_offset(offset)
	, m_size(size)
	, m_formatter(formatter)
	, m_shaper(nullptr)
	, m_is_hidden(hidden)
	{}
};

/* TODO(omer): Consider enabling a flat-parse for an xtype (slot, slot, slot, ...) */

/**
 * XTYPE
 * Type which is composed from multiple slots (complex type)
 * Those types are usually being registered to one or more xpath_node
 */
struct xtype {
	std::string			m_name;				/*!< Name of this X-Type (unique)			*/
	size_t				m_size;				/*!< Size needed to allocate the type		*/
	mri_slot_map_t		m_slots;			/*!< Ordered mapping of all type's slots	*/
	struct mri_slot		*m_private_key;		/*!< Explicit pointer to the type's PK slot	*/
	mri_capture_map_t	m_slot_capture_map;	/*!< Externalize the per-slot data captures	*/

	template <typename string_t>
	xtype(string_t &&name, size_t size)
	: m_name(std::forward<string_t>(name))
	, m_size(size)
	, m_slots()
	, m_private_key(nullptr)
	, m_slot_capture_map()
	{}

	/**
	 * Create a slot (data-member)
	 *	@param name - name of the new slot
	 *	@param offset - byte offset in data struct (offsetof)
	 *	@param size - size in bytes of contained data (sizeof)
	 *	@param formatter - format callback for the new slot (to_string)
	 *	@param flags - bitwise flags of slot options (pk, rate, hidden, etc...)
	 *	@return C-Style boolean (0 = OK, <0 = ERROR) (-ERRNO)
	 */
	int add_slot(cstring_t name, size_t offset, size_t size, mri_formatter_cb formatter, int flags);
};

/**
 * xpath_node
 *	Chains of nodes defining path (/ is root)
 */
struct xpath_node {
	/* General definition of a node */
	std::string			m_name;				/*>! Name of node (name unique per parent)	*/
	mri_subpath_map_t	m_sub_nodes;		/*>! Mapping of sub-nodes (each is unique)	*/
	xtype				*m_type;			/*>! Type coresponding with the node's data	*/
	xpath_node			*m_parent;			/*>! Reverse reference to the parent's node	*/

	/* User-data (specified on registration) */
	tid_t				m_tid;				/*>! Thread whom registered node (origin)	*/
	void				*m_xdata;			/*>! User-defined data (node data internal)	*/
	mri_iterator_cb		m_xdata_iterator;	/*>! User-defined data iterator (tableview)	*/

	/**
	 * Notes:
	 *	1. No type = empty node, path-building
	 *	2. No parent = root node, others should not do this
	 *	3. No data = only allowed for empty node
	 *	4. Iterator = optional, data can be flat
	 *	5. Name cannot contain / (except root) to allow path traversal
	 *	6. Path is defined as <ROOT>/<NODE-1>/<NODE-2>/.../<NODE-X>
	 */
	template <typename string_t>
	xpath_node(	string_t		&&name,
				xtype			*type,
				xpath_node		*parent,
				void			*xdata,
				mri_iterator_cb	data_iter)
	: m_name(std::forward<string_t>(name))
	, m_sub_nodes()
	, m_type(type)
	, m_parent(parent)
	, m_tid(_gettid())
	, m_xdata(xdata)
	, m_xdata_iterator(data_iter)
	{ mri_assert(parent || ! data);  }
};

/* Global variables */
static mri_xtype_map_t		mri_xtypes;
static mri_shaper_map_t		mri_shapers;
static mri_formatter_map_t	mri_formatters;
static xpath_node			mri_root ("/", nullptr, nullptr, nullptr, nullptr);

#define MRI_MAP_INSERT(map, key, ...)																\
	/* Note: ignore potential copy, even by using std::move, compiler optimization will fix this */	\
	/* Note: usage of m_name.c_str() as the map-key allows us to avoid another string allocation */	\
	decltype(map)::mapped_type __temp_cls (key, ##__VA_ARGS__);										\
	auto&& [iter, did_insert] = map.try_emplace(__temp_cls.m_name.c_str(), std::move(__temp_cls))	\

#define MRI_SAFE_MAP_INSERTION(exists_err, map_data_t, map, key, ...)								\
	MRI_MAP_INSERT(map, key, ##__VA_ARGS__);														\
																									\
	if (likely(did_insert)) return 0; /* Bailout if all worked */									\
																									\
	if (map.end() == iter) {																		\
		MRI_LOG(MRI_ERROR, "Failed to insert " #key " %s " #map_data_t " (OOM)", key);				\
		return -ENOMEM;																				\
	}																								\
																									\
	MRI_LOG(MRI_WARNING, #map_data_t " %s already exists", key);									\
	return exists_err

int xtype::add_slot(cstring_t name, size_t offset, size_t size, mri_formatter_cb formatter, int flags) {
	(void) name;
	(void) offset;
	(void) size;
	(void) formatter;
	(void) flags;

	return 0;
}

extern "C" {

	int mri_init(cstring_t domain) {
		MRI_SAFE_MAP_INSERTION(	0,
								xpath_node,
								mri_root.m_sub_nodes,
								domain,
								nullptr,
								&mri_root,
								nullptr,
								nullptr);
	}

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
								cstring_t	name,
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
		int retval = mri_type.add_slot(name, offset, size, nullptr, MRI_SLOT_FLAG_HIDDEN);
		if (unlikely(0 != retval && -EEXIST != retval)) return retval;

		/* Save shaper to use later in xdump */
		auto const &xshaper = shaper_iter->second;
		auto xslot_iter = mri_type.m_slots.find(name);
		xslot_iter->second.m_shaper = xshaper.m_shaper;

		return 0;
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
