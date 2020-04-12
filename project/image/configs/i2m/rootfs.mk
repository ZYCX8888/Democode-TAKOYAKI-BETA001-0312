IMAGE_INSTALL_DIR:=$(OUTPUTDIR)
-include $(PROJ_ROOT)/../sdk/verify/application/app.mk
-include $(PROJ_ROOT)/release/customer_tailor/$(CUSTOMER_TAILOR)

LIB_DIR_PATH:=$(PROJ_ROOT)/release/$(PRODUCT)/$(CHIP)/common/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)

.PHONY: rootfs root app
rootfs:root app
root:
	cd rootfs; tar xf rootfs.tar.gz -C $(OUTPUTDIR)
	tar xf busybox/$(BUSYBOX).tar.gz -C $(OUTPUTDIR)/rootfs
	tar xf $(LIB_DIR_PATH)/package/$(LIBC).tar.gz -C $(OUTPUTDIR)/rootfs/lib
	cp $(LIB_DIR_PATH)/mi_libs/dynamic/* $(OUTPUTDIR)/rootfs/lib/
	cp $(LIB_DIR_PATH)/ex_libs/dynamic/* $(OUTPUTDIR)/rootfs/lib/

	mkdir -p $(miservice$(RESOUCE))
	if [ "$(BOARD)" = "010A" ]; then \
		cp -rf $(PROJ_ROOT)/board/ini/* $(miservice$(RESOUCE)) ;\
	else \
		cp -rf $(PROJ_ROOT)/board/ini/LCM $(miservice$(RESOUCE)) ;\
	fi;

	if [ "$(BOARD)" = "010A" ]; then \
		cp -rf $(PROJ_ROOT)/board/$(CHIP)/$(BOARD_NAME)/config/* $(miservice$(RESOUCE)) ; \
	else \
		cp -rf $(PROJ_ROOT)/board/$(CHIP)/$(BOARD_NAME)/config/fbdev.ini  $(miservice$(RESOUCE)) ; \
	fi;

	cp -vf $(PROJ_ROOT)/board/$(CHIP)/mmap/$(MMAP)  $(miservice$(RESOUCE))/mmap.ini
	cp -rvf $(LIB_DIR_PATH)/bin/config_tool/*  $(miservice$(RESOUCE))
	cd  $(miservice$(RESOUCE)); chmod +x config_tool; ln -sf config_tool dump_config; ln -sf config_tool dump_mmap
	if [ "$(BOARD)" = "010A" ]; then \
		cp -rf $(PROJ_ROOT)/board/$(CHIP)/pq  $(miservice$(RESOUCE))/ ; \
		find   $(miservice$(RESOUCE))/pq/ -type f ! -name "Main.bin" -type f ! -name "Main_Ex.bin" -type f ! -name "Bandwidth_RegTable.bin"| xargs rm -rf ; \
	fi;

	if [ $(interface_vdec) = "enable" ]; then \
		cp -rf $(PROJ_ROOT)/board/$(CHIP)/vdec_fw  $(miservice$(RESOUCE))/ ; \
	fi;

	mkdir -p $(OUTPUTDIR)/rootfs/config
	cp -rf etc/* $(OUTPUTDIR)/rootfs/etc
	if [ "$(appconfigs$(RESOUCE))" != "" ]; then \
		mkdir -p  $(appconfigs$(RESOUCE)); \
		mkdir -p $(OUTPUTDIR)/rootfs/appconfigs;\
	fi;
	
	if [ $(BENCH) = "yes" ]; then \
		cp -rf /home/edie.chen/bench  $(miservice$(RESOUCE)) ; \
		cp  $(miservice$(RESOUCE))/bench/demo.bash  $(miservice$(RESOUCE))/bench.sh ; \
		chmod 755  $(miservice$(RESOUCE))/bench.sh ; \
	fi;

	if [ "$(PHY_TEST)" = "yes" ]; then \
		mkdir  $(miservice$(RESOUCE))/sata_phy ; \
		cp $(LIB_DIR_PATH)/bin/sata_phy/*  $(miservice$(RESOUCE))/sata_phy ; \
	fi;

	mkdir -p $(OUTPUTDIR)/rootfs/lib/modules/
	mkdir -p  $(miservice$(RESOUCE))/modules/$(KERNEL_VERSION)

	touch ${OUTPUTDIR}/rootfs/etc/mdev.conf
	echo mice 0:0 0660 =input/ >> ${OUTPUTDIR}/rootfs/etc/mdev.conf
	echo mouse.* 0:0 0660 =input/ >> ${OUTPUTDIR}/rootfs/etc/mdev.conf
	echo event.* 0:0 0660 =input/ >> ${OUTPUTDIR}/rootfs/etc/mdev.conf

	echo export PATH=\$$PATH:/config >> ${OUTPUTDIR}/rootfs/etc/profile
	echo export TERMINFO=/config/terminfo >> ${OUTPUTDIR}/rootfs/etc/profile
	sed -i '/^mount.*/d' $(OUTPUTDIR)/rootfs/etc/profile
	echo mkdir -p /dev/pts >> ${OUTPUTDIR}/rootfs/etc/profile
	echo mount -t sysfs none /sys >> $(OUTPUTDIR)/rootfs/etc/profile
	echo mount -t tmpfs mdev /dev >> $(OUTPUTDIR)/rootfs/etc/profile
	echo mount -t debugfs none /sys/kernel/debug/ >>  $(OUTPUTDIR)/rootfs/etc/profile
	echo mdev -s >> $(OUTPUTDIR)/rootfs/etc/profile
	cp -rvf $(PROJ_ROOT)/tools/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/fw_printenv/* $(OUTPUTDIR)/rootfs/etc/
	echo "$(ENV_CFG)" > $(OUTPUTDIR)/rootfs/etc/fw_env.config
	cd $(OUTPUTDIR)/rootfs/etc/;ln -sf fw_printenv fw_setenv
	echo mkdir -p /var/lock >> ${OUTPUTDIR}/rootfs/etc/profile
	if [ "$(FPGA)" = "1" ]; then \
		echo mount -t jffs2 /dev/mtdblock1 /config >> $(OUTPUTDIR)/rootfs/etc/profile ; \
	fi;
	echo -e $(foreach block, $(USR_MOUNT_BLOCKS), "mount -t $($(block)$(FSTYPE)) $($(block)$(MOUNTPT)) $($(block)$(MOUNTTG))\n") >> $(OUTPUTDIR)/rootfs/etc/profile

	chmod 755 $(LIB_DIR_PATH)/bin/debug/*
	cp -rf $(LIB_DIR_PATH)/bin/debug/*  $(miservice$(RESOUCE))
	rm -rf $(OUTPUTDIR)/customer/demo.sh
	mkdir -p $(OUTPUTDIR)/customer
	touch $(OUTPUTDIR)/customer/demo.sh
	chmod 755 $(OUTPUTDIR)/customer/demo.sh


	# creat insmod ko scrpit
	if [ -f "$(PROJ_ROOT)/kbuild/$(KERNEL_VERSION)/$(CHIP)/configs/$(PRODUCT)/$(BOARD)/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/$(FLASH_TYPE)/modules/kernel_mod_list" ]; then \
		cat $(PROJ_ROOT)/kbuild/$(KERNEL_VERSION)/$(CHIP)/configs/$(PRODUCT)/$(BOARD)/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/$(FLASH_TYPE)/modules/kernel_mod_list | \
			sed 's#\(.*\).ko#insmod /config/modules/$(KERNEL_VERSION)/\1.ko#' > $(OUTPUTDIR)/customer/demo.sh; \
		cat $(PROJ_ROOT)/kbuild/$(KERNEL_VERSION)/$(CHIP)/configs/$(PRODUCT)/$(BOARD)/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/$(FLASH_TYPE)/modules/kernel_mod_list | \
			sed 's#\(.*\).ko\(.*\)#$(PROJ_ROOT)/kbuild/$(KERNEL_VERSION)/$(CHIP)/configs/$(PRODUCT)/$(BOARD)/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/$(FLASH_TYPE)/modules/\1.ko#' | \
				xargs -i cp -rvf {}  $(miservice$(RESOUCE))/modules/$(KERNEL_VERSION); \
		echo "#kernel_mod_list" >> $(OUTPUTDIR)/customer/demo.sh; \
	fi;

	if [ -f "$(LIB_DIR_PATH)/modules/$(KERNEL_VERSION)/misc_mod_list" ]; then \
		cat $(LIB_DIR_PATH)/modules/$(KERNEL_VERSION)/misc_mod_list | \
			sed 's#\(.*\).ko#insmod /config/modules/$(KERNEL_VERSION)/\1.ko#' >> $(OUTPUTDIR)/customer/demo.sh; \
		cat $(LIB_DIR_PATH)/modules/$(KERNEL_VERSION)/misc_mod_list | \
			sed 's#\(.*\).ko\(.*\)#$(LIB_DIR_PATH)/modules/$(KERNEL_VERSION)/\1.ko#' | \
				xargs -i cp -rvf {}  $(miservice$(RESOUCE))/modules/$(KERNEL_VERSION); \
		echo "#misc_mod_list" >> $(OUTPUTDIR)/customer/demo.sh; \
	fi;

	if [ -f "$(LIB_DIR_PATH)/modules/$(KERNEL_VERSION)/.mods_depend" ]; then \
		cat $(LIB_DIR_PATH)/modules/$(KERNEL_VERSION)/.mods_depend | \
			sed '2,20s#\(.*\)#insmod /config/modules/$(KERNEL_VERSION)/\1.ko\nif [ $$? -eq 0 ]; then\n	busybox mknod /dev/\1 c $$major $$minor\n	let minor++\nfi\n\n#' >> $(OUTPUTDIR)/customer/demo.sh; \
		cat $(LIB_DIR_PATH)/modules/$(KERNEL_VERSION)/.mods_depend | \
			sed 's#\(.*\)#$(LIB_DIR_PATH)/modules/$(KERNEL_VERSION)/\1.ko#' | \
				xargs -i cp -rvf {}  $(miservice$(RESOUCE))/modules/$(KERNEL_VERSION); \
		echo "#mi module" >> $(OUTPUTDIR)/customer/demo.sh; \
	fi;

	if [ -f "$(LIB_DIR_PATH)/modules/$(KERNEL_VERSION)/misc_mod_list_late" ]; then \
		cat $(LIB_DIR_PATH)/modules/$(KERNEL_VERSION)/misc_mod_list_late | sed 's#\(.*\).ko#insmod /config/modules/$(KERNEL_VERSION)/\1.ko#' >> $(OUTPUTDIR)/customer/demo.sh; \
		cat $(LIB_DIR_PATH)/modules/$(KERNEL_VERSION)/misc_mod_list_late | sed 's#\(.*\).ko\(.*\)#$(LIB_DIR_PATH)/modules/$(KERNEL_VERSION)/\1.ko#' | \
			xargs -i cp -rvf {}  $(miservice$(RESOUCE))/modules/$(KERNEL_VERSION); \
		echo "#misc_mod_list_late" >> $(OUTPUTDIR)/customer/demo.sh; \
	fi;

	sed -i 's/mi_common/insmod \/config\/modules\/$(KERNEL_VERSION)\/mi_common.ko\nmajor=\`cat \/proc\/devices \| busybox awk "\\\\$$2==\\""mi"\\" {print \\\\$$1}"\\n`\nminor=0/g' $(OUTPUTDIR)/customer/demo.sh; \
	sed -i '/#mi module/a	major=`cat /proc/devices | busybox awk "\\\\$$2==\\""mi_poll"\\" {print \\\\$$1}"`\nbusybox mknod \/dev\/mi_poll c $$major 0' $(OUTPUTDIR)/customer/demo.sh; \
	if [ $(PHY_TEST) = "yes" ]; then \
		echo -e "\033[41;33;5m !!! Replace "mdrv-sata-host.ko" with "sata_bench_test.ko" !!!\033[0m" ; \
		sed '/mdrv-sata-host/d' $(OUTPUTDIR)/customer/demo.sh >  $(miservice$(RESOUCE))/demotemp.sh ; \
		echo insmod /config/sata_phy/sata_bench_test.ko >>  $(miservice$(RESOUCE))/demotemp.sh ; \
		cp  $(miservice$(RESOUCE))/demotemp.sh $(OUTPUTDIR)/customer/demo.sh ; \
		rm  $(miservice$(RESOUCE))/demotemp.sh ; \
	fi;

	if [ $(interface_wlan) = "enable" ]; then \
		mkdir -p  $(miservice$(RESOUCE))/wifi ; \
		if [ $(FLASH_TYPE) = "spinand" ]; then \
			cp -rf $(LIB_DIR_PATH)/wifi/libs/ap/*   $(miservice$(RESOUCE))/wifi ; \
			cp -rf $(LIB_DIR_PATH)/wifi/bin/ap/*   $(miservice$(RESOUCE))/wifi ; \
		fi;	\
		find $(LIB_DIR_PATH)/wifi/bin/ -maxdepth 1 -type f -exec cp -P {}  $(miservice$(RESOUCE))/wifi \; ;\
		find $(LIB_DIR_PATH)/wifi/bin/ -maxdepth 1 -type l -exec cp -P {}  $(miservice$(RESOUCE))/wifi \; ;\
		find $(LIB_DIR_PATH)/wifi/libs/ -maxdepth 1 -type f -exec cp -P {}  $(miservice$(RESOUCE))/wifi \; ;\
		find $(LIB_DIR_PATH)/wifi/libs/ -maxdepth 1 -type l -exec cp -P {}  $(miservice$(RESOUCE))/wifi \; ;\
		cp -rf $(LIB_DIR_PATH)/wifi/modules/*   $(miservice$(RESOUCE))/wifi ; \
		cp -rf $(LIB_DIR_PATH)/wifi/configs/*   $(miservice$(RESOUCE))/wifi ; \
	fi;
	if [ "$(appconfigs$(RESOUCE))" != "" ]; then \
		if [ -f "$(miservice$(RESOUCE))/wifi/wpa_supplicant.conf" ]; then	\
			mv  $(miservice$(RESOUCE))/wifi/wpa_supplicant.conf $(appconfigs$(RESOUCE));	\
			cp $(OUTPUTDIR)/appconfigs/wpa_supplicant.conf $(appconfigs$(RESOUCE))/wpa_supplicant.conf_bak;	\
		fi;	\
		cp -rf $(PROJ_ROOT)/board/$(CHIP)/$(BOARD_NAME)/config/model/LCM.ini $(appconfigs$(RESOUCE));	\
	fi;
	# Enable MIU protect on CMDQ buffer as default (While List: CPU)
	# [I5] The 1st 1MB of MIU is not for CMDQ buffer
#	echo 'echo set_miu_block3_status 0 0x70 0 0x100000 1 > /proc/mi_modules/mi_sys_mma/miu_protect' >>  $(miservice$(RESOUCE))/demo.sh

#	echo mount -t jffs2 /dev/mtdblock3 /config >> $(OUTPUTDIR)/rootfs/etc/profile
	ln -fs /config/modules/$(KERNEL_VERSION) $(OUTPUTDIR)/rootfs/lib/modules/
	find  $(miservice$(RESOUCE))/modules/$(KERNEL_VERSION) -name "*.ko" | xargs $(TOOLCHAIN_REL)strip  --strip-unneeded;
	find $(OUTPUTDIR)/rootfs/lib/ -name "*.so*" | xargs $(TOOLCHAIN_REL)strip  --strip-unneeded;
	echo mkdir -p /dev/pts >> $(OUTPUTDIR)/rootfs/etc/profile
	echo mount -t devpts devpts /dev/pts >> $(OUTPUTDIR)/rootfs/etc/profile
	echo "busybox telnetd&" >> $(OUTPUTDIR)/rootfs/etc/profile
	echo "if [ -e /customer/demo.sh ]; then" >> $(OUTPUTDIR)/rootfs/etc/profile
	echo "    /customer/demo.sh" >> $(OUTPUTDIR)/rootfs/etc/profile
	echo "fi;" >> $(OUTPUTDIR)/rootfs/etc/profile
	echo mdev -s >> $(OUTPUTDIR)/customer/demo.sh
	if [ $(BENCH) = "yes" ]; then \
		echo ./config/bench.sh >> $(OUTPUTDIR)/customer/demo.sh ; \
	fi;
	if [ "$(BOARD)" = "011A" ]; then \
		sed -i 's/mi_sys.ko/mi_sys.ko cmdQBufSize=128 logBufSize=0/g' $(OUTPUTDIR)/customer/demo.sh ;\
	fi;

	mkdir -p $(OUTPUTDIR)/vendor
	mkdir -p $(OUTPUTDIR)/customer
	mkdir -p $(OUTPUTDIR)/rootfs/vendor
	mkdir -p $(OUTPUTDIR)/rootfs/customer
