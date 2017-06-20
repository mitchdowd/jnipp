#ifdef _WIN32
# define WIN32_LEAN_AND_MEAN 1

  // Windows Dependencies
# include <windows.h>
#else
  // UNIX Dependencies
# include <dlfcn.h>
#endif

// External Dependencies
#include <jni.h>

// Standard Dependencies
#include <atomic>
#include <string>

// Local Dependencies
#include "jnipp.h"

namespace jni
{
	// Static Variables
	static std::atomic_bool isVm(false);
	static JavaVM* javaVm = nullptr;

	/**
		Maintains the lifecycle of a JNIEnv.
	 */
	class ScopedEnv final
	{
	public:
		ScopedEnv() : _vm(nullptr), _env(nullptr), _attached(false) {}
		~ScopedEnv();

		void init(JavaVM* vm);
		JNIEnv* get() const { return _env; }

	private:
		// Instance Variables
		JavaVM* _vm;
		JNIEnv* _env;
		bool    _attached;	///< Manually attached, as opposed to already attached.
	};

	ScopedEnv::~ScopedEnv()
	{
		if (_vm && _attached)
			_vm->DetachCurrentThread();
	}

	void ScopedEnv::init(JavaVM* vm)
	{
		if (_env != nullptr)
			return;

		if (vm == nullptr)
			throw InitializationException("JNI not initialized");

		if (vm->GetEnv((void**) &_env, JNI_VERSION_1_2) != JNI_OK)
		{
#ifdef __ANDROID__
			if (vm->AttachCurrentThread(&_env, nullptr) != 0)
#else
			if (vm->AttachCurrentThread((void**) &_env, nullptr) != 0)
#endif
				throw InitializationException("Could not attach JNI to thread");

			_attached = true;
		}

		_vm = vm;
	}

	/*
		Helper Functions
	 */

#ifndef _WIN32

	std::wstring toWString(const jchar* str, jsize length)
	{
		std::wstring result;

		result.reserve(length);

		for (jsize i = 0; i < length; ++i)
		{
			wchar_t ch = str[i];

			// Check for a two-segment character.
			if (ch >= wchar_t(0xD800) && ch <= wchar_t(0xDBFF)) {
				if (i + 1 >= length)
					break;

				// Create a single, 32-bit character.
				ch = (ch - wchar_t(0xD800)) << 10;
				ch += str[i++] - wchar_t(0x1DC00);
			}

			result += ch;
		}

		return result;
	}

	std::basic_string<jchar> toJString(const wchar_t* str, size_t length)
	{
		std::basic_string<jchar> result;

		result.reserve(length * 2);	// Worst case scenario.

		for (size_t i = 0; i < length; ++i)
		{
			wchar_t ch = str[i];

			// Check for multi-byte UTF-16 character.
			if (ch > wchar_t(0xFFFF)) {
				ch -= uint32_t(0x10000);

				// Add the first of the two-segment character.
				result += jchar(0xD800 + (ch >> 10));
				ch = wchar_t(0xDC00) + (ch & 0x03FF);
			}

			result += jchar(ch);
		}

		return result;
	}

#endif // _WIN32

	JNIEnv* env()
	{
		static thread_local ScopedEnv env;

		if (env.get() == nullptr)
			env.init(javaVm);

		return env.get();
	}

	static jclass findClass(const char* name)
	{
		jclass ref = env()->FindClass(name);

		if (ref == NULL)
			throw NameResolutionException(name);

		return ref;
	}

	static void handleJavaExceptions()
	{
		JNIEnv* env = jni::env();

		jthrowable exception = env->ExceptionOccurred();

		if (exception != NULL)
		{
			Object obj(exception, Object::Temporary);

			env->ExceptionClear();
			std::string msg = obj.call<std::string>("toString");
			throw InvocationException(msg.c_str());
		}
	}

	static std::string toString(jobject handle, bool deleteLocal = true)
	{
		std::string result;

		if (handle != NULL)
		{
			JNIEnv* env = jni::env();

			const char* chars = env->GetStringUTFChars(jstring(handle), NULL);
			result.assign(chars, env->GetStringUTFLength(jstring(handle)));
			env->ReleaseStringUTFChars(jstring(handle), chars);

			if (deleteLocal)
				env->DeleteLocalRef(handle);
		}

		return result;
	}

