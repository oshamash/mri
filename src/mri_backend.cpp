#include <string>
#include <functional>
#include <string_view>
#include <unordered_map>

#include "../include/mri.h"
#include "../deps/ordered_map.h"

/* TODO(omer): Change to something real */
#define mri_assert(cond) (void) 0

/* Declarations (should move soon) */
struct vslot;
struct xslot;
struct xtype;
struct xflat_type;
struct xpath_node;

/* TODO(omer): Remove */
namespace std { struct string_view; };

template <typename T>
using mri_slot_map_t = tsl::ordered_map<std::string_view, std::reference_wrapper<T>>;
using mri_capture_map_t = std::unordered_map<std::string_view, std::vector<mri_capture_sample_t>>;
using mri_subpath_map_t = tsl::ordered_map<std::string_view, std::reference_wrapper<xpath_node>>;
using mri_formatter_map_t = std::unordered_map<std::string, mri_formatter_cb>;
using mri_shaper_map_t = std::unordered_map<std::string, mri_shaper_cb>;

/**
 * MRI_SLOT
 * Member of a registered user-defined data structure
 */
struct mri_slot {
	std::string			m_name;
	size_t				m_offset;
	xflat_type const	&m_type;
	bool				m_is_hidden;

	template <typename string_t>
	mri_slot(string_t &&name, size_t offset, xflat_type const &slt_type, bool hidden)
	: m_name(std::forward(name))
	, m_offset(offset)
	, m_type(slt_type)
	, m_is_hidden(hidden)
	{}
};

/**
 * XFLAT_TYPE
 * Type which can be formatted into string
 * All slots are flat-typed to avoid complexity
 */
struct xflat_type {
	std::string			m_name;			/*!< Name of this flatten-type (unique)		*/
	size_t				m_size;			/*!< Size of the type (used by m_format_cb)	*/
	mri_formatter_cb	m_format_cb;	/*!< Format callback (transform to string)	*/

	template <typename string_t>
	xflat_type(string_t &&name, size_t size, mri_formatter_cb callback)
	: m_name(std::forward(name))
	, m_size(size),
	, m_format_cb(callback)
	{}
};

/**
 * XTYPE
 */
struct xtype {
	std::string					m_name;				/*!< Name of this X-Type (unique)			*/
	mri_slot_map_t<mri_slot>	m_slots;			/*!< Ordered mapping of all type's slots	*/
	struct mri_slot				*m_private_key;		/*!< Explicit pointer to the type's PK slot	*/
	mri_capture_map_t			m_slot_capture_map;	/*!< Externalize the per-slot data captures	*/

	template <typename string_t>
	xtype(string_t &&name)
	: m_name(std::forward(name))
	{}

	/**
	 * Create a slot (data-member)
	 *	@param name - name of the new slot
	 *	@param offset - byte offset in data struct (offsetof)
	 *	@param type - name of the member type (used to fetch formatter)
	 *	@param flags - bitwise flags of slot options (pk, rate, hidden, etc...)
	 */
	void add_slot(std::string_view name, size_t offset, std::string_view type, int flags=0);

	/**
	 * Create a virtual slot ->
	 *		1. Creates new xflat_type using the custom-callback
	 *		2. Adds a new slot with type of the newly created xflat_type
	 *
	 *	@param name - name of the new virtual slot
	 *	@param callback - user-defined function to format vslot
	 */
	void add_vslot(std::string_view name, mri_formatter_cb callback);
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
	: m_name(std::forward(name))
	, m_sub_nodes()
	, m_type(type)
	, m_parent(parent)
	, m_tid(_gettid())
	, m_xdata(xdata)
	, m_xdata_iterator(data_iter)
	{ mri_assert(parent || ! data);  }
};

/* Global variables */
static mri_shaper_map_t		mri_shapers;
static mri_formatter_map_t	mri_formatters;
static xpath_node			mri_root ("/", nullptr, nullptr, nullptr, nullptr);

