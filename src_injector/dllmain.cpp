#include <stdio.h>

// because GCC's exports aren't as powerful as MSVC's pragma(comment), we need
// to name our functions identically to the real ones, which actually collides
// with the real ones if we include <windows.h>. So instead, include these
// lesser-used actual imports
#include <libloaderapi.h>
#include <sysinfoapi.h>

#define DLL_TO_HOOK "ifs_hook.dll"
//#define DO_LOG

// leave a bunch of room in the string so aspiring modders can just hex edit the filename
#define HELP "\0Hello hex editor - The DLL name + this message have 256 bytes allocated for them. You can change the DLL name if you want!"
const char DLL_TO_LOAD[256] = DLL_TO_HOOK HELP;

#if defined(USE_D3D9) // Anything that uses D3d9.dll, which is most games
	#define DLL_NAME L"d3d9.dll"

	#define FOREACH_D3D_FUNC(X) \
		X("16,NONAME", __ord16) \
		X("17,NONAME", __ord17) \
		X("18,NONAME", __ord18) \
		X("19,NONAME", __ord19) \
		X("20", Direct3DCreate9On12) \
		X("21", Direct3DCreate9On12Ex) \
		X("22,NONAME", __ord22) \
		X("23,NONAME", __ord23) \
		X("24", Direct3DShaderValidatorCreate9) \
		X("25", PSGPError) \
		X("26", PSGPSampleTexture) \
		X("27", D3DPERF_BeginEvent) \
		X("28", D3DPERF_EndEvent) \
		X("29", D3DPERF_GetStatus) \
		X("30", D3DPERF_QueryRepeatFrame) \
		X("31", D3DPERF_SetMarker) \
		X("32", D3DPERF_SetOptions) \
		X("33", D3DPERF_SetRegion) \
		X("34", DebugSetLevel) \
		X("35", DebugSetMute) \
		X("36", Direct3D9EnableMaximizedWindowedModeShim) \
		X("37", Direct3DCreate9) \
		X("38", Direct3DCreate9Ex) \

#elif defined(USE_DXGI) // Anything newer will include dxgi transitively, so D3d10/11/12 are all covered
	#define DLL_NAME L"dxgi.dll"

	#define FOREACH_D3D_FUNC(X) \
		X("1", ApplyCompatResolutionQuirking) \
		X("2", CompatString) \
		X("3", CompatValue) \
		X("4", DXGIDumpJournal) \
		X("5", PIXBeginCapture) \
		X("6", PIXEndCapture) \
		X("7", PIXGetCaptureState) \
		X("8", SetAppCompatStringPointer) \
		X("9", UpdateHMDEmulationStatus) \
		X("10", CreateDXGIFactory) \
		X("11", CreateDXGIFactory1) \
		X("12", CreateDXGIFactory2) \
		X("13", DXGID3D10CreateDevice) \
		X("14", DXGID3D10CreateLayeredDevice) \
		X("15", DXGID3D10GetLayeredDeviceSize) \
		X("16", DXGID3D10RegisterLayers) \
		X("17", DXGIDeclareAdapterRemovalSupport) \
		X("18", DXGIGetDebugInterface1) \
		X("19", DXGIReportAdapterConfiguration) \

#elif defined(USE_OGL) // OpenGL used by jubeat, older gfdm
    #define DLL_NAME L"opengl32.dll"

    // this one has tons of defs so put it in a separate file
	#include "opengl_defs.h"

#else
	#error choose a valid DLL to hook
#endif

// Notes on register preservation and safety etc
// cdecl/stdcall both push args on stack, on x64 also uses registers.
//
// On x86, with at least 1 arg in the wrapped function, the compiler
// correctly generates stack preservation code.
//
// Note! If the wrapped function is cdecl instead of stdcall, it'll
// need arg propagation. Having it be stdcall makes it a jmp.
//
// On x64, things are more complex because stdcall isn't used.
// 0-arg functions are fine, but because args are placed in registers
// (RCX, RDX, R8, R9) and only then overflowed into stack, they need anti-clobber
// code.
//
// Thus, by making the wrapped function have 4 args and stdcall, we
// generate correct register preservation for both x86 and x64 targets.
// If the function isn't stdcall or cdecl, you're out of luck, but I've
// never seen a DLL do that so it Should Totes Be Fine Yo (TM).

#define DECLARE_ORIGINAL(ordinal, name) void (WINAPI *real_ ## name)(void* a, void* b, void* c, void* d);
FOREACH_D3D_FUNC(DECLARE_ORIGINAL);

#ifdef DO_LOG
FILE* log;
#define LOG(...) if (log) fprintf(log, __VA_ARGS__); fflush(log);

#else
#define LOG(...)

#endif

void __cdecl onetime_setup(const char *fn_name) {
	static bool setup_complete = false;

	LOG("%s\n", fn_name);

	if (setup_complete)
		return;

#ifdef BOMBERGIRL_BULLSHIT
	// it rewrites the path loader and needs "_dxgi.dll" copied out of system32
	// since the loader doesn't deal with full paths properly
	HMODULE orig = LoadLibraryW(L"_" DLL_NAME);
#else
	wchar_t path[MAX_PATH];
	if (GetSystemDirectoryW(path, sizeof(path)) == 0) {
		LOG("Couldn't GetSystemDirectoryW, enjoy your crash\n");
		return;
	}

	if (wcslen(path) + wcslen(L"\\" DLL_NAME) >= MAX_PATH) {
		LOG("Original DLL path length exceeds MAX_PATH, enjoy your crash\n");
		return;
	}

	// can't use wcscat_s, not available on XP
	wcscat(path, L"\\" DLL_NAME);
	HMODULE orig = LoadLibraryW(path);
#endif

#ifdef DO_LOG
	fopen_s(&log, "d3d_hook.log", "w");
#endif

	if (!orig) {
		LOG("Couldn't load original dll, enjoy your crash\n");
	}

#define LOAD_ORIGINAL(ordinal, name) real_ ## name = (decltype(real_ ## name))GetProcAddress(orig, #name);

	FOREACH_D3D_FUNC(LOAD_ORIGINAL);

	// off we go
	LoadLibraryA(DLL_TO_LOAD);
	setup_complete = true;

	LOG("Hook loaded!\n");
}

#define REPLICATE_FUNC(ordinal, name) \
extern "C" void __stdcall name(void* a, void* b, void* c, void* d) { \
/* Using this avoids name mangling for stdcall functions (which happens even with extern "C"!) */ \
/* It also removes the need for a .def file */ \
/* ...but only for MSVC. GCC's asm(".section .drectve") and -export directive does not work on ordinals, nor on mangled names */ \
/*__pragma(comment(linker, "/EXPORT:" __FUNCTION__ "=" __FUNCDNAME__ ",@" ordinal))*/ \
	onetime_setup(__FUNCTION__); \
	return real_ ## name(a,b,c,d); \
}

FOREACH_D3D_FUNC(REPLICATE_FUNC);

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    return TRUE;
}

