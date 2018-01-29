#ifndef _JNIPP_H_
#define _JNIPP_H_ 1

// Standard Dependencies
#include <cstring>
#include <stdexcept>		// For std::runtime_error
#include <string>

// Forward Declarations
struct JNIEnv_;
struct _JNIEnv;
struct _jmethodID;
struct _jfieldID;
class  _jobject;
class  _jclass;

namespace jni
{
	// JNI Base Types
#ifdef __ANDROID__
	typedef _JNIEnv		JNIEnv;
#else
	typedef JNIEnv_		JNIEnv;
#endif
	typedef _jobject*	jobject;
	typedef _jclass*	jclass;

	/**
		You can save a method via its handle using Class::getMethod() if it is
		going to be used often. This saves Object::call() from having to locate
		the method each time by name.

		Note that these handles are global and do not need to be deleted.
	 */
	typedef _jmethodID* method_t;

	/**
		You can save a field via its handle using Class::getField() if it is
		going to be used often. This saves Object::set() and Object::get() from
		having to locate the field each time by name.

		Note that these handles are global and do not need to be deleted.
	 */
	typedef _jfieldID* field_t;

	/**
		Base class for thrown Exceptions. Change it to whatever your chosen
		Exception class, as long as that class has a `const char*` constructor.
	*/
	typedef std::runtime_error Exception;

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
		inline std::string valueSig(const Object* const* obj)	{ return valueSig(obj ? *obj : nullptr); }

		template <int n, class TArg>
		inline std::string valueSig(const TArg (*arg)[n]) { return valueSig((const TArg* const*) arg); }

		inline std::string sig() { return ""; }

		template <class TArg, class... TArgs>
		std::string sig(const TArg& arg, const TArgs&... args) {
			return valueSig(&arg) + sig(args...);
		}

		/*
			Argument Conversion
		 */

		typedef long long value_t;

		void valueArg(value_t* v, bool a);
		void valueArg(value_t* v, wchar_t a);
		void valueArg(value_t* v, short a);
		void valueArg(value_t* v, int a);
		void valueArg(value_t* v, long long a);
		void valueArg(value_t* v, float a);
		void valueArg(value_t* v, double a);
		void valueArg(value_t* v, jobject a);
		void valueArg(value_t* v, const Object& a);
		void valueArg(value_t* v, const Object* const& a);
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

		template <class TArg>	void cleanupArg(value_t* value) {}
		template <>				void cleanupArg<std::string>(value_t* value);
		template <>				void cleanupArg<std::wstring>(value_t* value);
		template <>				void cleanupArg<const char*>(value_t* value);
		template <>				void cleanupArg<const wchar_t*>(value_t* value);

		template <class TArg = void, class... TArgs>
		void cleanupArgs(value_t* values) {
			cleanupArg<TArg>(values);
			cleanupArgs<TArgs...>(values + 1);
		}

		template <>
		inline void cleanupArgs<void>(value_t* values) {}

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

	/**
		Initialises the Java Native Interface with the given JNIEnv handle, which
		gets passed into a native function which is called from Java. This only
		needs to be done once per process - further calls are no-ops.
		\param env A JNI environment handle.
	 */
	void init(JNIEnv* env);

	/**
		Object corresponds with a `java.lang.Object` instance. With an Object,
		you can then call Java methods, and access fields on the Object. To
		instantiate an Object of a given class, use the `Class` class.
	 */
	class Object
	{
	public:
		/** Flags which can be passed to the Object constructor. */
		enum ScopeFlags
		{
			Temporary			= 1,	///< Temporary object. Do not create a global reference.
			DeleteLocalInput	= 2		///< The input reference is temporary and can be deleted.
		};

		/** Default constructor. Creates a `null` object. */
		Object() noexcept;

		/**
			Copies a reference to another Object. Note that this is not a deep
			copy operation, and both Objects will reference the same Java
			Object.
			\param other The Object to copy.
		 */
		Object(const Object& other);

		/**
			Move constructor. Copies the Object reference from the supplied
			Object, and then nulls the supplied Object reference.
			\param other The Object to move.
		 */
		Object(Object&& other) noexcept;

		/**
			Creates an Object from a local JNI reference.
			\param ref The local JNI reference.
			\param scopeFlags Bitmask of ScopeFlags values.
		 */
		Object(jobject ref, int scopeFlags = 0);

