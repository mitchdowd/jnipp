// External Dependencies
#include <jni.h>

// Local Dependencies
#include "jnipp.h"

namespace jni
{
	// Forward Declarations
	JNIEnv* env();

#ifndef _WIN32
    extern std::basic_string<jchar> toJString(const wchar_t* str, size_t length);
#endif

	namespace internal
	{
		// Base Type Conversions
		void valueArg(value_t* v, bool a)			{ ((jvalue*) v)->z = a; }
		void valueArg(value_t* v, wchar_t a)		{ ((jvalue*) v)->c = a; }
		void valueArg(value_t* v, short a)			{ ((jvalue*) v)->s = a; }
		void valueArg(value_t* v, int a)			{ ((jvalue*) v)->i = a; }
		void valueArg(value_t* v, long long a)		{ ((jvalue*) v)->j = a; }
		void valueArg(value_t* v, float a)			{ ((jvalue*) v)->f = a; }
		void valueArg(value_t* v, double a)			{ ((jvalue*) v)->d = a; }
		void valueArg(value_t* v, jobject a)		{ ((jvalue*) v)->l = a; }
		void valueArg(value_t* v, const Object& a)	{ ((jvalue*) v)->l = a.getHandle(); }

		/*
			Object Implementations
		 */
		std::string valueSig(const Object* obj)
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

		void valueArg(value_t* v, const std::string& a)
		{
			((jvalue*) v)->l = env()->NewStringUTF(a.c_str());
		}

		template <> void cleanupArg<std::string>(value_t* v)
		{
			env()->DeleteLocalRef(((jvalue*) v)->l);
		}

		void valueArg(value_t* v, const char* a)
		{
			((jvalue*) v)->l = env()->NewStringUTF(a);
		}

		template <> void cleanupArg<const char*>(value_t* v)
		{
			env()->DeleteLocalRef(((jvalue*) v)->l);
		}

#ifdef _WIN32

		void valueArg(value_t* v, const std::wstring& a)
		{
			((jvalue*)v)->l = env()->NewString((const jchar*) a.c_str(), jsize(a.length()));
		}

		void valueArg(value_t* v, const wchar_t* a)
		{
			((jvalue*) v)->l = env()->NewString((const jchar*) a, jsize(std::wcslen(a)));
		}
#else

		void valueArg(value_t* v, const std::wstring& a)
		{
			auto jstr = toJString(a.c_str(), a.length());
			((jvalue*) v)->l = env()->NewString(jstr.c_str(), jsize(jstr.length()));
		}

		void valueArg(value_t* v, const wchar_t* a)
		{
			auto jstr = toJString(a, std::wcslen(a));
			((jvalue*) v)->l = env()->NewString(jstr.c_str(), jsize(jstr.length()));
		}

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

