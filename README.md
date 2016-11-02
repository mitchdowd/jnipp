Java Native Interface for C++
=============================

## Overview

JNIPP is just a C++ wrapper for the standard Java Native Interface (JNI). It
tries to take some of the long-winded annoyance out of integrating your Java
and C++ code.

## Requirements

To compile you will need:
 - A C++11 compatible compiler
 - An installation of the Java Development Kit (JDK)
 - The `JAVA_HOME` environment variable, directed to your JDK installation.

## Usage

> For comprehensive examples on how to use *jnipp*, see the `tests` project
> in the project source code.

There are two situations where the Java Native Interface would be needed.
 - A Java application calling C/C++ functions; or
 - A C/C++ application calling Java methods

### Calling Java from C++

The following is an example of calling Java from C++.

```C++
#include <jnipp.h>

int main()
{
	// An instance of the Java VM needs to be created.
	jni::Vm vm;
	
	// Create an instance of java.lang.Integer
	jni::Class Integer = jni::Class("java/lang/Integer");
	jni::Object i = Integer.newInstance("1000");
	
	// Call the `toString` method on that integer
	std::string str = i.call<std::string>("toString");
	
	// The Java VM is automatically destroyed when it goes out of scope.
	return 0;
}
```

### Calling C++ from Java

Consider a basic Java program:

```Java
package com.example;

class Demo {
	public int value;

	public static void main(String[] args) {
		Demo demo = new Demo();
		demo.value = 1000;
		demo.run();
	}
	
	public native void run();
}
```
A matching C++ library which uses *jnipp* could look like:

```C++
#include <jnipp.h>
#include <iostream>

/*
	The signature here is defind by the JNI standard, so must be adhered to.
	Although, to prevent pollution of the global namespace, the JNIEnv and
	jobject types defind by the standard JNI have been placed into the
	jni namespace.
 */
extern "C" void Java_com_example_Demo_run(jni::JNIEnv* env, jni::jobject obj)
{
	// jnipp only needs initialising once, but it doesn't hurt to do it again.
	jni::init(env);
	
	// Capture the supplied object.
	jni::Object demo(obj);
	
	// Print the contents of the `value` field to stdout.
	std::cout << demo.get<int>("value") << std::endl;
}
```

## Configuration

By default, *jnipp* uses std::runtime_error as the base exception class. If you wish,
you can update the type definition in `jnipp.h` to refer to your own exception class
if you so choose. It just needs a `const char*` constructor.
