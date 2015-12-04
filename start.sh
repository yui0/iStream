#!/bin/sh
export LD_LIBRARY_PATH="$(pwd)"
./mjpg_streamer -i "./input_screen.so -d 0" -o "./output_http.so -w ./www -p 17743"
