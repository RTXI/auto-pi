
PLUGIN_NAME = auto_pi

HEADERS = auto-pi.h

SOURCES = auto-pi.cpp\
	moc_auto-pi.cpp

LIBS = 

### Do not edit below this line ###

include $(shell rtxi_plugin_config --pkgdata-dir)/Makefile.plugin_compile
