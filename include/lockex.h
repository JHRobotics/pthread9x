#ifndef __9X__LOCKEX_H__INCLUDED__
#define __9X__LOCKEX_H__INCLUDED__

#define LOCK_INTERNAL_MEM -1
#define LOCK_INTERNAL_CNT 1

#define LOCK_TOTAL_MAX 32

void crt_locks_init(int count);
void crt_locks_destroy();
void crt_lock(int lock_no);
void crt_unlock(int lock_no);

#endif /* __9X__LOCKEX_H__INCLUDED__ */
