build:
	cc -framework AppKit -framework CoreVideo -framework QuartzCore antiqua/*.m -o antiqua.o
package:
	rm antiqua.app/Contents/MacOS/antiqua
	cp -f antiqua.o antiqua.app/Contents/MacOS/antiqua
run:
	open antiqua.app
