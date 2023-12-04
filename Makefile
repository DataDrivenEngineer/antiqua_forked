build:
	# Compile all Obj-C files
	cc -c -g -DANTIQUA_SLOW=1 -DANTIQUA_INTERNAL=1 -ffast-math -fno-rtti -fno-exceptions -O0 -Wall -pedantic -Wno-null-dereference -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -Wno-gnu-anonymous-struct -Wno-nested-anon-types -Wno-nullability-completeness antiqua/antiqua.m -o m.o
	# Compile all C++ files
	cc -shared -g -DANTIQUA_SLOW=1 -DANTIQUA_INTERNAL=1 -ffast-math -fno-rtti -fno-exceptions -O0 -std=c++17 -Wall -pedantic -Wno-null-dereference -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -Wno-gnu-anonymous-struct -Wno-nested-anon-types -Wno-nullability-completeness antiqua/antiqua.cpp -fvisibility=hidden -o libantiqua.dylib
	# Link
	cc -g -O0 -framework Carbon -framework AppKit -framework CoreVideo -framework QuartzCore -framework CoreAudio -framework IOKit m.o -o antiqua.o
	# Generate debug symbols
	dsymutil antiqua.o
	echo "compile success"
package:
	cp -f antiqua.o antiqua.app/Contents/MacOS/antiqua
	cp -rf antiqua.o.dSYM antiqua.app/Contents/MacOS/antiqua.dSYM
	mv libantiqua.dylib.dSYM libantiquatmp.dylib.dSYM
	echo "package success"
run:
	open antiqua.app
clean:
	# Remove game's state recordings
	rm -rf tmp/*
	# Remove XCode's build info
	rm -rf xcode_build
	rm -rf build
	rm -rf libantiqua*
	# Remove executable and debug symbols from the app's bundle
	rm -rf antiqua.app/Contents/MacOS/*
	# Remove executable and debug symbols from the build location
	rm -rf *.o*
	echo "clean success"
