#include <windows.h>
#include <io.h>

BOOL WINAPI IsProcessorFeaturePresent95(DWORD ProcessorFeature)
{
	return FALSE;
}

BOOL WINAPI IsProcessorFeaturePresent(DWORD ProcessorFeature)
{
	return IsProcessorFeaturePresent95(ProcessorFeature);
}

void __cdecl __attribute__((__noreturn__)) __chk_fail(void) {
  if (IsProcessorFeaturePresent95(PF_FASTFAIL_AVAILABLE)) {
    __fastfail(FAST_FAIL_RANGE_CHECK_FAILURE);
  } else {
    TerminateProcess(GetCurrentProcess(), STATUS_STACK_BUFFER_OVERRUN);
    __builtin_unreachable();
  }
}

