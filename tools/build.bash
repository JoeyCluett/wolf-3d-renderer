#!/bin/bash

g++ -o sdl_image_show sdl_image_show.cpp -std=c++11 -march=native -lSDL
gcc -o compress compress.c -march=native -O3
