#ifndef _JNIPP_INTERNAL_H_
#define _JNIPP_INTERNAL_H_ 1

// Standard Dependencies
#include <cstring>

// Local Dependencies
#include "types.h"

namespace jni
{
	// Foward Declarations
	class Object;

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

		inline std::string valueSig(const void*)			{ return "V"; }
		inline std::string valueSig(const bool*)			{ return "Z"; }
		inline std::string valueSig(const wchar_t*)			{ return "C"; }
		inline std::string valueSig(const short*)			{ return "S"; }
		inline std::string valueSig(const int*)				{ return "I"; }
		inline std::string valueSig(const long long*)		{ return "J"; }
		inline std::string valueSig(const float*)			{ return "F"; }
		inline std::string valueSig(const double*)			{ return "D"; }
		inline std::string valueSig(const std::string*)		{ return "Ljava/lang/String;"; }
		inline std::string valueSig(const std::wstring*)	{ return "Ljava/lang/String;"; }
		inline std::string valueSig(const char* const*)		{ return "Ljava/lang/String;"; }
		inline std::string valueSig(const wchar_t* const*)	{ return "Ljava/lang/String;"; }
		std::string valueSig(const Object* obj);

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

		void valueArg(value_t* v, bool a);
		void valueArg(value_t* v, wchar_t a);
		void valueArg(value_t* v, short a);
		void valueArg(value_t* v, int a);
		void valueArg(value_t* v, long long a);
		void valueArg(value_t* v, float a);
		void valueArg(value_t* v, double a);
		void valueArg(value_t* v, jobject a);
		void valueArg(value_t* v, const Object& a);
		void valueArg(value_t* v, const std::string& a);
		void valueArg(value_t* v, const char* a);
		void valueArg(value_t* v, const std::wstring& a);
		void valueArg(value_t* v, const wchar_t* a);

		inline void args(value_t*) {}

		template <class TArg, class... TArgs>
		void args(value_t* values, const TArg& arg, const TArgs&... args) {
			valueArg(values, arg);
			internal::args(values + 1, args...);
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
				internal::args(values, args...);
			}

			~ArgArray() {
				cleanupArgs<TArgs...>(values);
			}

			value_t values[sizeof...(TArgs)];
		};
	}
}

#endif // _JNIPP_INTERNAL_H_

