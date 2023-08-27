#include <windows.h>
#include <stdio.h> 
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <io.h>
#include <lockex.h>

BOOL WINAPI TryEnterCriticalSection9x(CRITICAL_SECTION* cs);

__declspec(dllimport) BOOL WINAPI TryEnterCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
	return TryEnterCriticalSection9x(lpCriticalSection);
}

static unsigned int crt_locks_num = 0;
static CRITICAL_SECTION crt_locks[LOCK_TOTAL_MAX];
static int crt_locks_cnt[LOCK_TOTAL_MAX] = {0}; /* number of 'init' of individual locks */

void crt_locks_init(int count)
{
	int i;
	int count_in = count+LOCK_INTERNAL_CNT;
	
	if(count_in > LOCK_TOTAL_MAX)
	{
		count_in = LOCK_TOTAL_MAX;
	}

	for(i = 0; i < count_in; i++)
	{
		if(crt_locks_cnt[i] <= 0)
		{
			crt_locks_cnt[i] = 0;
			InitializeCriticalSection(&crt_locks[i]);
		}
		
		crt_locks_cnt[i]++;
	}
	
	if(count_in > crt_locks_num)
	{
		crt_locks_num = count_in;
	}
}

void crt_locks_destroy()
{
	int i;
	for(i = 0; i < crt_locks_num; i++)
	{
		if(--crt_locks_cnt[i] <= 0)
		{
			DeleteCriticalSection(&crt_locks[i]);
		}
	}
}

void crt_lock(int lock_no)
{
	int lock_no_in = lock_no + LOCK_INTERNAL_CNT;
	if(lock_no_in < crt_locks_num)
	{
		EnterCriticalSection(&crt_locks[lock_no_in]);
	}
}

void crt_unlock(int lock_no)
{
	int lock_no_in = lock_no + LOCK_INTERNAL_CNT;
	if(lock_no_in < crt_locks_num)
	{
		LeaveCriticalSection(&crt_locks[lock_no_in]);
	}
}
