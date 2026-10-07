
cmake_minimum_required(VERSION 3.16.3)

set (CMAKE_CXX_STANDARD 11) 
set (CMAKE_CXX_STANDARD_REQUIRED True)

project (NetworkLib) 

#设置全局链接库，因为#include <pthread.h> 不典型的库，需要告诉编译器去pthread中寻找。
set(LIBS
    pthread
)

#添加子目录，将中型项目的cmakelist分开文件夹写再去统筹是合理的
add_subdirectory(src)



