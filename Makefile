#kdupreez@hotmail.com

APP=controller-test
SRC_MAIN=src
OUT_DIR=bin

CXX=g++
#G++ options
# To Make Debug Build set: -g
# To Debug with GDB run: gdb -ex=r --args APP ARG1 ARG2
CFLAGS=-g -Wall -ansi
CPP_STD=c++17

#extra include paths
I1=$(SRC_MAIN)
I2=
I3=
I4=
#include folders
INC=$(I1) $(I2) $(I3) $(I4)
INCLUDES=$(foreach d, $(INC), -I$d)

#additional sources files (add extras if needed)
S1=$(SRC_MAIN)/XBOX360.cpp
S2=
S3=
S4=
SOURCES=$(S1) $(S2) $(S3) $(S4)

#lib paths (add extras if needed)
LP1=
LP2=
LP3=
LP4=
LIB_PATH=$(LP1) $(LP2) $(LP3) $(LP4)
LIB_PATHS=$(foreach d, $(LIB_PATH), -L$d)

#link libs (add extras if needed)
L1=usb-1.0
L2=
L3=
L4=pthread
LIB=$(L1) $(L2) $(L3) $(L4)
LIBS=$(foreach d, $(LIB), -l$d)

#pkg-configs (add extras if needed)
PKG1=
PKG2=
PKG3= 
PKG4= 
PKGS=$(PKG1) $(PKG2) $(PKG3) $(PKG4)
ifneq ($(strip $(PKGS)),)
PKG_INCS=$(shell pkg-config --cflags $(PKGS))
PKG_LIBS=$(shell pkg-config --libs $(PKGS))
endif

#defines (add extras if needed)
#env variables can also be passed through with $(ENV_VAR_NAME)
D1="USER_NAME=\"$(USER)\""
D2=
D3=
D4=
DEF=$(D1) $(D2) $(D3) $(D4)
DEFS=$(foreach d, $(DEF), -D$d)

$(APP): $(SRC_MAIN)/$(APP).cpp 
	test -d bin || mkdir -p bin
	$(CXX) $(CFLAGS) -std=$(CPP_STD) $(DEFS) $(SRC_MAIN)/$(APP).cpp $(SOURCES) -o $(OUT_DIR)/$(APP) $(INCLUDES) $(PKG_INCS) $(LIB_PATHS) $(LIBS) $(PKG_LIBS)

clean:
	rm bin/$(APP)
	
