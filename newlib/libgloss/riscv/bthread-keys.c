#include <machine/bthread.h>

__bthread_key_data_t __bthread_keys[__BTHREAD_KEYS_MAX];
void* __bthread_key_data[__BTHREAD_THREADS_MAX][__BTHREAD_KEYS_MAX];
