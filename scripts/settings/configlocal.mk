#Note: Local platform config


config_local: config_header
	@echo --------------------------------
ifeq ($(BUILDPLATFORM), windows)
#	${shell $(writecommand) "platform_includes=-I\"$(SUPPORTPATH)/include\" " >> $(configninja)}
#	${shell $(writecommand) "platform_lib_flags=-L\"$(SUPPORTPATH)/lib\"" >> $(configninja)}
else
#	@echo platform_includes=${shell sdl2-config --cflags} >> $(configninja)
#	@echo platform_lib_flags=${shell sdl2-config --libs} >> $(configninja)
endif

options: 
	@echo --------------------------------
	@echo Build Options
	@echo - CONFIG_BUILDTEST: $(CONFIG_BUILDTEST)
	@echo --------------------------------