	static std::wstring toWString(jobject handle, bool deleteLocal = true)
	{
		std::wstring result;

		if (handle != NULL)
		{
			JNIEnv* env = jni::env();

			const jchar* chars = env->GetStringChars(jstring(handle), NULL);
#ifdef _WIN32
			result.assign((const wchar_t*) chars, env->GetStringLength(jstring(handle)));
#else
			result = toWString(chars, env->GetStringLength(jstring(handle)));
#endif
			env->ReleaseStringChars(jstring(handle), chars);

			if (deleteLocal)
				env->DeleteLocalRef(handle);
		}

		return result;
	}


	/*
		Stand-alone Function Implementations
	 */

	void init(JNIEnv* env)
	{
		bool expected = false;

		if (isVm.compare_exchange_strong(expected, true))
		{
			if (javaVm == nullptr && env->GetJavaVM(&javaVm) != 0)
				throw InitializationException("Could not acquire Java VM");
		}
	}

	/*
		Object Implementation
	 */

	Object::Object() noexcept : _handle(NULL), _class(NULL), _isGlobal(false)
	{
	}

	Object::Object(const Object& other) : _handle(NULL), _class(NULL), _isGlobal(!other.isNull())
	{
		if (!other.isNull())
			_handle = env()->NewGlobalRef(other._handle);
	}

	Object::Object(Object&& other) noexcept : _handle(other._handle), _class(other._class), _isGlobal(other._isGlobal)
	{
		other._handle = NULL;
		other._class  = NULL;
		other._isGlobal = false;
	}

	Object::Object(jobject ref, int scopeFlags) : _handle(ref), _class(NULL), _isGlobal((scopeFlags & Temporary) == 0)
	{
		if (!_isGlobal)
			return;

		JNIEnv* env = jni::env();

		_handle = env->NewGlobalRef(ref);

		if (scopeFlags & DeleteLocalInput)
			env->DeleteLocalRef(ref);
	}

	Object::~Object() noexcept
	{
		JNIEnv* env = jni::env();

		if (_isGlobal)
			env->DeleteGlobalRef(_handle);

		if (_class != NULL)
			env->DeleteGlobalRef(_class);
	}

	Object& Object::operator=(const Object& other)
	{
		if (_handle != other._handle)
		{
			JNIEnv* env = jni::env();

			// Ditch the old reference.
			if (_isGlobal)
				env->DeleteGlobalRef(_handle);
			if (_class != NULL)
				env->DeleteGlobalRef(_class);

			// Assign the new reference.
			if ((_isGlobal = !other.isNull()) != false)
				_handle = env->NewGlobalRef(other._handle);

			_class = NULL;
		}

		return *this;
	}

	bool Object::operator==(const Object& other) const
	{
		return env()->IsSameObject(_handle, other._handle) != JNI_FALSE;
	}

	Object& Object::operator=(Object&& other)
	{
		if (_handle != other._handle)
		{
			JNIEnv* env = jni::env();

			// Ditch the old reference.
			if (_isGlobal)
				env->DeleteGlobalRef(_handle);
			if (_class != NULL)
				env->DeleteGlobalRef(_class);

			// Assign the new reference.
			_handle   = other._handle;
			_isGlobal = other._isGlobal;
			_class    = other._class;

			other._handle   = NULL;
			other._isGlobal = false;
			other._class    = NULL;
		}

		return *this;
	}

	bool Object::isNull() const noexcept
	{
		return _handle == NULL || env()->IsSameObject(_handle, NULL);
	}

	template <> void Object::callMethod(method_t method, internal::value_t* args) const
	{
		env()->CallVoidMethodA(_handle, method, (jvalue*) args);
		handleJavaExceptions();
	}

	template <>	bool Object::callMethod(method_t method, internal::value_t* args) const
	{
		auto result = env()->CallBooleanMethodA(_handle, method, (jvalue*) args);
		handleJavaExceptions();
		return result != 0;
	}

