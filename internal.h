#ifndef _JNIPP_INTERNAL_H_
#define _JNIPP_INTERNAL_H_ 1

// Local Dependencies
#include "types.h"

namespace jni
{
	/**
		This namespace is for messy implementation details only. It is not a part
		of the external API and is subject to change at any time. It is only in a
		header file due to the fact it is required by some template functions.
	 */
	namespace internal
	{
		typedef long long value_t;

		/*
			Signature Generation
		 */

		template <class TArg>
		String valueSig(const TArg*);

		inline String sig() { return ""; }

		template <class TArg, class... TArgs>
		String sig(const TArg& arg, TArgs... args) {
			return valueSig(&arg) + sig(args...);
		}

		/*
			Argument Conversion
		 */

		template <class TArg>
		void valueArg(value_t* value, const TArg& arg);

		inline value_t* args(value_t* values) { return values; }

		template <class TArg, class... TArgs>
		value_t* args(value_t* values, const TArg& arg, TArgs... args) {
			valueArg(values, arg);
			args(values + 1, args...);
			return values;
		}
	}
}

#endif // _JNIPP_INTERNAL_H_

