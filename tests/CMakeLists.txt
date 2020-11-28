cmake_minimum_required(VERSION 3.13.1)

find_package(zmk REQUIRED HINTS ../.)
find_package(Zephyr REQUIRED HINTS ../../zephyr)

project(NONE)

target_sources(app PRIVATE main.c)