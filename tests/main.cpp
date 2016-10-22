// Local Dependencies
#include "../jnipp.h"

int main()
{
	jni::Vm vm("C:\\Program Files\\Java\\jre1.8.0_25\\bin\\server\\jvm.dll");

	jni::Class String = jni::Class("java/lang/String");
	jni::Object str = String.newInstance();

	return 0;
}

