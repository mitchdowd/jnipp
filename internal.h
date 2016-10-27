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

		Long story short...  this stuff be messy, yo.
	 */
	namespace internal
	{
		typedef long long value_t;

		/*
			Signature Generation
		 */

		template <class TArg>
		std::string valueSig(const TArg*);
		template <int n, class TArg>
		inline std::string valueSig(const TArg (*)[n]) {
			return valueSig((const TArg* const*) 0);
		}

		inline std::string sig() { return ""; }

		template <class TArg, class... TArgs>
		std::string sig(const TArg& arg, const TArgs&... args) {
			return valueSig((const TArg*) &arg) + sig(args...);
		}

		/*
			Argument Conversion
		 */

		template <class TArg>
		void valueArg(value_t* value, const TArg* arg);
		template <int n, class TArg>
		inline void valueArg(value_t* value, const TArg (*arg)[n]) { 
			valueArg(value, (const TArg* const*) &arg); 
		}

		inline void args(value_t*) {}

		template <class TArg, class... TArgs>
		void args(value_t* values, const TArg& arg, const TArgs&... args) {
			valueArg(values, &arg);
			args(values + 1, args...);
		}

		template <class TArg> void cleanupArg(value_t* value) {}
		template <> void cleanupArg<std::string>(value_t* value);
		template <> void cleanupArg<std::wstring>(value_t* value);
		template <> void cleanupArg<const char*>(value_t* value);
		template <> void cleanupArg<const wchar_t*>(value_t* value);

		template <class TArg = void, class... TArgs>
		void cleanupArgs(value_t* values) {
			cleanupArg<TArg>(values);
			cleanupArgs<TArgs...>(values + 1);
		}

		template <> inline void cleanupArgs<void>(value_t* values) {}

		template <class... TArgs>
		class ArgArray
		{
		public:
			ArgArray(const TArgs&... args) {
				std::memset(this, 0, sizeof(ArgArray<TArgs...>));
				args(values, args...);
			}

			~ArgArray() {
				cleanupArgs<TArgs...>(values);
			}

			value_t values[sizeof...(TArgs)];
		};
	}
}

#endif // _JNIPP_INTERNAL_H_

