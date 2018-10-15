CMAKE_MINIMUM_REQUIRED(VERSION 3.4.1)

PROJECT(native_app)

FILE(GLOB sources ../*.cpp ../common/*.cpp ../android/*.cpp)

ADD_LIBRARY(native_app ${sources})
