include ./rules.mk
x2focuserEAF: x2focuser.o main.o

ifeq ($(IsLin), 1)

	$(CC) $(CFLAGS) $(DEFS) -shared -Wall -o x2focuserEAF.so x2focuser.o main.o ../../../FocuserSDK/lib/$(platform)/libEAFFocuser.a $(UDEV)

	test -d lib/$(platform) || mkdir -p lib/$(platform)
	mv -f x2focuserEAF.so lib/$(platform)
else

	$(CC) $(CFLAGS) $(DEFS) -dynamiclib -Wall -current_version $(VERSION) -compatibility_version 1.0.0 -o x2focuserEAF.dylib x2focuser.o main.o ../../../FocuserSDK/lib/$(platform)/libEAFFocuser.a $(UDEV)

	test -d lib/$(platform) || mkdir -p lib/$(platform)
	mv -f x2focuserEAF.dylib lib/$(platform)
endif

x2focuser.o:
	$(CC) -c $(CFLAGS) $(DEFS) x2focuser.cpp -I. $(UDEV)
main.o:
	$(CC) -c $(CFLAGS) $(DEFS) main.cpp -I. $(UDEV)

clean :
	rm *.o *.dylib *.so

clean_obj:
	test -f *.o && rm *.o 

