#include <windows.h>
#include <io.h>

typedef BOOL (WINAPI *IsProcessorFeaturePresent_func)(DWORD ProcessorFeature);
static IsProcessorFeaturePresent_func IsProcessorFeaturePresent_proc = NULL;
static BOOL IsProcessorFeaturePresent_set = FALSE;

BOOL WINAPI IsProcessorFeaturePresent95(DWORD ProcessorFeature)
{
	HMODULE hK = NULL;
	
	if(!IsProcessorFeaturePresent_set)
	{
		hK = GetModuleHandleA("kernel32.dll");
		if(hK)
		{
			IsProcessorFeaturePresent_proc =
			(IsProcessorFeaturePresent_func)GetProcAddress(hK, "IsProcessorFeaturePresent");
		}
		IsProcessorFeaturePresent_set = TRUE;
	}
	
	if(IsProcessorFeaturePresent_proc)
	{
		return IsProcessorFeaturePresent_proc(ProcessorFeature);
	}
	
	return FALSE;
}

BOOL WINAPI IsProcessorFeaturePresent(DWORD ProcessorFeature)
{
	return IsProcessorFeaturePresent95(ProcessorFeature);
}

/* Don't use this on mingw 4.x.x */
#if __GNUC__ > 4

void __fastfail(unsigned int code);

void __cdecl __attribute__((__noreturn__)) __chk_fail(void) {
  if (IsProcessorFeaturePresent95(PF_FASTFAIL_AVAILABLE)) {
    __fastfail(FAST_FAIL_RANGE_CHECK_FAILURE);
  } else {
    TerminateProcess(GetCurrentProcess(), STATUS_STACK_BUFFER_OVERRUN);
    __builtin_unreachable();
  }
}

#endif
