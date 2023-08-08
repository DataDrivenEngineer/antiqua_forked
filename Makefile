build:
	cc -g -O0 -framework AppKit -framework CoreVideo -framework QuartzCore antiqua/*.m -o antiqua.o
package:
	rm -f antiqua.app/Contents/MacOS/antiqua
	rm -rf antiqua.app/Contents/MacOS/antiqua.dSYM
	cp -f antiqua.o antiqua.app/Contents/MacOS/antiqua
	cp -rf antiqua.o.dSYM antiqua.app/Contents/MacOS/antiqua.dSYM
run:
	open antiqua.app
