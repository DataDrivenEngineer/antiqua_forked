build:
	# Create lock file to prevent code hot reloading during compilation
	echo "WAITING FOR COMPILATION" > lock.tmp
	# Compile all Obj-C files
	cc -c -g -DANTIQUA_SLOW=1 -DANTIQUA_INTERNAL=1 -ffast-math -fno-rtti -fno-exceptions -O0 -Wall -pedantic -Wno-dtor-name -Wno-null-dereference -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -Wno-gnu-anonymous-struct -Wno-nested-anon-types -Wno-nullability-completeness antiqua/antiqua.m -o m.o
	# Compile all C++ files
	cc -shared -g -std=c++17 -DANTIQUA_SLOW=1 -DANTIQUA_INTERNAL=1 -ffast-math -fno-rtti -fno-exceptions -O0 -Wall -pedantic -Wno-dtor-name -Wno-null-dereference -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -Wno-gnu-anonymous-struct -Wno-nested-anon-types -Wno-nullability-completeness antiqua/antiqua.cpp -fvisibility=hidden -o libantiqua.dylib
	# Compile Metal shaders
	xcrun -sdk macosx metal -o shaders.ir -c antiqua/shaders.metal
	xcrun -sdk macosx metallib -o shaders.metallib shaders.ir
	# Copy compiled Metal library to the bundle
	cp -f shaders.metallib antiqua.app/Contents/Resources/shaders.metallib
	# Link
	cc -g -O0 -framework Carbon -framework AppKit -framework CoreVideo -framework QuartzCore -framework CoreAudio -framework IOKit -framework Metal m.o -o antiqua.o
	# Generate debug symbols
	dsymutil antiqua.o
	# Remove lock file when compilation is done to allow code hot reloading
	rm -rf lock.tmp
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
	# Remove XCode's build info
	rm -rf xcode_build
	rm -rf build
	rm -rf libantiqua*
	# Remove executable and debug symbols from the app's bundle
	rm -rf antiqua.app/Contents/MacOS/*
	# Remove executable and debug symbols from the build location
	rm -rf *.o*
	# Remove compiled shaders
	rm -rf *.ir
	rm -rf *.metallib
	# Remove shaders library from the app's bundle
	rm -rf antiqua.app/Contents/Resources/*
	echo "clean success"
