// Local Dependencies
#include "../jnipp.h"

int main()
{
	jni::Vm vm("C:\\Program Files\\Java\\jre1.8.0_31\\bin\\server\\jvm.dll");

	jni::Class Integer = jni::Class("java/lang/Integer");

	int i = Integer.get<int>("SIZE");

	jni::Object obj = Integer.newInstance("100");

	std::string str = obj.call<std::string>("toString");

	return 0;
}

