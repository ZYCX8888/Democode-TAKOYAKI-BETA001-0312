all:
	$(MAKE) -f ./MakeObj.mk all
	$(MAKE) -f ./MakeLib.mk all
	$(MAKE) -f ./MakeAppShm.mk all
	$(MAKE) -f ./MakeAppSocket.mk all
clean:
	$(MAKE) -f ./MakeAppShm.mk clean
	$(MAKE) -f ./MakeAppSocket.mk clean
	$(MAKE) -f ./MakeLib.mk clean
	$(MAKE) -f ./MakeObj.mk clean