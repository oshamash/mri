#ifndef _MRI_DATA_HPP_
#define _MRI_DATA_HPP_

#include <string>
#include <stdint.h>
#include <string_view>

/* gettid() is undefined in glibc, use this instead */
#include <unistd.h>
#include <sys/syscall.h>
typedef pid_t tid_t;
static tid_t _gettid() { return syscall(SYS_gettid); }

#include "mri_typing.hpp"

/* TODO(omer): Fix this */
#define mri_assert(cond) (void) 0 

/* Declarations (should move soon) */
struct xtype;
struct xformat;
struct xshaper;
struct mri_slot;
struct xpath_node;
struct slot_capture_data;

using mri_slot_map_t = mri_ordered_map<cstring_t, mri_slot>;
using mri_subpath_map_t = mri_ordered_map<cstring_t, xpath_node>;
using mri_capture_map_t = mri_unordered_map<cstring_t, slot_capture_data>;
using mri_formatter_map_t = mri_unordered_map<cstring_t, xformat>;
using mri_shaper_map_t = mri_unordered_map<cstring_t, xshaper>;
using mri_xtype_map_t = mri_unordered_map<cstring_t, xtype>;

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

struct slot_capture_data {
	mri_vector<mri_capture_sample_t> samples;
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
	 *	@param slot - name of the new slot
	 *	@param offset - byte offset in data struct (offsetof)
	 *	@param size - size in bytes of contained data (sizeof)
	 *	@param formatter - format callback for the new slot (to_string)
	 *	@param flags - bitwise flags of slot options (pk, rate, hidden, etc...)
	 *	@return C-Style boolean (0 = OK, <0 = ERROR) (-ERRNO)
	 */
	int add_slot(cstring_t slot, size_t offset, size_t size, mri_formatter_cb formatter, int flags);
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
	{ mri_assert(parent || ! xdata);  }
};

/* Root node is available for all */
extern xpath_node mri_root;

#endif