	template <> bool Object::get(field_t field) const
	{
		return env()->GetBooleanField(_handle, field) != 0;
	}

	template <> void Object::set(field_t field, const bool& value)
	{
		env()->SetBooleanField(_handle, field, value);
	}

	template <>	wchar_t Object::callMethod(method_t method, internal::value_t* args) const
	{
		auto result = env()->CallCharMethodA(_handle, method, (jvalue*) args);
		handleJavaExceptions();
		return result;
	}

	template <>	short Object::callMethod(method_t method, internal::value_t* args) const
	{
		auto result = env()->CallShortMethodA(_handle, method, (jvalue*) args);
		handleJavaExceptions();
		return result;
	}

	template <>	int Object::callMethod(method_t method, internal::value_t* args) const
	{
		auto result = env()->CallIntMethodA(_handle, method, (jvalue*) args);
		handleJavaExceptions();
		return result;
	}

	template <>	long long Object::callMethod(method_t method, internal::value_t* args) const
	{
		auto result = env()->CallLongMethodA(_handle, method, (jvalue*) args);
		handleJavaExceptions();
		return result;
	}

	template <>	float Object::callMethod(method_t method, internal::value_t* args) const
	{
		auto result = env()->CallFloatMethodA(_handle, method, (jvalue*) args);
		handleJavaExceptions();
		return result;
	}

	template <>	double Object::callMethod(method_t method, internal::value_t* args) const
	{
		auto result = env()->CallDoubleMethodA(_handle, method, (jvalue*) args);
		handleJavaExceptions();
		return result;
	}

	template <>	std::string Object::callMethod(method_t method, internal::value_t* args) const
	{
		auto result = env()->CallObjectMethodA(_handle, method, (jvalue*) args);
		handleJavaExceptions();
		return toString(result);
	}

	template <>	std::wstring Object::callMethod(method_t method, internal::value_t* args) const
	{
		auto result = env()->CallObjectMethodA(_handle, method, (jvalue*) args);
		handleJavaExceptions();
		return toWString(result);
	}

	template <> jni::Object Object::callMethod(method_t method, internal::value_t* args) const
	{
		auto result = env()->CallObjectMethodA(_handle, method, (jvalue*) args);
		handleJavaExceptions();
		return Object(result, DeleteLocalInput);
	}

	template <> wchar_t Object::get(field_t field) const
	{
		return env()->GetCharField(_handle, field);
	}

	template <> short Object::get(field_t field) const
	{
		return env()->GetShortField(_handle, field);
	}

	template <> int Object::get(field_t field) const
	{
		return env()->GetIntField(_handle, field);
	}

	template <> long long Object::get(field_t field) const
	{
		return env()->GetLongField(_handle, field);
	}

	template <> float Object::get(field_t field) const
	{
		return env()->GetFloatField(_handle, field);
	}

	template <> double Object::get(field_t field) const
	{
		return env()->GetDoubleField(_handle, field);
	}

	template <> std::string Object::get(field_t field) const
	{
		return toString(env()->GetObjectField(_handle, field));
	}

	template <> std::wstring Object::get(field_t field) const
	{
		return toWString(env()->GetObjectField(_handle, field));
	}

	template <> Object Object::get(field_t field) const
	{
		return Object(env()->GetObjectField(_handle, field), DeleteLocalInput);
	}

	template <> void Object::set(field_t field, const wchar_t& value)
	{
		env()->SetCharField(_handle, field, value);
	}

	template <> void Object::set(field_t field, const short& value)
	{
		env()->SetShortField(_handle, field, value);
	}

	template <> void Object::set(field_t field, const int& value)
	{
		env()->SetIntField(_handle, field, value);
	}

	template <> void Object::set(field_t field, const long long& value)
	{
		env()->SetLongField(_handle, field, value);
	}

	template <> void Object::set(field_t field, const float& value)
	{
		env()->SetFloatField(_handle, field, value);
	}

	template <> void Object::set(field_t field, const double& value)
	{
		env()->SetDoubleField(_handle, field, value);
	}

