# compiler names
CC  = $(PREFIX)gcc
LD  = $(PREFIX)gcc

# optimize for speed
SPEED=1

# replace default MSVCRT.dll default malloc/calloc/realloc/free
# other related functions
NEW_ALLOC=1

# use program default heap instead of creating new ones
DEFAULT_HEAP=1

# allign malloced return memory to specific allign (must be power of 2)
# 8 is highly recomended, 16 you can use directly with SSE, 32 for AVX
#MEM_ALIGN=16

# allow enable faster realloc copy, or calloc zeroing accelerated
# by SSE, if your OS/HW allows it, enable in runtime by crt_enable_sse2()
# function.
# TIP: You can call crt_sse2_is_safe() and check if SSE2 is safe
#MEM_COPY_SSE2=1

# disable Windows 98/Me code for TryEnterCriticalSection and call TryEnterCritical instead
#CS_NATIVE_ONLY=1

# disable static TLS remove if it break something important
#NO_STATIC_TLS_REMOVE=1

# Pass exta CFLAGS, please don't use -march= for old mingw, SSE runtime
# is broken here!
#TUNE=-march=core2
