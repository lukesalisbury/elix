#This makefile creates the files needed to build the project with Ninja
BUILD_DIRECTORY=build

ninja_default="./$(BUILD_DIRECTORY)/default.ninja"
ninja_config="./$(BUILD_DIRECTORY)/config.ninja"
ninja_files="./$(BUILD_DIRECTORY)/files.ninja"
ninja_build="./$(BUILD_DIRECTORY)/local.ninja"

command_write="./$(BUILD_DIRECTORY)/writeout.exe"
command_remove=rm
command_mkdir=mkdir -p
command_tr=tr

BUILD_PLATFORM=$(strip ${shell uname})
BUILD_MACHINE=$(strip ${shell uname -m})
BUILD_DEBUG?=TRUE

PLATFORM_PROCESSOR?=x86
PLATFORM_BITS?=unknown

ifeq ($(BUILD_PLATFORM), Windows)
	command_remove=scripts\rm
	command_mkdir=scripts\mkdir
	command_tr=scripts\tr
endif

${shell $(command_mkdir) $(BUILD_DIRECTORY) }
${shell gcc scripts/writeout.c -s -o $(command_write) }

#Build platform & target
ifeq ($(PLATFORM_OS), ) # No platform set, so we guess
	PLATFORM_OS=windows
	ifeq ($(BUILD_PLATFORM), Linux)
		PLATFORM_OS=linux
	endif
	ifeq ($(BUILD_PLATFORM), Apple)
		PLATFORM_OS=apple
	endif
	ifeq ($(BUILD_PLATFORM), Darwin)
		PLATFORM_OS=apple
	endif
	ifeq ($(BUILD_PLATFORM), Haiku)
		PLATFORM_OS=haiku
	endif
endif

#Plaform bits
ifeq ($(PLATFORM_BITS), unknown) # No platform set, so we guess
	ifeq ($(BUILD_MACHINE), x86_64)
		PLATFORM_BITS=64
		PLATFORM_PROCESSOR=x86_64
	endif
	ifeq ($(BUILD_MACHINE), x86)
		PLATFORM_BITS=32
		PLATFORM_PROCESSOR=x86
	endif
	ifeq ($(BUILD_MACHINE), i686)
		PLATFORM_BITS=32
		PLATFORM_PROCESSOR=x86
	endif
	ifeq ($(BUILD_MACHINE), i386)
		PLATFORM_BITS=32
		PLATFORM_PROCESSOR=x86
	endif
	ifeq ($(BUILD_MACHINE), armv6l)
		PLATFORM_BITS=32
		PLATFORM_PROCESSOR=ARM
	endif
endif

PLATFORM_DEFINES=

PLATFORM_NAME=$(strip ${shell echo $(PLATFORM_OS)-$(PLATFORM_PROCESSOR) | $(command_tr) 'A-Z' 'a-z'})

ninja_default="$(BUILD_DIRECTORY)/default.ninja"
ninja_config="$(BUILD_DIRECTORY)/$(PLATFORM_NAME)/config.ninja"
ninja_files="$(BUILD_DIRECTORY)/$(PLATFORM_NAME)/files.ninja"
ninja_build="$(BUILD_DIRECTORY)/$(PLATFORM_NAME)/build.ninja"

${shell $(command_mkdir) $(BUILD_DIRECTORY)/$(PLATFORM_NAME) }

#Build
.PHONY: clean options info

include scripts/settings/options.mk
include scripts/modules/*.mk

all: debug
	@echo Run ninja to build
	@$(command_remove) $(command_write)
#	ninja 1>&2

clean:
#	ninja clean 1>&2
	$(command_remove) $(ninja_default)
	$(command_remove) $(ninja_files)
	$(command_remove) $(ninja_config)
	$(command_remove) $(command_write)

info: 
	@echo --------------------------------
	@echo Creating Ninja Build Scripts
	@echo Building on: $(BUILD_PLATFORM)
	@echo Targeting: $(PLATFORM_OS)-$(PLATFORM_PROCESSOR)
	@echo --------------------------------


$(ninja_files): $(MAINCODE) $(SHAREDCODE) $(BINARIES)
	${shell $(command_write) " " >> $(ninja_files)}
	${shell $(command_write) "default $(patsubst %.exe,\$${binary_prefix}%\$${binary_suffix},$(notdir $(BINARIES)))" >> $(ninja_files)}

%.so:
	${shell $(command_write) "build $(patsubst %.so,\$${binary_prefix}%\$${binary_suffix},$(notdir $@)): link \$${object_dir}/$(@:%.exe=%.o) $(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$(SHAREDCODE:%=\$${object_dir}/%)))" >> $(ninja_files)}

%.exe:
	${shell $(command_write) "build $(patsubst %.exe,\$${binary_prefix}%\$${binary_suffix},$(notdir $@)): link \$${object_dir}/$(@:%.exe=%.o) $(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$(SHAREDCODE:%=\$${object_dir}/%)))" >> $(ninja_files)}

%.cpp:
	${shell $(command_write) "build \$${object_dir}/$(@:%.cpp=%.o): compile_cpp ./src/$@" >> $(ninja_files)}

%.c:
	${shell $(command_write) "build \$${object_dir}/$(@:%.c=%.o): compile_c ./src/$@" >> $(ninja_files)}

%.asm:
	${shell $(command_write) "build \$${object_dir}/$(@:%.asm=%.o): compile_asm ./src/$@" >> $(ninja_files)}

file_header:
	${shell $(command_write) "#FileObjects" > $(ninja_files)}

file: file_header $(ninja_files)
	@echo Creating File list: $(ninja_files)

config_header:
	${shell $(command_write) "#Config" > $(ninja_config)}
	${shell $(command_write) "platform_ninja=$(PLATFORM_NAME)" > $(ninja_default)}
	${shell $(command_write) "platform_ninja=$(PLATFORM_NAME)" > $(ninja_build)}
	${shell $(command_write) "include core.ninja" >> $(ninja_build)}

include scripts/settings/configlocal.mk

config: config_local
	@echo Creating Config: $(ninja_config)

debug: info config file
	${shell $(command_write) "compile_mode=debug" >> $(ninja_config)}
	${shell $(command_write) "debug_flag=-O0 -g3" >> $(ninja_config)}
	${shell $(command_write) "platform_defines=-DDEBUG $(PLATFORM_DEFINES)" >> $(ninja_config)}


release: info config file
	${shell $(command_write) "compile_mode=release" >> $(ninja_config)}
	${shell $(command_write) "platform_defines=$(PLATFORM_DEFINES)" >> $(ninja_config)}
