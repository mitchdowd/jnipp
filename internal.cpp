// External Dependencies
#include <jni.h>

// Local Dependencies
#include "internal.h"

namespace jni
{
	// Forward Declarations
	JNIEnv* env();

	namespace internal
	{
		// Base Type Signatures
		template <> String valueSig(const void*)		{ return "V"; }
		template <> String valueSig(const bool*)		{ return "Z"; }
		template <> String valueSig(const wchar_t*)		{ return "C"; }
		template <> String valueSig(const short*)		{ return "S"; }
		template <> String valueSig(const int*)			{ return "I"; }
		template <> String valueSig(const long long*)	{ return "J"; }
		template <> String valueSig(const float*)		{ return "F"; }
		template <> String valueSig(const double*)		{ return "D"; }

		// Base Type Conversions
		template <> void valueArg(value_t* v, const bool& a)		{ ((jvalue*) v)->z = a; }
		template <> void valueArg(value_t* v, const wchar_t& a)		{ ((jvalue*) v)->c = a; }
		template <> void valueArg(value_t* v, const short& a)		{ ((jvalue*) v)->s = a; }
		template <> void valueArg(value_t* v, const int& a)			{ ((jvalue*) v)->i = a; }
		template <> void valueArg(value_t* v, const long long& a)	{ ((jvalue*) v)->j = a; }
		template <> void valueArg(value_t* v, const float& a)		{ ((jvalue*) v)->f = a; }
		template <> void valueArg(value_t* v, const double& a)		{ ((jvalue*) v)->d = a; }
		template <> void valueArg(value_t* v, const jobject& a)		{ ((jvalue*) v)->l = a; }

		/*
			String Implementations
		 */

		template <> String valueSig(const String*)
		{
			return "Ljava/lang/String;";
		}

		template <> void valueArg(value_t* v, const String& a)
		{
			((jvalue*) v)->l = env()->NewStringUTF(a.c_str());
		}
	}
}

