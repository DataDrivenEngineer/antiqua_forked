build:
	cc -framework AppKit -framework CoreVideo antiqua/*.m -o antiqua.o
package:
	rm antiqua.app/Contents/MacOS/antiqua
	cp -f antiqua.o antiqua.app/Contents/MacOS/antiqua
run:
	open antiqua.app