	template <> void Object::set(field_t field, const std::string& value)
	{
		JNIEnv* env = jni::env();

		jobject handle = env->NewStringUTF(value.c_str());
		env->SetObjectField(_handle, field, handle);
		env->DeleteLocalRef(handle);
	}

	template <> void Object::set(field_t field, const std::wstring& value)
	{
		JNIEnv* env = jni::env();

#ifdef _WIN32
		jobject handle = env->NewString((const jchar*) value.c_str(), jsize(value.length()));
#else
        auto jstr = toJString(value.c_str(), value.length());
		jobject handle = env->NewString(jstr.c_str(), jsize(jstr.length()));
#endif
		env->SetObjectField(_handle, field, handle);
		env->DeleteLocalRef(handle);
	}

	template <> void Object::set(field_t field, const wchar_t* const& value)
	{
		JNIEnv* env = jni::env();
#ifdef _WIN32
		jobject handle = env->NewString((const jchar*) value, jsize(std::wcslen(value)));
#else
        auto jstr = toJString(value, std::wcslen(value));
		jobject handle = env->NewString(jstr.c_str(), jsize(jstr.length()));
#endif
		env->SetObjectField(_handle, field, handle);
		env->DeleteLocalRef(handle);
	}

	template <> void Object::set(field_t field, const char* const& value)
	{
		JNIEnv* env = jni::env();

		jobject handle = env->NewStringUTF(value);
		env->SetObjectField(_handle, field, handle);
		env->DeleteLocalRef(handle);
	}

	template <> void Object::set(field_t field, const Object& value)
	{
		env()->SetObjectField(_handle, field, value.getHandle());
	}

	template <> void Object::set(field_t field, const Object* const& value)
	{
		env()->SetObjectField(_handle, field, value ? value->getHandle() : NULL);
	}

	jclass Object::getClass() const
	{
		if (_class == NULL)
		{
			JNIEnv* env = jni::env();

			jclass classRef = env->GetObjectClass(_handle);
			_class = jclass(env->NewGlobalRef(classRef));
			env->DeleteLocalRef(classRef);
		}

		return _class;
	}

	method_t Object::getMethod(const char* name, const char* signature) const
	{
		return Class(getClass(), Temporary).getMethod(name, signature);
	}

	method_t Object::getMethod(const char* nameAndSignature) const
	{
		return Class(getClass(), Temporary).getMethod(nameAndSignature);
	}

	field_t Object::getField(const char* name, const char* signature) const
	{
		return Class(getClass(), Temporary).getField(name, signature);
	}

	/*
		Class Implementation
	 */

	Class::Class(const char* name) : Object(findClass(name), DeleteLocalInput)
	{
	}

	Class::Class(jclass ref, int scopeFlags) : Object(ref, scopeFlags)
	{
	}

	Object Class::newInstance() const
	{
		method_t constructor = getMethod("<init>", "()V");
		jobject obj = env()->NewObject(jclass(getHandle()), constructor);

		handleJavaExceptions();

		return Object(obj, Object::DeleteLocalInput);
	}

	field_t Class::getField(const char* name, const char* signature) const
	{
		jfieldID id = env()->GetFieldID(jclass(getHandle()), name, signature);

		if (id == nullptr)
			throw NameResolutionException(name);

		return id;
	}

	field_t Class::getStaticField(const char* name, const char* signature) const
	{
		jfieldID id = env()->GetStaticFieldID(jclass(getHandle()), name, signature);

		if (id == nullptr)
			throw NameResolutionException(name);

		return id;
	}

	method_t Class::getMethod(const char* name, const char* signature) const
	{
		jmethodID id = env()->GetMethodID(jclass(getHandle()), name, signature);

		if (id == nullptr)
			throw NameResolutionException(name);

		return id;
	}


	method_t Class::getMethod(const char* nameAndSignature) const
	{
		jmethodID id = nullptr;
		const char* sig = std::strchr(nameAndSignature, '(');

		if (sig != NULL)
			return getMethod(std::string(nameAndSignature, sig - nameAndSignature).c_str(), sig);

		if (id == nullptr)
			throw NameResolutionException(nameAndSignature);

		return id;
	}

