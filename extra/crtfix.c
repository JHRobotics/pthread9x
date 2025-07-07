#include <windows.h>
#include <stdlib.h>
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

#ifdef NEW_ALLOC

/* replace CRT with */

#define _EXIT_LOCK1     13      /* lock #1 for exit code            */

#define ONEXITTBLINCR   4

#ifndef _MT
#define _MT
#endif

typedef void (__cdecl *_PVFV)(void);
typedef int  (__cdecl *_PIFV)(void);

/* msvcrt imports */
void __cdecl _lock(int);
void __cdecl _unlock(int);

void _lockexit (void)
{
	_lock(_EXIT_LOCK1);
}

void _unlockexit(void)
{
	_unlock(_EXIT_LOCK1);
}

size_t _msize_int(void* ptr);

_onexit_t __dllonexit (_onexit_t func, _PVFV ** pbegin, _PVFV ** pend)
{
	_PVFV   *p;
	unsigned oldsize;

#ifdef _MT
	_lockexit();            /* lock the exit code */
#endif  /* _MT */

	/*
	 * First, make sure the table has room for a new entry
	 */
	if ( (oldsize = _msize_int( *pbegin )) <= (unsigned)((char *)(*pend) - (char *)(*pbegin)) )
        {
            /*
             * not enough room, try to grow the table
             */
            if ( (p = (_PVFV *) realloc((*pbegin), oldsize +
                ONEXITTBLINCR * sizeof(_PVFV))) == NULL )
            {
                /*
                 * didn't work. don't do anything rash, just fail
                 */
#ifdef _MT
                _unlockexit();
#endif  /* _MT */

                return NULL;
            }

            /*
             * update (*pend) and (*pbegin)
             */
            (*pend) = p + ((*pend) - (*pbegin));
            (*pbegin) = p;
        }

        /*
         * Put the new entry into the table and update the end-of-table
         * pointer.
         */
         *((*pend)++) = (_PVFV)func;

#ifdef _MT
        _unlockexit();
#endif  /* _MT */

        return func;

}

#if 0
/* no in use yet */
void *_aligned_malloc9x(size_t size, size_t alignment);
void *_aligned_realloc9x(void *memblock, size_t size, size_t alignment);
void _aligned_free9x(void *memblock);

__declspec(dllimport) void *_aligned_malloc(size_t size, size_t alignment)
{
	return _aligned_malloc9x(size, alignment);
}

__declspec(dllimport) void *_aligned_realloc(void *memblock, size_t size, size_t alignment)
{
	return _aligned_realloc9x(memblock, size, alignment);
}

__declspec(dllimport) void _aligned_free(void *memblock)
{
	_aligned_free9x(memblock);
}
#endif

#endif /* #ifdef NEW_ALLOC */

#ifndef NO_STATIC_TLS_REMOVE

BOOL WINAPI __dyn_tls_init (HANDLE hDllHandle, DWORD dwReason, LPVOID lpreserved)
{
  return TRUE;
}

const PIMAGE_TLS_CALLBACK __dyn_tls_init_callback = (const PIMAGE_TLS_CALLBACK) __dyn_tls_init;

int __mingw_initltsdrot_force = 0;
int __mingw_initltsdyn_force = 0;
int __mingw_initltssuo_force = 0;
int mingw_initltsdrot_force = 0;
int mingw_initltsdyn_force = 0;
int mingw_initltssuo_force = 0;

#endif

#include "int64.c"
#include "linkfix.c"
