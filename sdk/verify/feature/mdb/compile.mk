.PHONY :all clean gen_exe gen_obj clean_files gen_lib

include $(DB_BUILD_TOP)/define.mk
include $(PROJ_ROOT)/configs/current.configs
ifneq ($(USE_X86), 1)
include $(PROJ_ROOT)/release/$(PRODUCT)/$(CHIP)/$(BOARD)/$(TOOLCHAIN)/toolchain.mk
else
CC := gcc
CXX := g++
AR := ar
STRIP := strip
endif

#GCCFLAGS := -Wall -g -Werror
GCCFLAGS := -g
CXXFLAGS := $(GCCFLAGS) $(LOCAL_CXXFLAGS)
CXXFLAGS += $(CODEDEFINE) -DLINUX_OS
CXXFLAGS += $(foreach dir,$(INC),-I$(dir))

CFLAGS := $(GCCFLAGS) $(LOCAL_CFLAGS)
CFLAGS += $(CODEDEFINE) -DLINUX_OS
CFLAGS += $(foreach dir,$(INC),-I$(dir))

SRC_CXX  +=  $(foreach dir,$(SUBDIRS),$(wildcard $(dir)/*.cpp))
SRC      +=  $(foreach dir,$(SUBDIRS),$(wildcard $(dir)/*.c))
ifneq ($(USE_X86), 1)
OBJS_CXX := $(patsubst %.cpp,%.o,$(SRC_CXX))
OBJS     := $(patsubst %.c,%.o,$(SRC))
else
OBJS_CXX := $(patsubst %.cpp,%_x86.o,$(SRC_CXX))
OBJS     := $(patsubst %.c,%_x86.o,$(SRC))
endif

DFILES := $(foreach dir,$(SUBDIRS),$(wildcard $(dir)/*.d))
sinclude $(DFILES)
#@$(CC) $(CFLAGS) -MM $< | sed -e 's/\(.*\)\.o/\$$\(SUBDIRS\)\/\1.o/g' > $(@:.o=.d)
ifneq ($(USE_X86), 1)
$(OBJS):%.o:%.c
	@echo compile $<...
	@$(CC) $(CFLAGS) -MM $< | sed -e '1s/'"^.*"'/'"$(subst /,"'\/'",$@) : $(subst /,"'\/'",$(@:.o=.d)) $(subst /,"'\/'",$<)"'\\''/' > $(@:.o=.d)
	@$(CC) $(CFLAGS) -c $< -o $@
$(OBJS_CXX):%.o:%.cpp
	@echo compile $<...
	@$(CXX) $(CXXFLAGS) -MM $< | sed -e '1s/'"^.*"'/'"$(subst /,"'\/'",$@) : $(subst /,"'\/'",$(@:.o=.d)) $(subst /,"'\/'",$<)"'\\''/' > $(@:.o=.d)
	@$(CXX) $(CXXFLAGS) -c $< -o $@
else
$(OBJS):%_x86.o:%.c
	@echo compile $<...
	@$(CC) $(CFLAGS) -MM $< | sed -e '1s/'"^.*"'/'"$(subst /,"'\/'",$@) : $(subst /,"'\/'",$(@:.o=.d)) $(subst /,"'\/'",$<)"'\\''/' > $(@:.o=.d)
	@$(CC) $(CFLAGS) -c $< -o $@
$(OBJS_CXX):%_x86.o:%.cpp
	@echo compile $<...
	@$(CXX) $(CXXFLAGS) -MM $< | sed -e '1s/'"^.*"'/'"$(subst /,"'\/'",$@) : $(subst /,"'\/'",$(@:.o=.d)) $(subst /,"'\/'",$<)"'\\''/' > $(@:.o=.d)
	@$(CXX) $(CXXFLAGS) -c $< -o $@
endif

gen_exe:$(OBJS_CXX) $(OBJS)
ifneq ($(DB_OUT_PATH), )
ifneq ($(OBJS_CXX), )
	@$(CXX) $(OBJS_CXX) $(OBJS) $(LIBS) -o $(EXEFILE)
	@mkdir -p $(OUTPUT_DIR)
	@mv $(EXEFILE) $(OUTPUT_DIR) -v
else
ifneq ($(OBJS), )
	@$(CC) $(CFLAGS) $(OBJS) $(LIBS) -o $(EXEFILE)
	@mkdir -p $(OUTPUT_DIR)
	@mv $(EXEFILE) $(OUTPUT_DIR) -v
else
	@echo "Error no object files!!!"
endif
endif
else
	@echo "Please source xxx.sh first!"
endif

gen_obj:$(OBJS_CXX) $(OBJS)
ifneq ($(DB_OUT_PATH), )
ifneq ($(OBJS_CXX), )
	@mkdir -p $(OUTPUT_DIR)
	@cp $(OBJS_CXX) $(OUTPUT_DIR) -v
endif
ifneq ($(OBJS), )
	@mkdir -p $(OUTPUT_DIR)
	@cp $(OBJS) $(OUTPUT_DIR) -v
endif
else
	@echo "Please source xxx.sh first!"
endif

gen_lib:
ifeq ($(LIB_TYPE), static)
	@$(AR) sq lib$(LIB_NAME).a $(OBJ_FILES)
ifneq ($(OUTPUT_DIR), )
	@mkdir -p $(OUTPUT_DIR)
	@mv ./lib$(LIB_NAME).a $(OUTPUT_DIR) -v
endif
endif
ifeq ($(LIB_TYPE), shared)
	@$(CC) -rdynamic -ldl -fPIC  -shared -o lib$(LIB_NAME).so $(OBJ_FILES)
ifneq ($(OUTPUT_DIR), )
	@mkdir -p $(OUTPUT_DIR)
	@mv ./lib$(LIB_NAME).so $(OUTPUT_DIR) -v
endif
endif
ifeq ($(LIB_TYPE), shared_cxx)
	@$(CXX) -rdynamic -ldl -fPIC  -shared -o lib$(LIB_NAME).so -Wl,-soname,lib$(LIB_NAME).so $(OBJ_FILES)
ifneq ($(OUTPUT_DIR), )
	@mkdir -p $(OUTPUT_DIR)
	@mv ./lib$(LIB_NAME).so $(OUTPUT_DIR) -v
endif
endif
ifneq ($(LIB_TYPE), )
	@echo ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>"
	@echo lib$(LIB_NAME) is ready!!!!!!!
	@echo ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>"
else
	@echo "Error LIB_TYPE not set!!"
endif

clean_files:
ifneq ($(DB_OUT_PATH), )
	@rm -rvf $(OBJS_CXX) $(OBJS) $(OBJS_CXX:.o=.d) $(OBJS:.o=.d)
	@rm -rvf $(OUTPUT_DIR)
ifneq ($(EXEFILE), )
	@rm -rvf $(OUTPUT_DIR)/$(EXEFILE)
endif
else
	@echo "Please source xxx.sh first!"
endif
