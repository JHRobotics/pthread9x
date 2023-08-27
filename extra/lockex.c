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

void crt_locks_init(int count)
{
	int i;
	int count_in = count+LOCK_INTERNAL_CNT;
	if(count_in > crt_locks_num)
	{
		//crt_locks = realloc(crt_locks, sizeof(CRITICAL_SECTION) * crt_locks_num);
		
		for(i = crt_locks_num; i < count_in; i++)
		{
			InitializeCriticalSection(crt_locks+i);
		}
		
		crt_locks_num = count_in;
	}
}

void crt_locks_destroy()
{
	int i;
	for(i = 0; i < crt_locks_num; i++)
	{
		DeleteCriticalSection(crt_locks+i);
	}
	
	free(crt_locks);
}

void crt_lock(int lock_no)
{
	int lock_no_in = lock_no + LOCK_INTERNAL_CNT;
	if(lock_no_in < crt_locks_num)
	{
		EnterCriticalSection(crt_locks+lock_no_in);
	}
}

void crt_unlock(int lock_no)
{
	int lock_no_in = lock_no + LOCK_INTERNAL_CNT;
	if(lock_no_in < crt_locks_num)
	{
		LeaveCriticalSection(crt_locks+lock_no_in);
	}
}
