build:
	# Compile all Obj-C files
	cc -c -g -DANTIQUA_SLOW=1 -DANTIQUA_INTERNAL=1 -O0 -Wall -pedantic -Wnull-dereference -Wno-unused-but-set-variable -Wno-gnu-anonymous-struct -Wno-nested-anon-types antiqua/antiqua.m -o m.o
	# Compile all C++ files
	cc -c -g -DANTIQUA_SLOW=1 -DANTIQUA_INTERNAL=1 -O0 -std=c++17 -Wall -pedantic -Wno-null-dereference -Wno-unused-but-set-variable -Wno-gnu-anonymous-struct -Wno-nested-anon-types antiqua/antiqua.cpp -o cpp.o
	# Link
	cc -g -O0 -framework Carbon -framework AppKit -framework CoreVideo -framework QuartzCore -framework CoreAudio -framework IOKit m.o cpp.o -o antiqua.o
	# Generate debug symbols
	dsymutil antiqua.o
	echo ""
package:
	cp -f antiqua.o antiqua.app/Contents/MacOS/antiqua
	cp -rf antiqua.o.dSYM antiqua.app/Contents/MacOS/antiqua.dSYM
run:
	open antiqua.app
clean:
	# Remove executable and debug symbols from the app's bundle
	rm -rf antiqua.app/Contents/MacOS/antiqua*
	# Remove executable and debug symbols from the build location
	rm -rf *.o*
	echo ""
