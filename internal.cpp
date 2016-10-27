// External Dependencies
#include <jni.h>

// Local Dependencies
#include "jnipp.h"

namespace jni
{
	// Forward Declarations
	JNIEnv* env();

	namespace internal
	{
		// Base Type Signatures
		template <> std::string valueSig(const void*)			{ return "V"; }
		template <> std::string valueSig(const bool*)			{ return "Z"; }
		template <> std::string valueSig(const wchar_t*)		{ return "C"; }
		template <> std::string valueSig(const short*)			{ return "S"; }
		template <> std::string valueSig(const int*)			{ return "I"; }
		template <> std::string valueSig(const long long*)		{ return "J"; }
		template <> std::string valueSig(const float*)			{ return "F"; }
		template <> std::string valueSig(const double*)			{ return "D"; }
		template <> std::string valueSig(const std::string*)	{ return "Ljava/lang/String;"; }
		template <> std::string valueSig(const std::wstring*)	{ return "Ljava/lang/String;"; }
		template <> std::string valueSig(const char* const*)	{ return "Ljava/lang/String;"; }
		template <> std::string valueSig(const wchar_t* const*)	{ return "Ljava/lang/String;"; }

		// Base Type Conversions
		template <> void valueArg(value_t* v, const bool* a)		{ ((jvalue*) v)->z = *a; }
		template <> void valueArg(value_t* v, const wchar_t* a)		{ ((jvalue*) v)->c = *a; }
		template <> void valueArg(value_t* v, const short* a)		{ ((jvalue*) v)->s = *a; }
		template <> void valueArg(value_t* v, const int* a)			{ ((jvalue*) v)->i = *a; }
		template <> void valueArg(value_t* v, const long long* a)	{ ((jvalue*) v)->j = *a; }
		template <> void valueArg(value_t* v, const float* a)		{ ((jvalue*) v)->f = *a; }
		template <> void valueArg(value_t* v, const double* a)		{ ((jvalue*) v)->d = *a; }
		template <> void valueArg(value_t* v, const jobject* a)		{ ((jvalue*) v)->l = *a; }
		template <> void valueArg(value_t* v, const Object* a)		{ ((jvalue*) v)->l = a->getHandle(); }

		/*
			Object Implementations
		 */
		template <> std::string valueSig(const Object* obj)
		{
			if (obj == nullptr || obj->isNull())
				return "Ljava/lang/Object;";	// One can always hope...

			std::string name = Class(obj->getClass(), Object::Temporary).getName();
			
			// Change from "java.lang.Object" format to "java/lang/Object";
			for (size_t i = 0; i < name.length(); ++i)
				if (name[i] == '.')
					name[i] = '/';

			return "L" + name + ";";
		}

		/*
			String Implementations
		 */

		template <> void valueArg(value_t* v, const std::string* a)
		{
			((jvalue*) v)->l = env()->NewStringUTF(a->c_str());
		}

		template <> void cleanupArg<std::string>(value_t* v)
		{
			env()->DeleteLocalRef(((jvalue*) v)->l);
		}

		template <> void valueArg(value_t* v, const char* const* a)
		{
			((jvalue*) v)->l = env()->NewStringUTF(*a);
		}

		template <> void cleanupArg<const char*>(value_t* v)
		{
			env()->DeleteLocalRef(((jvalue*) v)->l);
		}

#ifdef _WIN32

		template <> void valueArg(value_t* v, const std::wstring* a)
		{
			((jvalue*)v)->l = env()->NewString((const jchar*) a->c_str(), jsize(a->length()));
		}

		template <> void valueArg(value_t* v, const wchar_t* const* a)
		{
			((jvalue*) v)->l = env()->NewString((const jchar*) *a, jsize(std::wcslen(*a)));
		}
#else
# error "32-bit character support not yet implemented"
#endif

		template <> void cleanupArg<const std::wstring*>(value_t* v)
		{
			env()->DeleteLocalRef(((jvalue*)v)->l);
		}

		template <> void cleanupArg<const wchar_t*>(value_t* v)
		{
			env()->DeleteLocalRef(((jvalue*)v)->l);
		}
	}
}

