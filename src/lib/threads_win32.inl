
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

u64
atomic_add64(volatile u64 *value, u64 addend) {
	u64 result = InterlockedExchangeAdd64((volatile long long *)value, addend);
	return result;
}

Thread
create_thread(ThreadProc *proc, void *param) {
	Thread result = {0};
	
	DWORD thread_id;
	HANDLE thread_handle = CreateThread(0, 0, proc, param, 0, &thread_id);
	CloseHandle(thread_handle);
	
	result.id = thread_id;
	
	return result;
}

void
exit_thread(void) {
    ExitThread(0);
}

u32 
get_core_count(void) {
	SYSTEM_INFO info;
	GetSystemInfo(&info);
	return info.dwNumberOfProcessors;
}

u32 
get_thread_id(void) {
	// @NOTE this is basically GetThreadID function disassembly made with intrinsics
	// so we achieve the same stuff without system calls
	u8 *tls = (u8 *)__readgsqword(0x30);
	return *(u32 *)(tls + 0x48);
}
