#!/bin/bash

# This script is set to run using the root project directory.
# You might want to change the relative paths if you're running it outside an IDE.
# Also, don't forget to adjust the paths for the OpenSSL libraries and include files if they are in non-standard locations.

echo "Building KxHTTP"

# Compile the project
# Adjust the include and library paths for OpenSSL as necessary
g++ -s -O2 src/*.cpp -Iinclude -I/usr/local/include -o bin/kxh -L/usr/local/lib -lssl -lcrypto -lpthread

# Check if the build was successful
if [ $? -eq 0 ]; then
    echo "Finished Task"
else
    echo "Build failed"
fi
