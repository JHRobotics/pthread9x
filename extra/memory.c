#include <windows.h>
#include <stdio.h> 
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>

#include <cpuid.h>

// include SSE2 intrinsics require some target switching
#ifdef MEM_COPY_SSE2
#pragma GCC push_options
#pragma GCC target ("arch=core2")
#include <emmintrin.h>
// restore the target selection
#pragma GCC pop_options
#endif

#ifndef MEM_ALIGN
#define MEM_ALIGN 16
#endif

#if MEM_ALIGN != 4 && MEM_ALIGN != 8 && MEM_ALIGN != 16 && MEM_ALIGN != 32 && MEM_ALIGN != 64 && MEM_ALIGN != 128
#error "Wrong memory aligment!"
#endif

#define MEM_R1 64
#define MEM_R2 1024
#define MEM_R3 8*1024
#define MEM_R4 64*1024

typedef void(*memcpy_func)(void *dst, void *src, size_t size);
typedef void(*zeromem_func)(void *dst, size_t size);

#define AROUND(_s, _a) _s = ((_s + _a - 1) & (~(size_t)(_a - 1)))

#ifndef DEFAULT_HEAP
static HANDLE glHeaps[4] = {NULL, NULL, NULL, NULL};

static HANDLE NewHeap()
{
	SYSTEM_INFO si;
	GetSystemInfo(&si);
			
	return HeapCreate(0, si.dwPageSize*32, 0);
}
#endif

typedef struct _memblk_t
{
	HANDLE heap;
	uint8_t *heap_ptr;
	size_t mem_size;
} memblk_t;

static HANDLE GetHeap(size_t memsize)
{
#ifndef DEFAULT_HEAP
	int sel = 3;
	
	if(memsize < MEM_R2)
	{
		sel = 0;
	}
	else if(memsize < MEM_R3)
	{
		sel = 1;
	}
	else if(memsize < MEM_R4)
	{
		sel = 2;
	}
	
	if(glHeaps[sel] == NULL)
	{
		glHeaps[sel] = NewHeap();
	}
	
	return glHeaps[sel];
#else
 	return GetProcessHeap();
#endif
}

static void memcpy_c(void *dst, void *src, size_t size)
{
	size_t bsize = size/4;
	
	uint32_t *psrc = src;
	uint32_t *pdst = dst;
	
	for (; bsize > 0; bsize--, psrc++, pdst++)
	{
		*pdst = *psrc;
	}
}

static void zeromem_c(void *dst, size_t size)
{
	size_t bsize = size/4;
	
	uint32_t *pdst = dst;
	
	for (; bsize > 0; bsize--, pdst++)
	{
		*pdst = 0;
	}
}

static memcpy_func memcpy_fast = memcpy_c;
static zeromem_func zeromem_fast = zeromem_c;

#ifdef MEM_COPY_SSE2

#pragma GCC push_options
#pragma GCC target ("arch=core2")

static void memcpy_sse2(void *dst, void *src, size_t size)
{
	size_t bsize = size/16;
	
	__m128i *psrc = (__m128i*)src;
	__m128i *pdst = (__m128i*)dst;
	
	for (; bsize > 0; bsize--, psrc++, pdst++)
	{
		const __m128i t = _mm_load_si128(psrc);
		_mm_store_si128(pdst, t);
		//pdst* = *psrc;
	}
}

static void zeromem_sse2(void *dst, size_t size)
{
	size_t bsize = size/16;
	__m128i t = _mm_setzero_si128();
	__m128i *pdst = (__m128i*)dst;
	
	for (; bsize > 0; bsize--, pdst++)
	{
		_mm_store_si128(pdst, t);
	}
}

#pragma GCC pop_options
#endif

void crt_enable_sse2()
{
#ifdef MEM_COPY_SSE2
# if MEM_ALIGN >= 16
	memcpy_fast = memcpy_sse2;
	zeromem_fast = zeromem_sse2;
# endif
#endif
}

typedef DWORD (WINAPI *GetVersionFunc)(void);

static int windows_sse_support()
{
	HANDLE h = GetModuleHandleA("kernel32.dll");
	if(h)
	{
		GetVersionFunc GetVersionPtr = (GetVersionFunc)GetProcAddress(h, "GetVersion");
    DWORD dwVersion = 0; 
    DWORD dwMajorVersion = 0;
    DWORD dwMinorVersion = 0; 
		
		if(GetVersionPtr == NULL)
		{
			/* windows >= 8.1 don't have this function */
			return 3;
		}
		
		dwVersion = GetVersionPtr();
		
		dwMajorVersion = (DWORD)(LOBYTE(LOWORD(dwVersion)));
		dwMinorVersion = (DWORD)(HIBYTE(LOWORD(dwVersion)));
    // 95     - 4.0
    // NT 4.0 - 4.0
    // 98     - 4.10
    // ME     - 4.90
    // 2k     - 5.0
    //printf("Version is %d.%d (%d)\n", dwMajorVersion, dwMinorVersion, dwBuild);
		
		if(dwMajorVersion > 5)
		{
			return 1;
		}
		
		if(dwMajorVersion == 4)
		{
			if(dwMinorVersion >= 10)
			{
				return 1;
			}
		}
			
		return 0;
	}
	
	return 0;
}

