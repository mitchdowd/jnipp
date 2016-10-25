// Project Dependencies
#include <jnipp.h>

// Local Dependencies
#include "testing.h"

TEST(VM_throwsExceptionOnBadPath)
{
	try
	{
		jni::Vm vm("bad path");
	}
	catch (jni::InitializationException&)
	{
		ASSERT(1);
		return;
	}

	ASSERT(0);
}

int main()
{
	RUN_TEST(VM_throwsExceptionOnBadPath);

	std::cin.get();
	return 0;
}