	method_t Class::getStaticMethod(const char* name, const char* signature) const
	{
		jmethodID id = env()->GetStaticMethodID(jclass(getHandle()), name, signature);

		if (id == nullptr)
			throw NameResolutionException(name);

		return id;
	}

	method_t Class::getStaticMethod(const char* nameAndSignature) const
	{
		jmethodID id = nullptr;
		const char* sig = std::strchr(nameAndSignature, '(');

		if (sig != NULL)
			return getStaticMethod(std::string(nameAndSignature, sig - nameAndSignature).c_str(), sig);

		if (id == nullptr)
			throw NameResolutionException(nameAndSignature);

		return id;
	}

	Class Class::getParent() const
	{
		return Class(env()->GetSuperclass(jclass(getHandle())), DeleteLocalInput);
	}

	std::string Class::getName() const
	{
		return Object::call<std::string>("getName");
	}

	template <> bool Class::get(field_t field) const
	{
		return env()->GetStaticBooleanField(jclass(getHandle()), field) != 0;
	}

	template <> wchar_t Class::get(field_t field) const
	{
		return env()->GetStaticCharField(jclass(getHandle()), field);
	}

	template <> short Class::get(field_t field) const
	{
		return env()->GetStaticShortField(jclass(getHandle()), field);
	}

	template <> int Class::get(field_t field) const
	{
		return env()->GetStaticIntField(jclass(getHandle()), field);
	}

	template <> long long Class::get(field_t field) const
	{
		return env()->GetStaticLongField(jclass(getHandle()), field);
	}

	template <> float Class::get(field_t field) const
	{
		return env()->GetStaticFloatField(jclass(getHandle()), field);
	}

	template <> double Class::get(field_t field) const
	{
		return env()->GetStaticDoubleField(jclass(getHandle()), field);
	}

	template <> std::string Class::get(field_t field) const
	{
		return toString(env()->GetStaticObjectField(jclass(getHandle()), field));
	}

	template <> std::wstring Class::get(field_t field) const
	{
		return toWString(env()->GetStaticObjectField(jclass(getHandle()), field));
	}

	template <> Object Class::get(field_t field) const
	{
		return Object(env()->GetStaticObjectField(jclass(getHandle()), field), DeleteLocalInput);
	}

	template <> void Class::set(field_t field, const bool& value)
	{
		env()->SetStaticBooleanField(jclass(getHandle()), field, value);
	}

	template <> void Class::set(field_t field, const wchar_t& value)
	{
		env()->SetStaticCharField(jclass(getHandle()), field, value);
	}

	template <> void Class::set(field_t field, const short& value)
	{
		env()->SetStaticShortField(jclass(getHandle()), field, value);
	}

	template <> void Class::set(field_t field, const int& value)
	{
		env()->SetStaticIntField(jclass(getHandle()), field, value);
	}

	template <> void Class::set(field_t field, const long long& value)
	{
		env()->SetStaticLongField(jclass(getHandle()), field, value);
	}

	template <> void Class::set(field_t field, const float& value)
	{
		env()->SetStaticFloatField(jclass(getHandle()), field, value);
	}

	template <> void Class::set(field_t field, const double& value)
	{
		env()->SetStaticDoubleField(jclass(getHandle()), field, value);
	}

	template <> void Class::set(field_t field, const Object& value)
	{
		env()->SetStaticObjectField(jclass(getHandle()), field, value.getHandle());
	}

	template <> void Class::set(field_t field, const Object* const& value)
	{
		env()->SetStaticObjectField(jclass(getHandle()), field, value ? value->getHandle() : NULL);
	}

	template <> void Class::set(field_t field, const std::string& value)
	{
		JNIEnv* env = jni::env();

		jobject handle = env->NewStringUTF(value.c_str());
		env->SetStaticObjectField(jclass(getHandle()), field, handle);
		env->DeleteLocalRef(handle);
	}

