#ifndef _TESTING_H_
#define _TESTING_H_ 1

// Standard Dependencies
#include <iostream>
#include <iomanip>

/** Run the test with the given name. */
#define RUN_TEST(TestName) {													\
	bool result = true;															\
	std::cout << "Executing test " << std::left << std::setw(40) << #TestName;	\
	TestName(result);															\
	std::cout << "=> " << (result ? "Success" : "Fail") << std::endl;			\
}

/** Define a test with the given name. */
#define TEST(TestName)	\
	void TestName(bool& result)

/** Ensures the given condition passes for the test to pass. */
#define ASSERT(condition) {		\
	if (!(condition)) {			\
		result = false;			\
		return;					\
	}							\
}

#endif // _TESTING_H_

