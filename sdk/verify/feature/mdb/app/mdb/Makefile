.PHONY : all clean
all:
	$(MAKE) -C ./console/ all
	$(MAKE) -C ./service/ all
	$(MAKE) -C ./onebin/ all
	$(MAKE) -C ./exec/ all
	$(MAKE) -C ./shell/ all

clean:
	$(MAKE) -C ./shell/ clean
	$(MAKE) -C ./exec/ clean
	$(MAKE) -C ./onebin/ clean
	$(MAKE) -C ./console/ clean
	$(MAKE) -C ./service/ clean
