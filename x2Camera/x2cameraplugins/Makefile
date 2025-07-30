include ./rules.mk
x2cameraASI: linuxstub.o x2camera.o main.o
ifeq ($(APPLE), 0)

	$(CC) $(CFLAGS) $(DEFS) -shared -Wall -o x2cameraASI.so x2camera.o main.o ../../../linux/lib/$(platform)/libASICamera2.a 
	mv -f x2cameraASI.so lib/$(platform)
else
	$(CC) $(CFLAGS) $(DEFS) -dynamiclib -Wall -current_version $(VERSION) -compatibility_version 1.0.0 -o x2cameraASI.dylib x2camera.o main.o ../../../linux/lib/$(platform)/libASICamera2.a 
ifeq ($(FakeUSB), n)
	#	install_name_tool -change /usr/local/lib/libusb-1.0.0.dylib @loader_path/libusb-1.0.0.dylib x2cameraASI.dylib
		install_name_tool -change /usr/local/lib/libusb-1.0.0.dylib "@executable_path/../Frameworks/libusb-1.0.0.dylib" x2cameraASI.dylib
		#cp ../../../linux/libusb/$(platform)/libusb-1.0.0.dylib lib/$(platform)
		cp ../../../linux/libusb/$(platform)/libusb-1.0.0-1.0.23.dylib lib/$(platform)/libusb-1.0.0.dylib
endif
	mv -f x2cameraASI.dylib lib/$(platform)
endif

#linuxstub.o:
#	$(CC) -c $(CFLAGS) $(DEFS) linuxstub.cpp
x2camera.o:
	$(CC) -c $(CFLAGS) $(DEFS) x2camera.cpp -I../../../ASICamera -I../../../tinyXML -I.
main.o:
	$(CC) -c $(CFLAGS) $(DEFS) main.cpp -I../../../ASICamera -I../../../tinyXML -I.

clean :
	rm *.o *.dylib *so