	template <> void Class::set(field_t field, const std::wstring& value)
	{
		JNIEnv* env = jni::env();

#ifdef _WIN32
		jobject handle = env->NewString((const jchar*) value.c_str(), jsize(value.length()));
#else
        auto jstr = toJString(value.c_str(), value.length());
		jobject handle = env->NewString(jstr.c_str(), jsize(jstr.length()));
#endif
		env->SetStaticObjectField(jclass(getHandle()), field, handle);
		env->DeleteLocalRef(handle);
	}

	template <>	void Class::callStaticMethod(method_t method, internal::value_t* args) const
	{
		env()->CallStaticVoidMethodA(jclass(getHandle()), method, (jvalue*) args);
		handleJavaExceptions();
	}

	template <>	bool Class::callStaticMethod(method_t method, internal::value_t* args) const
	{
		auto result = env()->CallStaticBooleanMethodA(jclass(getHandle()), method, (jvalue*) args);
		handleJavaExceptions();
		return result != 0;
	}

	template <>	wchar_t Class::callStaticMethod(method_t method, internal::value_t* args) const
	{
		auto result = env()->CallStaticCharMethodA(jclass(getHandle()), method, (jvalue*) args);
		handleJavaExceptions();
		return result;
	}

	template <>	short Class::callStaticMethod(method_t method, internal::value_t* args) const
	{
		auto result = env()->CallStaticShortMethodA(jclass(getHandle()), method, (jvalue*) args);
		handleJavaExceptions();
		return result;
	}

	template <>	int Class::callStaticMethod(method_t method, internal::value_t* args) const
	{
		auto result = env()->CallStaticIntMethodA(jclass(getHandle()), method, (jvalue*) args);
		handleJavaExceptions();
		return result;
	}

	template <>	long long Class::callStaticMethod(method_t method, internal::value_t* args) const
	{
		auto result = env()->CallStaticLongMethodA(jclass(getHandle()), method, (jvalue*) args);
		handleJavaExceptions();
		return result;
	}

	template <>	float Class::callStaticMethod(method_t method, internal::value_t* args) const
	{
		auto result = env()->CallStaticFloatMethodA(jclass(getHandle()), method, (jvalue*) args);
		handleJavaExceptions();
		return result;
	}

	template <>	double Class::callStaticMethod(method_t method, internal::value_t* args) const
	{
		auto result = env()->CallStaticDoubleMethodA(jclass(getHandle()), method, (jvalue*) args);
		handleJavaExceptions();
		return result;
	}

	template <>	std::string Class::callStaticMethod(method_t method, internal::value_t* args) const
	{
		auto result = env()->CallStaticObjectMethodA(jclass(getHandle()), method, (jvalue*) args);
		handleJavaExceptions();
		return toString(result);
	}

	template <>	std::wstring Class::callStaticMethod(method_t method, internal::value_t* args) const
	{
		auto result = env()->CallStaticObjectMethodA(jclass(getHandle()), method, (jvalue*) args);
		handleJavaExceptions();
		return toWString(result);
	}

	template <> jni::Object Class::callStaticMethod(method_t method, internal::value_t* args) const
	{
		auto result = env()->CallStaticObjectMethodA(jclass(getHandle()), method, (jvalue*) args);
		handleJavaExceptions();
		return Object(result, DeleteLocalInput);
	}

	template <> void Class::callExactMethod(jobject obj, method_t method, internal::value_t* args) const
	{
		env()->CallNonvirtualVoidMethodA(obj, jclass(getHandle()), method, (jvalue*) args);
		handleJavaExceptions();
	}

	template <>	bool Class::callExactMethod(jobject obj, method_t method, internal::value_t* args) const
	{
		auto result = env()->CallNonvirtualBooleanMethodA(obj, jclass(getHandle()), method, (jvalue*) args);
		handleJavaExceptions();
		return result != 0;
	}

	template <>	wchar_t Class::callExactMethod(jobject obj, method_t method, internal::value_t* args) const
	{
		auto result = env()->CallNonvirtualCharMethodA(obj, jclass(getHandle()), method, (jvalue*) args);
		handleJavaExceptions();
		return result;
	}

