// Project Dependencies
#include <jnipp.h>

// Local Dependencies
#include "testing.h"

TEST(Vm_detectsJreInstall)
{
	try
	{
		jni::Vm vm;
	}
	catch (jni::InitializationException&)
	{
		ASSERT(0);
		return;
	}

	ASSERT(1);
}

TEST(VM_notAllowedMultipleVms)
{
	try
	{
		jni::Vm secondVm;	// Throws an exception.
	}
	catch (jni::InitializationException&)
	{
		ASSERT(1);
		return;
	}

	ASSERT(0);
}

TEST(Class_findByName)
{
	jni::Class cls("java/lang/String");

	ASSERT(!cls.isNull());
}

TEST(Class_newInstance)
{
	jni::Object str = jni::Class("java/lang/String").newInstance();

	ASSERT(!str.isNull());
}

TEST(Class_newInstance_withArgs)
{
	jni::Object str1 = jni::Class("java/lang/String").newInstance("Testing...");
	jni::Object str2 = jni::Class("java/lang/String").newInstance(L"Testing...");

	ASSERT(!str1.isNull());
	ASSERT(!str2.isNull());
}

TEST(Class_getStaticField)
{
	jni::field_t field = jni::Class("java/lang/Integer").getStaticField("MAX_VALUE", "I");

	ASSERT(field);
}

TEST(Class_getMethod)
{
	jni::method_t method = jni::Class("java/lang/Integer").getMethod("intValue", "()I");

	ASSERT(method);
}

TEST(Class_getStaticMethod)
{
	jni::method_t method = jni::Class("java/lang/Integer").getStaticMethod("compare", "(II)I");

	ASSERT(method);
}

TEST(Class_get_staticField)
{
	jni::Class Integer("java/lang/Integer");
	jni::field_t field = Integer.getStaticField("SIZE", "I");

	ASSERT(Integer.get<int>(field) == 32);
}

TEST(Class_get_staticField_byName)
{
	jni::Class Integer("java/lang/Integer");

	ASSERT(Integer.get<int>("SIZE") == 32);
}

TEST(Class_call_staticMethod)
{
	jni::Class Integer("java/lang/Integer");
	jni::method_t method = Integer.getStaticMethod("parseInt", "(Ljava/lang/String;)I");

	int i = Integer.call<int>(method, "1000");

	ASSERT(i == 1000);
}

TEST(Class_call_staticMethod_byName)
{
	int i = jni::Class("java/lang/Integer").call<int>("parseInt", "1000");

	ASSERT(i = 1000);
}

TEST(Object_defaultConstructor_isNull)
{
	jni::Object o;

	ASSERT(o.isNull());
}

TEST(Object_copyConstructorIsSameObject)
{
	jni::Object a = jni::Class("java/lang/String").newInstance();
	jni::Object b = a;

	ASSERT(a == b);
}

TEST(Object_moveConstructor)
{
	jni::Object a = jni::Class("java/lang/String").newInstance();
	jni::Object b = std::move(a);

	ASSERT(a.isNull());
	ASSERT(!b.isNull());
}

TEST(Object_call)
{
	jni::Class Integer("java/lang/Integer");
	jni::method_t intValue = Integer.getMethod("intValue", "()I");
	jni::Object i = Integer.newInstance(100);

	ASSERT(i.call<int>(intValue) == 100);
}

TEST(Object_call_byName)
{
	jni::Object i = jni::Class("java/lang/Integer").newInstance(100);

	ASSERT(i.call<int>("intValue") == 100);
}

TEST(Object_call_withArgs)
{
	jni::Class String("java/lang/String");
	jni::method_t charAt = String.getMethod("charAt", "(I)C");
	jni::Object str = String.newInstance("Testing");

	ASSERT(str.call<wchar_t>(charAt, 1) == L'e');
}

TEST(Object_call_byNameWithArgs)
{
	jni::Object str = jni::Class("java/lang/String").newInstance("Testing");

	ASSERT(str.call<wchar_t>("charAt", 1) == L'e');
}

TEST(Arg_bool)
{
	jni::String str = jni::Class("java/lang/String").call<jni::String>("valueOf", true);

	ASSERT(str == "true");
}

TEST(Arg_wchar)
{
	jni::String str = jni::Class("java/lang/String").call<jni::String>("valueOf", L'X');

	ASSERT(str == "X");
}

TEST(Arg_double)
{
	jni::String str = jni::Class("java/lang/String").call<jni::String>("valueOf", 123.0);

	ASSERT(str == "123.0");
}

TEST(Arg_float)
{
	jni::String str = jni::Class("java/lang/String").call<jni::String>("valueOf", 123.0f);

	ASSERT(str == "123.0");
}

TEST(Arg_int)
{
	jni::String str = jni::Class("java/lang/String").call<jni::String>("valueOf", 123);

	ASSERT(str == "123");
}

TEST(Arg_longLong)
{
	jni::String str = jni::Class("java/lang/String").call<jni::String>("valueOf", 123LL);

	ASSERT(str == "123");
}

int main()
{
	// jni::Vm Tests
	RUN_TEST(Vm_detectsJreInstall);
	RUN_TEST(VM_notAllowedMultipleVms);

	{
		jni::Vm vm;

		// jni::Class Tests
		RUN_TEST(Class_findByName);
		RUN_TEST(Class_newInstance);
		RUN_TEST(Class_newInstance_withArgs);
		RUN_TEST(Class_getStaticField);
		RUN_TEST(Class_getMethod);
		RUN_TEST(Class_getStaticMethod);
		RUN_TEST(Class_get_staticField);
		RUN_TEST(Class_get_staticField_byName);
		RUN_TEST(Class_call_staticMethod_byName);

		// jni::Object Tests
		RUN_TEST(Object_defaultConstructor_isNull);
		RUN_TEST(Object_copyConstructorIsSameObject);
		RUN_TEST(Object_moveConstructor);
		RUN_TEST(Object_call);
		RUN_TEST(Object_call_byName);
		RUN_TEST(Object_call_withArgs);
		RUN_TEST(Object_call_byNameWithArgs);

		// Argument Type Tests
		RUN_TEST(Arg_bool);
		RUN_TEST(Arg_wchar);
		RUN_TEST(Arg_double);
		RUN_TEST(Arg_float);
		RUN_TEST(Arg_int);
		RUN_TEST(Arg_longLong);
	}

	std::cout << "Press a key to continue..." << std::endl;
	std::cin.get();
	return 0;
}

