#include <windows.h>
#include <stdio.h> 
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <io.h>

BOOL WINAPI TryEnterCriticalSection9x(CRITICAL_SECTION* cs);

__declspec(dllimport) BOOL WINAPI TryEnterCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
	return TryEnterCriticalSection9x(lpCriticalSection);
}

static unsigned int crt_locks_num = 0;
CRITICAL_SECTION *crt_locks = NULL;

void crt_locks_init(int count)
{
	int i;
	if(count > crt_locks_num)
	{
		crt_locks = realloc(crt_locks, sizeof(CRITICAL_SECTION) * crt_locks_num);
		
		for(i = crt_locks_num; i < count; i++)
		{
			InitializeCriticalSection(crt_locks+i);
		}
		
		crt_locks_num = count;
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
	if(lock_no < crt_locks_num)
	{
		EnterCriticalSection(crt_locks+lock_no);
	}
}

void crt_unlock(int lock_no)
{
	if(lock_no < crt_locks_num)
	{
		LeaveCriticalSection(crt_locks+lock_no);
	}
}
