# This is ONLY for the playpen, because it's just used for testing/dev.
# The expected build setup for creating the DLL is MSVC

CXX=x86_64-w64-mingw32-g++
CC=x86_64-w64-mingw32-gcc
CXXFLAGS=-Werror -std=c++17 -static
CFLAGS=-Werror -std=c11 -static

OBJECTS=layeredfs/playpen.o layeredfs/utils.o layeredfs/log.o \
	layeredfs/config.o layeredfs/avs.o layeredfs/hook.o \
	layeredfs/ramfs_demangler.o layeredfs/modpath_handler.o layeredfs/dllmain.o \
	layeredfs/texture_packer.o \
	minhook/src/hook.o minhook/src/buffer.o minhook/src/trampoline.o \
	minhook/src/hde/hde32.o minhook/src/hde/hde64.o \
	layeredfs/3rd_party/lodepng.o \
	layeredfs/3rd_party/GuillotineBinPack.o layeredfs/3rd_party/stb_dxt.o

playpen.exe: $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o playpen.exe $(OBJECTS)

clean:
	rm -f $(OBJECTS) playpen.exe