	template <>	short Class::callExactMethod(jobject obj, method_t method, internal::value_t* args) const
	{
		auto result = env()->CallNonvirtualShortMethodA(obj, jclass(getHandle()), method, (jvalue*) args);
		handleJavaExceptions();
		return result;
	}

	template <>	int Class::callExactMethod(jobject obj, method_t method, internal::value_t* args) const
	{
		auto result = env()->CallNonvirtualIntMethodA(obj, jclass(getHandle()), method, (jvalue*) args);
		handleJavaExceptions();
		return result;
	}

	template <>	long long Class::callExactMethod(jobject obj, method_t method, internal::value_t* args) const
	{
		auto result = env()->CallNonvirtualLongMethodA(obj, jclass(getHandle()), method, (jvalue*) args);
		handleJavaExceptions();
		return result;
	}

	template <>	float Class::callExactMethod(jobject obj, method_t method, internal::value_t* args) const
	{
		auto result = env()->CallNonvirtualFloatMethodA(obj, jclass(getHandle()), method, (jvalue*) args);
		handleJavaExceptions();
		return result;
	}

	template <>	double Class::callExactMethod(jobject obj, method_t method, internal::value_t* args) const
	{
		auto result = env()->CallNonvirtualDoubleMethodA(obj, jclass(getHandle()), method, (jvalue*) args);
		handleJavaExceptions();
		return result;
	}

	template <>	std::string Class::callExactMethod(jobject obj, method_t method, internal::value_t* args) const
	{
		auto result = env()->CallNonvirtualObjectMethodA(obj, jclass(getHandle()), method, (jvalue*)args);
		handleJavaExceptions();
		return toString(result);
	}

	template <>	std::wstring Class::callExactMethod(jobject obj, method_t method, internal::value_t* args) const
	{
		auto result = env()->CallNonvirtualObjectMethodA(obj, jclass(getHandle()), method, (jvalue*)args);
		handleJavaExceptions();
		return toWString(result);
	}

	template <> Object Class::callExactMethod(jobject obj, method_t method, internal::value_t* args) const
	{
		auto result = env()->CallNonvirtualObjectMethodA(obj, jclass(getHandle()), method, (jvalue*)args);
		handleJavaExceptions();
		return Object(result, DeleteLocalInput);
	}

	Object Class::newObject(method_t constructor, internal::value_t* args) const
	{
		jobject ref = env()->NewObjectA(jclass(getHandle()), constructor, (jvalue*)args);
		handleJavaExceptions();
		return Object(ref, DeleteLocalInput);
	}

	/*
		Enum Implementation
	 */

	Enum::Enum(const char* name) : Class(name)
	{
		_name  = "L";
		_name += name;
		_name += ";";
	}

	Object Enum::get(const char* name) const
	{
		return Class::get<Object>(getStaticField(name, _name.c_str()));
	}

	/*
		Vm Implementation
	 */

	typedef jint (JNICALL *CreateVm_t)(JavaVM**, void**, void*);


	static std::string detectJvmPath()
	{
		std::string result;

#ifdef _WIN32

		BYTE buffer[1024];
		DWORD size = sizeof(buffer);
		HKEY versionKey;

		if (::RegOpenKeyA(HKEY_LOCAL_MACHINE, "Software\\JavaSoft\\Java Runtime Environment\\", &versionKey) == ERROR_SUCCESS)
		{
			if (::RegQueryValueEx(versionKey, "CurrentVersion", NULL, NULL, buffer, &size) == ERROR_SUCCESS)
			{
				HKEY libKey;

				std::string keyName = std::string("Software\\JavaSoft\\Java Runtime Environment\\") + (const char*) buffer;

				::RegCloseKey(versionKey);

				if (::RegOpenKeyA(HKEY_LOCAL_MACHINE, keyName.c_str(), &libKey) == ERROR_SUCCESS)
				{
					size = sizeof(buffer);

					if (::RegQueryValueEx(libKey, "RuntimeLib", NULL, NULL, buffer, &size) == ERROR_SUCCESS)
					{
						result = (const char*) buffer;
					}

					::RegCloseKey(libKey);
				}
			}
		}
#else

		// Best guess so far.
		result = "/usr/lib/jvm/default-java/jre/lib/amd64/server/libjvm.so";

#endif // _WIN32

		return result;
	}

