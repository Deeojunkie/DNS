cmake_minimum_required(VERSION 3.14)
project(clion_version C)

set(CMAKE_C_STANDARD 99)

add_executable(clion_version main.c DNS.c hash.c idmap.c ipcache.c set.c)