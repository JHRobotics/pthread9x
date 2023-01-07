
ifeq (,$(wildcard config.mk))
  ifeq (,$(wildcard ../pthread.mk))
    CONFIG=default.mk
  else
    CONFIG=../pthread.mk
  endif
else
  CONFIG=config.mk
endif

$(info Using config: $(CONFIG))

include $(CONFIG)

all: winpthreads_target
.PHONY: all clean winpthreads_target

DEPS = Makefile $(CONFIG)

ifdef MSC
  # this is only for mingw
  OBJ := .obj
else
  OBJ := .o
  LIBSUFFIX := .a
  LIBPREFIX := lib

  INCLUDE = -Iinclude -I.
  
  DEFS = -DHAVE_CONFIG_H -I. -DIN_WINPTHREAD -Wall -DWIN32_LEAN_AND_MEAN 

  ifdef NEW_ALLOC
    DEFS += -DNEW_ALLOC
  endif
  
  ifdef DEFAULT_HEAP
    DEFS += -DDEFAULT_HEAP
  endif
  
  ifdef MEM_ALIGN
    DEFS += -DMEM_ALIGN=$(MEM_ALIGN)
  endif
  
  ifdef MEM_COPY_SSE2
  	DEFS += -DMEM_COPY_SSE2
  endif
  
  ifdef CS_NATIVE_ONLY
    DEFS += -DCS_NATIVE_ONLY
  endif

  ifdef NO_STATIC_TLS_REMOVE
    DEFS += -DNO_STATIC_TLS_REMOVE
  endif

  ifdef SPEED
    CFLAGS = -std=gnu99 -O3 -fno-exceptions -ffast-math $(INCLUDE) -DNDEBUG $(DEFS)
    LDLAGS = -fno-exceptions
  else
    CFLAGS = -std=gnu99 -O0 -g -ffast-math $(INCLUDE) -DDEBUG -DWINPTHREAD_DBG=1 $(DEFS)
    LDLAGS = 
  endif

  %.c.o: %.c $(DEPS)
		$(CC) $(CFLAGS) -c -o $@ $<
	
  LIBSTATIC = ar rcs -o $@ 
endif

winpthreads_OBJS = \
  src/barrier.c$(OBJ) \
  src/cond.c$(OBJ) \
  src/misc.c$(OBJ) \
  src/mutex.c$(OBJ) \
  src/rwlock.c$(OBJ) \
  src/spinlock.c$(OBJ) \
  src/thread.c$(OBJ) \
  src/ref.c$(OBJ) \
  src/sem.c$(OBJ) \
  src/sched.c$(OBJ) \
  src/clock.c$(OBJ) \
  src/nanosleep.c$(OBJ) \
  src/tryentercriticalsection.c$(OBJ) \
  extra/memory.c$(OBJ) \
  extra/int64.c$(OBJ) \
  extra/lockex.c$(OBJ)


winpthreads_target: $(LIBPREFIX)pthread$(LIBSUFFIX) crtfix$(OBJ)

crtfix$(OBJ): extra/crtfix.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(LIBPREFIX)pthread$(LIBSUFFIX): $(winpthreads_OBJS)
	-$(RM) $@
	$(LIBSTATIC) $(winpthreads_OBJS)

ifdef OBJ
clean:
	-$(RM) $(LIBPREFIX)pthread$(LIBSUFFIX)
	-$(RM) $(winpthreads_OBJS)
	-$(RM) crtfix$(OBJ)
else
clean:

endif
