
#include <unistd.h>
#include <pthread.h>

u64
atomic_add64(volatile u64 *value, u64 addend) {
	u64 result = __sync_fetch_and_add(value, addend);
	return result;
}

Thread
create_thread(ThreadProc *proc, void *param) {
	Thread result = {0};

    CT_ASSERT(sizeof(result) >= sizeof(pthread_t));
    bool success = pthread_create((pthread_t *)&result, 0, proc, param);
	assert(!success);

	return result;
}

void
exit_thread(void) {
    pthread_exit(0);
}

u32 
get_core_count(void) {
    return sysconf(_SC_NPROCESSORS_ONLN);
}