		/** 
			Destructor. Releases this reference on the Java Object so it can be
			picked up by the garbage collector.
		 */
		virtual ~Object() noexcept;

		/**
			Assignment operator. Copies the object reference from the supplied
			Object. They will now both point to the same Java Object.
			\param other The Object to copy.
			\return This Object.
		 */
		Object& operator=(const Object& other);

		/**
			Assignment operator. Moves the object reference from the supplied
			Object to this one, and leaves the other one as a null.
			\param other The Object to move.
			\return This Object.
		 */
		Object& operator=(Object&& other);

		/**
			Tells whether the two Objects refer to the same Java Object.
			\param other the Object to compare with.
			\return `true` if the same, `false` otherwise.
		 */
		bool operator==(const Object& other) const;

		/**
			Tells whether the two Objects refer to the same Java Object.
			\param other the Object to compare with.
			\return `true` if the different, `false` otherwise.
		 */
		bool operator!=(const Object& other) const { return !operator==(other); }

		/**
			Calls the given method on this Object. The method should have no
			parameters. Note that the return type should be explicitly stated
			in the function call.
			\param method A method handle which applies to this Object.
			\return The method's return value.
		 */
		template <class TReturn>
		TReturn call(method_t method) const { return callMethod<TReturn>(method, nullptr); }

		/**
			Calls the method on this Object with the given name, and no arguments.
			Note that the return type should be explicitly stated in the function
			call.
			\param name The name of the method to call (with optional signature).
			\return The method's return value.
		 */
		template <class TReturn>
		TReturn call(const char* name) const {
			if (std::strstr(name, "()"))
				return call<TReturn>(getMethod(name));

			// No signature supplied. Generate our own.
			method_t method = getMethod(name, ("()" + internal::valueSig((TReturn*) nullptr)).c_str());
			return call<TReturn>(method);
		}

		/**
			Calls the method on this Object and supplies the given arguments.
			Note that the return type should be explicitly stated in the function
			call.
			\param method The method to call.
			\param args Arguments to supply to the method.
			\return The method's return value.
		 */
		template <class TReturn, class... TArgs>
		TReturn call(method_t method, const TArgs&... args) const {
			internal::ArgArray<TArgs...> transform(args...);
			return callMethod<TReturn>(method, transform.values);
		}

		/**
			Calls the method on this Object and supplies the given arguments.
			Note that the return type should be explicitly stated in the function
			call. The type signature of the method is calculated by the types of
			the supplied arguments.
			\param name The name of the method to call (and optional signature).
			\param args Arguments to supply to the method.
			\return The method's return value.
		 */
		template <class TReturn, class... TArgs>
		TReturn call(const char* name, const TArgs&... args) const {
			if (std::strchr(name, '('))
				return call<TReturn>(getMethod(name), args...);

			std::string sig = "(" + internal::sig(args...) + ")" + internal::valueSig((TReturn*) nullptr);
			method_t method = getMethod(name, sig.c_str());
			return call<TReturn>(method, args...);
		}

		/**
			Gets a field value from this Object. The field must belong to the
			Object's class. Note that the field type should be explicitly stated
			in the function call.
			\param field Identifier for the field to retrieve.
			\return The field's value.
		 */
		template <class TType>
		TType get(field_t field) const;

		/**
			Gets a field value from this Object. The field must belong to the
			Object's class. Note that the field type should be explicitly stated
			in the function call.
			\param name The name of the field to retrieve.
			\return The field's value.
		 */
		template <class TType>
		TType get(const char* name) const {
			field_t field = getField(name, internal::valueSig((TType*) nullptr).c_str());
			return get<TType>(field);
		}

		/**
			Sets a field's value on this Object. The field must belong to the
			Object's class, and the parameter's type should correspond to the
			type of the field.
			\param field The field to set the value to.
			\param value The value to set.
		 */
		template <class TType>
		void set(field_t field, const TType& value);

		/**
			Sets a field's value on this Object. The field must belong to the
			Object's class, and the parameter's type should correspond to the
			type of the field.
			\param name The name of the field to set the value to.
			\param value The value to set.
		 */
		template <class TType>
		void set(const char* name, const TType& value) {
			field_t field = getField(name, internal::valueSig((TType*) nullptr).c_str());
			set(field, value);
		}

		/**
			Tells whether this Object is currently a `null` pointer.
			\return `true` if `null`, `false` if it references an object.
		 */
		bool isNull() const noexcept;

