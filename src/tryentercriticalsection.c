/*
 *  KernelEx
 *  Copyright (C) 2008, Xeno86
 *
 *  This file is part of KernelEx source code.
 *
 *  KernelEx is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published
 *  by the Free Software Foundation; version 2 of the License.
 *
 *  KernelEx is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <windows.h>

#define K32OBJ_CRITICAL_SECTION		4

typedef struct _CRIT_SECT     // Size = 0x20 
{
	BYTE      Type;           // 00 = 4: K32_OBJECT_CRITICAL_SECTION
	int       RecursionCount; // 04 initially 0, incremented on lock
	void*     OwningThread;   // 08 pointer to TDBX
	DWORD     un3;            // 0C
	int       LockCount;      // 10 initially 1, decremented on lock
	struct _CRIT_SECT* Next;  // 14
	void*     PLst;           // 18 list of processes using it?
	struct _WIN_CRITICAL_SECTION* UserCS; // 1C pointer to user defined CRITICAL_SECTION
} CRIT_SECT, *PCRIT_SECT;

typedef struct _WIN_CRITICAL_SECTION
{
	BYTE Type; //= 4: K32_OBJECT_CRITICAL_SECTION
	PCRIT_SECT crit;
	DWORD un1;
	DWORD un2;
	DWORD un3;
	DWORD un4;
} WIN_CRITICAL_SECTION, *PWIN_CRITICAL_SECTION;

//static DWORD _offset;
DWORD tesc_offset;

BOOL init_tryentercritsec()
{
	DWORD GV = GetVersion();
	
	if (GV == 0xc0000a04) //98
	{
		tesc_offset = 0x58 - 0x8;
		return TRUE;
	}
	if (GV == 0xc0005a04) //Me
	{
		tesc_offset = 0x88 - 0x8;
		return TRUE;
	}
	// FIXME: 95
	return FALSE;
}

#ifdef __GNUC__
# ifdef __i386__

BOOL WINAPI TryEnterCrst(CRIT_SECT* crit);

__asm__(".text\n\t"
		".align 4\n\t"
		".globl _TryEnterCrst@4\n\t"
		".def _TryEnterCrst@4; .scl 2; .type 32; .endef\n"
		"_TryEnterCrst@4:\n\t"
		"movl 4(%esp),%edx\n\t"
		"xorl %eax,%eax\n\t"
		"incl %eax\n\t"
		"xorl %ecx,%ecx\n\t"
		"cmpxchgl %ecx,0x10(%edx)\n\t" /* if (OP1==eax) { OP1=OP2; ZF=1; } else { eax=OP1; ZF=0 } */
		"movl %fs:0x18, %ecx\n\t"
		"addl _tesc_offset,%ecx\n\t"
		"movl (%ecx),%ecx\n\t" /* ecx will contain TDBX now */
		"cmpl $1,%eax\n\t"
		"jnz .L1\n\t"
		/* critical section was unowned => successful lock */
		"movl %ecx,8(%edx)\n\t"
		"incl 4(%edx)\n\t"
		"ret $4\n\t"
".L1: \n\t"
		"cmpl %ecx,8(%edx)\n\t"
		"jnz .L2\n\t"
		/* critical section owned by this thread */
		"decl 0x10(%edx)\n\t"
		"incl 4(%edx)\n\t"
		"xorl %eax,%eax\n\t"
		"incl %eax\n\t"
		"ret $4\n\t"
".L2: \n\t"
		/* critical section owned by other thread - do nothing */
		"xorl %eax,%eax\n\t"
		"ret $4\n\t"
		);
# else
#  define CS_NATIVE_ONLY
# endif
#elif !(defined(_WIN64) || defined(_M_ARM))

__declspec(naked) BOOL WINAPI TryEnterCrst(CRIT_SECT* crit)
{
__asm {
	mov edx, [esp+4]
	xor eax, eax
	inc eax
	xor ecx, ecx
	cmpxchg [edx+10h], ecx ;if (OP1==eax) { OP1=OP2; ZF=1; } else { eax=OP1; ZF=0 }
	;mov ecx, ppTDBXCur
	mov ecx, fs:[18h]
	add ecx, [tesc_offset]
	mov ecx, [ecx] ;ecx will contain TDBX now
	cmp eax, 1
	jnz L1
	;critical section was unowned => successful lock
	mov [edx+8], ecx
	inc dword ptr [edx+4]
	ret 4
L1:
	cmp [edx+8], ecx
	jnz L2
	;critical section owned by this thread
	dec dword ptr [edx+10h]
	inc dword ptr [edx+4]
	xor eax, eax
	inc eax
	ret 4
L2:
	;critical section owned by other thread - do nothing
	xor eax, eax
	ret 4
	}
}

#endif

typedef BOOL (WINAPI *tecs_f)(CRITICAL_SECTION* cs);

static tecs_f tecs_p = NULL;

BOOL WINAPI TryEnterCriticalSectionNative(CRITICAL_SECTION* cs)
{
#if !(defined(_WIN64) || defined(_M_ARM))
	DWORD GV = GetVersion();
	DWORD major = (DWORD)(LOBYTE(LOWORD(GV)));
	DWORD minor = (DWORD)(HIBYTE(LOWORD(GV)));
	DWORD isNT = (int) GV >= 0;
	
	if(!isNT || major < 4)
	{
#ifndef CS_NATIVE_ONLY
		if(minor == 10 || minor == 90) /* 98 + Me*/
		{
			WIN_CRITICAL_SECTION* mycs = (WIN_CRITICAL_SECTION*) cs;
			if (mycs->Type != K32OBJ_CRITICAL_SECTION)
			{
				RaiseException(STATUS_ACCESS_VIOLATION, 0, 0, NULL);
			}
			
			return TryEnterCrst(mycs->crit);
		}
		else // FIXME: 95, NT3, Win32s
#endif
		{
			EnterCriticalSection(cs);
			return TRUE;
		}
	}
	else
	{
		if(tecs_p == NULL)
		{
			HMODULE hK = GetModuleHandleA("kernel32.dll");
			if(hK)
			{
				tecs_p = (tecs_f)GetProcAddress(hK, "TryEnterCriticalSection"); // Note: 98 & ME exports this as a stub
			}
		}
		
		if(tecs_p != NULL)
		{
			return tecs_p(cs);
		}
	}
	
	RaiseException(STATUS_ACCESS_VIOLATION, 0, 0, NULL);
	
	return FALSE;
#else
	return TryEnterCriticalSection(cs);
#endif
}

/* MAKE_EXPORT TryEnterCriticalSection_new=TryEnterCriticalSection */
BOOL WINAPI TryEnterCriticalSection9x(CRITICAL_SECTION* cs)
{
#if 0
	WIN_CRITICAL_SECTION* mycs = (WIN_CRITICAL_SECTION*) cs;
	if (mycs->Type != K32OBJ_CRITICAL_SECTION)
	{
		//RaiseException(STATUS_ACCESS_VIOLATION, 0, 0, NULL);
		return TryEnterCriticalSectionNative(cs);
	}
	
	return TryEnterCrst(mycs->crit);
#else
	return TryEnterCriticalSectionNative(cs);
#endif
}
