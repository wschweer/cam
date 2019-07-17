
cam:
	cd build; make -j16

t:
	./build/cam

d:
	gdb ./build/cam core