		/**
			Gets a handle for this Object's class.
			\return The Object's Class's handle.
		 */
		jclass getClass() const;

		/**
			Gets the underlying JNI jobject handle.
			\return The JNI handle.
		 */
		jobject getHandle() const noexcept { return _handle; }

	private:
		// Helper Functions
		method_t getMethod(const char* name, const char* signature) const;
		method_t getMethod(const char* nameAndSignature) const;
		field_t getField(const char* name, const char* signature) const;
		template <class TType> TType callMethod(method_t method, internal::value_t* values) const;

		// Instance Variables
		jobject _handle;
		mutable jclass _class;
		bool _isGlobal;
	};

	/**
		Class corresponds with `java.lang.Class`, and allows you to instantiate
		Objects and get class members such as methods and fields.
	 */
	class Class : protected Object
	{
	public:
		/**
			Obtains a class reference to the Java class with the given qualified
			name.
			\param name The qualified class name (e.g. "java/lang/String").
		 */
		Class(const char* name);

		/**
			Creates a Class object by JNI reference.
			\param ref The JNI class reference.
			\param scopeFlags Bitmask of Object::ScopeFlags.
		 */
		Class(jclass ref, int scopeFlags = 0);

		/**
			Tells whether this Class is null or valid.
			\return `true` if null, `false` if valid.
		 */
		bool isNull() const noexcept { return Object::isNull(); }

		/**
			Creates a new instance of this Java class and returns a reference to
			it. The item's parameterless constructor is called.
			\return The created instance.
		 */
		Object newInstance() const;

		/**
			Creates a new instance of this Java class and returns a reference to
			it.
			\param constructor The constructor to call.
			\param args Arguments to supply to the constructor.
			\return The created instance.
		 */
		template <class... TArgs>
		Object newInstance(method_t constructor, const TArgs&... args) const {
			internal::ArgArray<TArgs...> transform(args...);
			return newObject(constructor, transform.values);
		}

		/**
			Creates a new instance of this Java class and returns a reference to
			it. The constructor signature is determined by the supplied parameters,
			and the parameters are then passed to the constructor.
			\param args Arguments to supply to the constructor.
			\return The created instance.
		 */
		template <class... TArgs>
		Object newInstance(const TArgs&... args) const {
			method_t constructor = getMethod("<init>", ("(" + internal::sig(args...) + ")V").c_str());
			return newInstance(constructor, args...);
		}

		/**
			Gets a handle to the field with the given name and type signature.
			This handle can then be stored so that the field does not need to
			be looked up by name again. It does not need to be deleted.
			\param name The name of the field.
			\param signature The JNI type signature of the field.
			\return The field ID.
		 */
		field_t getField(const char* name, const char* signature) const;

		/**
			Gets a handle to the static field with the given name and type signature.
			This handle can then be stored so that the field does not need to
			be looked up by name again. It does not need to be deleted.
			\param name The name of the field.
			\param signature The JNI type signature of the field.
			\return The field ID.
		 */
		field_t getStaticField(const char* name, const char* signature) const;

		/**
			Gets a handle to the method with the given name and signature.
			This handle can then be stored so that the method does not need
			to be looked up by name again. It does not need to be deleted.
			\param name The name of the method.
			\param signature The JNI method signature.
			\return The method ID.
		 */
		method_t getMethod(const char* name, const char* signature) const;

		/**
			Gets a handle to the method with the given name and signature.
			This handle can then be stored so that the method does not need
			to be looked up by name again. It does not need to be deleted.
			\param nameAndSignature Name and signature identifier (e.g. "toString()Ljava/lang/String;").
			\return The method ID.
		 */
		method_t getMethod(const char* nameAndSignature) const;

		/**
			Gets a handle to the static method with the given name and signature.
			This handle can then be stored so that the method does not need
			to be looked up by name again. It does not need to be deleted.
			\param name The name of the method.
			\param signature The JNI method signature.
			\return The method ID.
		 */
		method_t getStaticMethod(const char* name, const char* signature) const;

		/**
			Gets a handle to the static method with the given name and signature.
			This handle can then be stored so that the method does not need
			to be looked up by name again. It does not need to be deleted.
			\param nameAndSignature Name and signature identifier (e.g. "toString()Ljava/lang/String;").
			\return The method ID.
		 */
		method_t getStaticMethod(const char* nameAndSignature) const;

