// External Dependencies
#include <jni.h>

// Local Dependencies
#include "jnipp.h"

namespace jni
{
	// Static Variables
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

		if (vm->GetEnv((void**) &_env, JNI_VERSION_1_4) != JNI_OK)
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

	Object::Object(jobject ref, int scopeFlags) : _handle(ref), _class(nullptr), _isGlobal((scopeFlags & Temporary) == 0)
	{
		if (!_isGlobal)
			return;

		JNIEnv* env = jni::env();

		_handle = env->NewGlobalRef(ref);

		if (scopeFlags & DeleteLocalInput)
			env->DeleteLocalRef(ref);
	}

	Object::~Object()
	{
		JNIEnv* env = jni::env();

		if (_isGlobal)
			env->DeleteGlobalRef(_handle);

		if (_class != nullptr)
			env->DeleteGlobalRef(_class);
	}

	/*
		Class Implementation
	 */

	Class::Class(jclass ref, int scopeFlags) : Object(ref, scopeFlags)
	{
	}
}

