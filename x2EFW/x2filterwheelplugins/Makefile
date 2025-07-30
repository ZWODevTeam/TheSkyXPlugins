include ./rules.mk
x2filterwheelEFW: x2filterwheel.o main.o

ifeq ($(IsLin), 1)

	$(CC) $(CFLAGS) $(DEFS) -shared -Wall -o x2filterwheelEFW.so x2filterwheel.o main.o ../../../FilterSDK/lib/$(platform)/libEFWFilter.a $(UDEV)

	mv -f x2filterwheelEFW.so lib/$(platform)
else

	$(CC) $(CFLAGS) $(DEFS) -dynamiclib -Wall -current_version $(VERSION) -compatibility_version 1.0.0 -o x2filterwheelEFW.dylib x2filterwheel.o main.o ../../../FilterSDK/lib/$(platform)/libEFWFilter.a $(UDEV)

	mv -f x2filterwheelEFW.dylib lib/$(platform)
endif

x2filterwheel.o:
	$(CC) -c $(CFLAGS) $(DEFS) x2filterwheel.cpp -I. $(UDEV)
main.o:
	$(CC) -c $(CFLAGS) $(DEFS) main.cpp -I. $(UDEV)

clean :
	rm *.o *.dylib *.so

clean_obj:
	test -f *.o && rm *.o 