		/**
			Gets a handle to the constructor for this Class with the given
			signature. Note that the return type should always be `void` ("V").
			\param signature The JNI method signature for the constructor.
			\return The constructor method ID.
		 */
		method_t getConstructor(const char* signature) const { return getMethod("<init>", signature); }

		/**
			Gets the parent Class of this Class.
			\return The parent class.
		 */
		Class getParent() const;

		/**
			Gets the JNI-qualified name of this Class.
			\return The Class name.
		 */
		std::string getName() const;

		/**
			Calls a static method on this Class. The method should have no
			parameters. Note that the return type should be explicitly stated
			in the function call.
			\param method A method handle which applies to this Object.
			\return The method's return value.
		 */
		template <class TReturn>
		TReturn call(method_t method) const { return callStaticMethod<TReturn>(method, nullptr); }

		/**
			Calls a static method on this Class with the given name, and no arguments.
			Note that the return type should be explicitly stated in the function
			call.
			\param name The name of the method to call.
			\return The method's return value.
		 */
		template <class TReturn>
		TReturn call(const char* name) const {
			method_t method = getMethod(name, ("()" + internal::valueSig((TReturn*) nullptr)).c_str());
			return call<TReturn>(method);
		}

		/**
			Calls a static method on this Class and supplies the given arguments.
			Note that the return type should be explicitly stated in the function
			call.
			\param method The method to call.
			\param args Arguments to supply to the method.
			\return The method's return value.
		 */
		template <class TReturn, class... TArgs>
		TReturn call(method_t method, const TArgs&... args) const {
			internal::ArgArray<TArgs...> transform(args...);
			return callStaticMethod<TReturn>(method, transform.values);
		}

		/**
			Calls a static method on this Class and supplies the given arguments.
			Note that the return type should be explicitly stated in the function
			call. The type signature of the method is calculated by the types of
			the supplied arguments.
			\param name The name of the method to call.
			\param args Arguments to supply to the method.
			\return The method's return value.
		 */
		template <class TReturn, class... TArgs>
		TReturn call(const char* name, const TArgs&... args) const {
            if (std::strchr(name, '('))
                return call<TReturn>(getStaticMethod(name), args...);

			std::string sig = "(" + internal::sig(args...) + ")" + internal::valueSig((TReturn*) nullptr);
			method_t method = getStaticMethod(name, sig.c_str());
			return call<TReturn>(method, args...);
		}

		/**
			Calls a non-static method on this Class, applying it to the supplied
			Object. The difference between this and Object.call() is that the
			specific class implementation of the method is called, rather than
			doing a virtual method lookup.
			\param obj The Object to call the method on.
			\param method The method to call.
			\return The method's return value.
		 */
		template <class TReturn>
		TReturn call(const Object& obj, method_t method) const {
			return callExactMethod<TReturn>(obj.getHandle(), method, nullptr);
		}

		/**
			Calls a non-static method on this Class, applying it to the supplied
			Object. The difference between this and Object.call() is that the
			specific class implementation of the method is called, rather than
			doing a virtual method lookup.
			\param obj The Object to call the method on.
			\param name The name of the method to call.
			\return The method's return value.
		 */
		template <class TReturn>
		TReturn call(const Object& obj, const char* name) const {
			method_t method = getMethod(name, ("()" + internal::valueSig((TReturn*) nullptr)).c_str());
			return call<TReturn>(obj, method);
		}
		template <class TReturn>
		TReturn call(const Object* obj, const char* name) const {
			return call<TReturn>(obj, name);
		}

		/**
			Calls a non-static method on this Class, applying it to the supplied
			Object. The difference between this and Object.call() is that the
			specific class implementation of the method is called, rather than
			doing a virtual method lookup.
			\param obj The Object to call the method on.
			\param method The method to call.
			\param args Arguments to pass to the method.
			\return The method's return value.
		 */
		template <class TReturn, class... TArgs>
		TReturn call(const Object& obj, method_t method, const TArgs&... args) const {
			internal::ArgArray<TArgs...> transform(args...);
			return callExactMethod<TReturn>(obj.getHandle(), method, transform.values);
		}
		template <class TReturn, class... TArgs>
		TReturn call(const Object* obj, method_t method, const TArgs&... args) const {
			return call<TReturn>(*obj, method, args...);
		}

