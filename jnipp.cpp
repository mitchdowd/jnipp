#ifdef _WIN32
# include <windows.h>
#else
# error "Platform not supported"
#endif

// External Dependencies
#include <jni.h>

// Standard Dependencies
#include <string>

// Local Dependencies
#include "jnipp.h"

namespace jni
{
	// Static Variables
	static JavaVM* javaVm  = nullptr;
	static void*   library = nullptr;

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
			if (vm->AttachCurrentThread((void**) &_env, nullptr) != 0)
				throw InitializationException("Could not attach JNI to thread");

			_attached = true;
		}
	}

	/*
		Helper Functions
	 */

	static JNIEnv* env()
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

		if (env->ExceptionCheck() != JNI_FALSE)
		{
			env->ExceptionClear();
			throw InvocationException();
		}
	}

	/*
		Stand-alone Function Impelementations
	 */

	void init(JNIEnv* env)
	{
		if (javaVm != nullptr)
			return;

		if (env->GetJavaVM(&javaVm) != 0)
			throw InitializationException("Could not acquire Java VM");
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
		return _handle == NULL;
	}

	template <>	bool Object::callMethod(method_t method, internal::value_t* args)
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

	template <>	wchar_t Object::callMethod(method_t method, internal::value_t* args)
	{
		auto result = env()->CallCharMethodA(_handle, method, (jvalue*) args);
		handleJavaExceptions();
		return result;
	}

	template <> wchar_t Object::get(field_t field) const
	{
		return env()->GetCharField(_handle, field);
	}

	template <> void Object::set(field_t field, const wchar_t& value)
	{
		env()->SetCharField(_handle, field, value);
	}

	template <>	short Object::callMethod(method_t method, internal::value_t* args)
	{
		auto result = env()->CallShortMethodA(_handle, method, (jvalue*) args);
		handleJavaExceptions();
		return result;
	}

	template <> short Object::get(field_t field) const
	{
		return env()->GetShortField(_handle, field);
	}

	template <> void Object::set(field_t field, const short& value)
	{
		env()->SetShortField(_handle, field, value);
	}

	template <>	int Object::callMethod(method_t method, internal::value_t* args)
	{
		auto result = env()->CallIntMethodA(_handle, method, (jvalue*) args);
		handleJavaExceptions();
		return result;
	}

	template <> int Object::get(field_t field) const
	{
		return env()->GetIntField(_handle, field);
	}

	template <> void Object::set(field_t field, const int& value)
	{
		env()->SetIntField(_handle, field, value);
	}

	template <>	long long Object::callMethod(method_t method, internal::value_t* args)
	{
		auto result = env()->CallLongMethodA(_handle, method, (jvalue*) args);
		handleJavaExceptions();
		return result;
	}

	template <> long long Object::get(field_t field) const
	{
		return env()->GetLongField(_handle, field);
	}

	template <> void Object::set(field_t field, const long long& value)
	{
		env()->SetLongField(_handle, field, value);
	}

	template <>	float Object::callMethod(method_t method, internal::value_t* args)
	{
		auto result = env()->CallFloatMethodA(_handle, method, (jvalue*) args);
		handleJavaExceptions();
		return result;
	}

	template <> float Object::get(field_t field) const
	{
		return env()->GetFloatField(_handle, field);
	}

	template <> void Object::set(field_t field, const float& value)
	{
		env()->SetFloatField(_handle, field, value);
	}

	template <>	double Object::callMethod(method_t method, internal::value_t* args)
	{
		auto result = env()->CallDoubleMethodA(_handle, method, (jvalue*) args);
		handleJavaExceptions();
		return result;
	}

	template <> double Object::get(field_t field) const
	{
		return env()->GetDoubleField(_handle, field);
	}

	template <> void Object::set(field_t field, const double& value)
	{
		env()->SetDoubleField(_handle, field, value);
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

	method_t Class::getMethod(const char* name, const char* signature) const
	{
		jmethodID id = env()->GetMethodID(jclass(getHandle()), name, signature);

		if (id == nullptr)
			throw NameResolutionException(name);

		return id;
	}

	Object Class::newObject(method_t constructor, internal::value_t* args) const
	{
		jobject ref = env()->NewObjectA(jclass(getHandle()), constructor, (jvalue*)args);
		handleJavaExceptions();
		return Object(ref, DeleteLocalInput);
	}

	/*
		Class Implementation
	 */

	typedef jint (JNICALL *CreateVm_t)(JavaVM**, void**, void*);

	Vm::Vm(const char* path)
	{
		if (javaVm != nullptr)
			throw InitializationException("Java Virtual Machine already initialized");
		if (path == nullptr)
			throw InitializationException("Automatic path selection not yet implemented");

		JNIEnv* env;
		JavaVMInitArgs args = {};

		args.version = JNI_VERSION_1_2;

#ifdef _WIN32
		
		HMODULE lib = ::LoadLibraryA(path);

		if (lib == NULL)
			throw InitializationException("Could not load JVM library");

		CreateVm_t JNI_CreateJavaVM = (CreateVm_t) ::GetProcAddress(lib, "JNI_CreateJavaVM");

		if (JNI_CreateJavaVM == NULL)
		{
			::FreeLibrary(lib);
			throw InitializationException("Invalid JVM library");
		}

		if (JNI_CreateJavaVM(&javaVm, (void**) &env, &args) != 0)
		{
			::FreeLibrary(lib);
			throw InitializationException("Java Virtual Machine failed during creation");
		}

		// Save the loaded DLL for later.
		library = lib;

#else
# error Platform not yet supported
#endif // _WIN32
	}

	Vm::~Vm()
	{
		javaVm->DestroyJavaVM();
		javaVm = nullptr;

#ifdef _WIN32
		::FreeLibrary(HMODULE(library));
#else
# error "Platform not supported"
#endif // _WIN32

		library = nullptr;
	}
}