	Vm::Vm(const char* path_)
	{
		bool expected = false;

		std::string path = path_ ? path_ : detectJvmPath();

		if (path.length() == 0)
			throw InitializationException("Could not locate Java Virtual Machine");
		if (!isVm.compare_exchange_strong(expected, true))
			throw InitializationException("Java Virtual Machine already initialized");

		if (javaVm == nullptr)
		{
			JNIEnv* env;
			JavaVMInitArgs args = {};
			args.version = JNI_VERSION_1_2;

#ifdef _WIN32

			HMODULE lib = ::LoadLibraryA(path.c_str());

			if (lib == NULL)
			{
				size_t index = path.rfind("\\client\\");

				// JRE path names switched from "client" to "server" directory.
				if (index != std::string::npos)
				{
					path = path.replace(index, 8, "\\server\\");
					lib = ::LoadLibraryA(path.c_str());
				}

				if (lib == NULL)
				{
					isVm.store(false);
					throw InitializationException("Could not load JVM library");
				}
			}

			CreateVm_t JNI_CreateJavaVM = (CreateVm_t) ::GetProcAddress(lib, "JNI_CreateJavaVM");

			if (JNI_CreateJavaVM == NULL || JNI_CreateJavaVM(&javaVm, (void**) &env, &args) != 0)
			{
				isVm.store(false);
				::FreeLibrary(lib);
				throw InitializationException("Java Virtual Machine failed during creation");
			}

#else

			void* lib = ::dlopen(path.c_str(), RTLD_NOW | RTLD_GLOBAL);

			if (lib == NULL)
			{
				isVm.store(false);
				throw InitializationException("Could not load JVM library");
			}

			CreateVm_t JNI_CreateJavaVM = (CreateVm_t) ::dlsym(lib, "JNI_CreateJavaVM");

			if (JNI_CreateJavaVM == NULL || JNI_CreateJavaVM(&javaVm, (void**) &env, &args) != 0)
			{
				isVm.store(false);
				::dlclose(lib);
				throw InitializationException("Java Virtual Machine failed during creation");
			}

#endif // _WIN32
		}
	}

	Vm::~Vm()
	{
		/*
			Note that you can't ever *really* unload the JavaVM. If you call
			DestroyJavaVM(), you can't then call JNI_CreateJavaVM() again.
			So, instead we just flag it as "gone".
		 */
		isVm.store(false);
	}

	// Forward Declarations
	JNIEnv* env();

#ifndef _WIN32
    extern std::basic_string<jchar> toJString(const wchar_t* str, size_t length);
#endif

	namespace internal
	{
		// Base Type Conversions
		void valueArg(value_t* v, bool a)					{ ((jvalue*) v)->z = jboolean(a); }
		void valueArg(value_t* v, wchar_t a)				{ ((jvalue*) v)->c = jchar(a); }	// Note: Possible truncation.
		void valueArg(value_t* v, short a)					{ ((jvalue*) v)->s = a; }
		void valueArg(value_t* v, int a)					{ ((jvalue*) v)->i = a; }
		void valueArg(value_t* v, long long a)				{ ((jvalue*) v)->j = a; }
		void valueArg(value_t* v, float a)					{ ((jvalue*) v)->f = a; }
		void valueArg(value_t* v, double a)					{ ((jvalue*) v)->d = a; }
		void valueArg(value_t* v, jobject a)				{ ((jvalue*) v)->l = a; }
		void valueArg(value_t* v, const Object& a)			{ ((jvalue*) v)->l = a.getHandle(); }
		void valueArg(value_t* v, const Object* const& a)	{ ((jvalue*) v)->l = a ? a->getHandle() : NULL; }

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
			env()->DeleteLocalRef(((jvalue*) v)->l);
		}

		template <> void cleanupArg<const wchar_t*>(value_t* v)
		{
			env()->DeleteLocalRef(((jvalue*) v)->l);
		}
	}
}