		/**
			Calls a non-static method on this Class, applying it to the supplied
			Object. The difference between this and Object.call() is that the
			specific class implementation of the method is called, rather than
			doing a virtual method lookup.
			\param obj The Object to call the method on.
			\param name The name of the method to call.
			\param args Arguments to pass to the method.
			\return The method's return value.
		 */
		template <class TReturn, class... TArgs>
		TReturn call(const Object& obj, const char* name, const TArgs&... args) const {
			std::string sig = "(" + internal::sig(args...) + ")" + internal::valueSig((TReturn*) nullptr);
			method_t method = getMethod(name, sig.c_str());
			return call<TReturn>(obj, method, args...);
		}
		template <class TReturn, class... TArgs>
		TReturn call(const Object* obj, const char* name, const TArgs&... args) const {
			return call<TReturn>(*obj, name, args...);
		}

		/**
			Gets a static field value from this Class. Note that the field type
			should be explicitly stated in the function call.
			\param field Identifier for the field to retrieve.
			\return The field's value.
		 */
		template <class TType>
		TType get(field_t field) const;

		/**
			Gets a static field value from this Class. Note that the field type
			should be explicitly stated in the function call.
			\param name The name of the field to retrieve.
			\return The field's value.
		 */
		template <class TType>
		TType get(const char* name) const {
			field_t field = getStaticField(name, internal::valueSig((TType*) nullptr).c_str());
			return get<TType>(field);
		}

		/**
			Sets a static field's value on this Class. The parameter's type should
			correspond to the type of the field.
			\param field The field to set the value to.
			\param value The value to set.
		 */
		template <class TType>
		void set(field_t field, const TType& value);

		/**
			Sets a static field's value on this Class. The parameter's type
			should correspond to the type of the field.
			\param name The name of the field to set the value to.
			\param value The value to set.
		 */
		template <class TType>
		void set(const char* name, const TType& value) {
			field_t field = getStaticField(name, internal::valueSig((TType*) nullptr).c_str());
			set(field, value);
		}

	private:
		// Helper Functions
		template <class TType> TType callStaticMethod(method_t method, internal::value_t* values) const;
		template <class TType> TType callExactMethod(jobject obj, method_t method, internal::value_t* values) const;
		Object newObject(method_t constructor, internal::value_t* args) const;
	};

	/**
		Convenience class for dealing with Java enums.
	 */
	class Enum : protected Class
	{
	public:
		/**
			Loads the Enum with the given JNI-formatted name.
			\param name The name of the enum.
		 */
		Enum(const char* name);

		/**
			Gets the enum value with the given name.
			\param name The name of the enum value.
			\return The enum value identifier.
		 */
		Object get(const char* name) const;

	private:
		// Instance Variables
		std::string _name;
	};

	/**
		When the application's entry point is in C++ rather than in Java, it will
		need to spin up its own instance of the Java Virtual Machine (JVM) before
		it can initialize the Java Native Interface. Vm is used to create and 
		destroy a running JVM instance.

		It uses the RAII idiom, so when the destructor is called, the Vm is shut
		down.

		Note that currently only one instance is supported. Attempts to create
		more will result in an InitializationException.
	 */
	class Vm final
	{
	public:
		/**
			Starts the Java Virtual Machine. 
			\param path The path to the jvm.dll (or null for auto-detect).
		 */
		Vm(const char* path = nullptr);

		/** Destroys the running instance of the JVM. */
		~Vm();
	};

	/**
		A Java method call threw an Exception.
	 */
	class InvocationException : public Exception
	{
	public:
		/**
			Constructor with an error message.
			\param msg Message to pass to the Exception.
		 */
		InvocationException(const char* msg = "Java Exception detected") : Exception(msg) {}
	};

	/**
		A supplied name or type signature could not be resolved.
	 */
	class NameResolutionException : public Exception
	{
	public:
		/**
			Constructor with an error message.
			\param name The name of the unresolved symbol.
		 */
		NameResolutionException(const char* name) : Exception(name) {}
	};

	/**
		The Java Native Interface was not properly initialized.
	 */
	class InitializationException : public Exception
	{
	public:
		/**
			Constructor with an error message.
			\param msg Message to pass to the Exception.
		 */
		InitializationException(const char* msg) : Exception(msg) {}
	};
}

#endif // _JNIPP_H_

