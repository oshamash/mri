#include <cstring>

extern "C" {
	#include "mri.h"
};

/**
 * Generic template to form any iteration of given container
 *
 *	@tparam container_t	Container-type to register.
 *	@tparam iterator_t	Value Iterator-type to register.
 *	@tparam value_t		Value-type to register.
 *
 *	Other parameters as defined in typedef mri_iterator_cb
 */
template <	typename container_t,
			typename iterator_t	= typename container_t::iterator,
			typename value_t	= typename container_t::value_type	>
int generic_mri_iterator_cb(void *container, mri_iter_state_t *state, void **mem) {
	/* We are getting the templated type as data */
	auto const &data_view = *((container_t *) container);

	/* Check for first run */
	if (0 == state->iteration_count) {
		/* Deep-copy since iterators don't have copy .ctor */
		auto begin_iterator = data_view.begin();

		/**
		 * Notice we disable the warning because normally one cannot copy-construct an iterator
		 * This is purely a hack we use to avoid the need for having an in-container dedicated iterator
		 */
		#pragma GCC diagnostic push
		#if __GNUC__ >= 8
			#pragma GCC diagnostic ignored "-Wclass-memaccess"
		#endif
		std::memcpy(&state->userdata, &begin_iterator, sizeof(iterator_t));
		#pragma GCC diagnostic pop
	}

	/* Parse private data as iterator (see above)  */
	auto &iterator = *((iterator_t *) &state->userdata);

	// If we are at the last argument -> restart iterator
	if (data_view.end() == iterator) {
		return MRI_ITER_STOP;
	}

	/* Set row memory from iterator using the operator */
	*mem = &(* iterator++); /* pointer to data reference */

	return MRI_ITER_CONTINUE;
}