int crt_sse2_is_safe()
{
	if(windows_sse_support() != 0)
	{
		uint32_t ceax, cebx, cecx, cedx;
		
		if(__get_cpuid (1, &ceax, &cebx, &cecx, &cedx))
		{
			if(cedx & (1 << 26)) // SSE2 support bit
			{
				return 1;
			}
		}
	}
	
	return 0;
}

static inline void *malloc_int(size_t size)
{
	size_t hsize;
	uint8_t *hptr;
	memblk_t *mem;
	uintptr_t p;
	HANDLE heap;
	
	hsize = size + sizeof(memblk_t) + MEM_ALIGN - 1;
	
	heap = GetHeap(size);
	
	hptr = HeapAlloc(heap, 0, hsize);
	if(hptr != NULL)
	{
		p = (uintptr_t)hptr;
		p += MEM_ALIGN - 1;
		p += sizeof(memblk_t);
		p &= ~((uintptr_t)(MEM_ALIGN - 1));
		
		mem = ((memblk_t*)p)-1;
		mem->heap = heap;
		mem->heap_ptr = hptr;
		mem->mem_size = size;
		
		return mem+1;
	}
	
	return NULL;
}

#ifdef NEW_ALLOC

void *malloc(size_t size)
{
	if(size == 0)
	{
		size = MEM_ALIGN;
	}
	AROUND(size, MEM_ALIGN);
		
	return malloc_int(size);
}

void *realloc(void *ptr, size_t new_size)
{
	memblk_t *mem;
	size_t hsize;
	void *tmp;
	
	if(new_size == 0 && ptr == NULL)
	{
		new_size = MEM_ALIGN;
		AROUND(new_size, MEM_ALIGN);
		return malloc_int(new_size);
	}
	else if(new_size == 0)
	{
		free(ptr);
		return NULL;
	}
	else if(ptr == NULL)
	{
		AROUND(new_size, MEM_ALIGN);
		return malloc_int(new_size);
	}
	
	AROUND(new_size, MEM_ALIGN);
	
	mem = ((memblk_t*)ptr)-1;
	if(new_size == mem->mem_size)
	{
		return ptr;
	}
	
	hsize = (uint8_t*)ptr - mem->heap_ptr + new_size;
	tmp = HeapReAlloc(mem->heap, HEAP_REALLOC_IN_PLACE_ONLY, mem->heap_ptr, hsize);
	
	if(tmp != NULL)
	{
		mem->mem_size = new_size;
		return ptr;
	}
	else
	{
		tmp = malloc_int(new_size);
		if(tmp)
		{
			memcpy_fast(tmp, ptr, mem->mem_size);
			free(ptr);
			
			return tmp;
		}
	}
	
	return NULL;
}

void *calloc(size_t num, size_t size)
{
	size_t total = num*size;
	void *ptr;
	if(total == 0)
	{
		size = MEM_ALIGN;
	}
	AROUND(total, MEM_ALIGN);
	
	ptr = malloc_int(total);
  if(ptr != NULL)
  {
  	zeromem_fast(ptr, total);
  }
  
  return ptr;
}

void free(void *ptr)
{
	if(ptr != NULL)
	{
		memblk_t *mem = ((memblk_t*)ptr)-1;
		
		HeapFree(mem->heap, 0, mem->heap_ptr);
	}
}

__declspec(dllimport) void* _expand(void* ptr, size_t new_size)
{	
	if(ptr != NULL)
	{
		size_t hsize;
		void *tmp;
		memblk_t *mem = ((memblk_t*)ptr)-1;
	
		hsize = (uint8_t*)ptr - mem->heap_ptr + new_size;
		
		tmp = HeapReAlloc(mem->heap, HEAP_REALLOC_IN_PLACE_ONLY, mem->heap_ptr, hsize);
		
		if(tmp)
		{
			mem->mem_size = new_size;
			return ptr;
		}
	}
	
	return NULL;
	//return HeapReAlloc(GetHeap(), HEAP_REALLOC_IN_PLACE_ONLY, ptr, new_size);
}

size_t _msize_int(void* ptr)
{	
	if(ptr != NULL)
	{
		//return HeapSize(GetHeap(), 0, ptr);
		memblk_t *mem = (memblk_t*)ptr;
		mem--;
		
		return mem->mem_size;
	}
	
	return 0;
}

__declspec(dllimport) size_t _msize(void* ptr)
{
	return _msize_int(ptr);
}

char *strdup(const char *str1)
{
	size_t len = strlen(str1);
	char *str2 = malloc(len+1);
	if(str2 != NULL)
	{
		memcpy(str2, str1, len+1);
	}
	
	return str2;
}

__declspec(dllimport) char *_strdup(const char *str1)
{
	size_t len = strlen(str1);
	char *str2 = malloc(len+1);
	if(str2 != NULL)
	{
		memcpy(str2, str1, len+1);
	}
	
	return str2;
}

char *strndup(const char *str1, size_t size)
{
	size_t len = strlen(str1);
	
	if(size == 0)
	{
		return NULL;
	}
	
	if(len > size-1)
	{
		len = size-1;
	}
	
	char *str2 = malloc(len+1);
	if(str2 != NULL)
	{
		memcpy(str2, str1, len+1);
	}
	
	str2[len] = '\0';
	
	return str2;
}

#endif /* NEW_ALLOC */
