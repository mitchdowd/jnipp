// Project Dependencies
#include <jnipp.h>

// Local Dependencies
#include "testing.h"

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

int main()
{
	jni::Vm vm;

	RUN_TEST(VM_notAllowedMultipleVms);
	RUN_TEST(Class_findByName);
	RUN_TEST(Class_newInstance);
	RUN_TEST(Class_newInstance_withArgs);
	RUN_TEST(Class_getStaticField);
	RUN_TEST(Class_getMethod);
	RUN_TEST(Class_getStaticMethod);
	RUN_TEST(Class_get_staticField);
	RUN_TEST(Class_get_staticField_byName);
	RUN_TEST(Class_call_staticMethod_byName);
	RUN_TEST(Object_defaultConstructor_isNull);

	std::cout << "Press a key to continue..." << std::endl;
	std::cin.get();
	return 0;
}

