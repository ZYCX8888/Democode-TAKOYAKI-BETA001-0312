PROG:=add_nalu_head_info del_nalu_head_info es_mess
PROG_CLEAN:=$(patsubst %, %_clean, $(PROG))

.PHONY:$(PROG) $(PROG_CLEAN)


all:$(PROG)
	@echo all is done

clean:$(PROG_CLEAN)
	@echo all is clean

$(PROG):
	@echo building $@...
	gcc -o $@ $(patsubst %, %.c, $@)
	@echo build $@ done

$(PROG_CLEAN):
	rm $(patsubst %_clean, %, $@)

