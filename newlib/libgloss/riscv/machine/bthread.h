#ifndef GCC_GTHR_BTHREAD_H
#define GCC_GTHR_BTHREAD_H

#define __GTHREADS 1

#include <errno.h>

#define __BTHREAD_MUTEX_INIT { 0 }
#define __BTHREAD_ONCE_INIT  { __BTHREAD_MUTEX_INIT }
#define __BTHREAD_KEYS_MAX 128
#define __BTHREAD_THREADS_MAX 128

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
  unsigned int lock;
} __bthread_mutex_t;

typedef struct
{
  void (*dtor)(void*);
  __bthread_mutex_t busy;
} __bthread_key_data_t;

extern __bthread_key_data_t __bthread_keys[__BTHREAD_KEYS_MAX];
// should use TLS for this!
extern void* __bthread_key_data[__BTHREAD_THREADS_MAX][__BTHREAD_KEYS_MAX];

typedef unsigned int __bthread_t;

typedef struct
{
  unsigned int key;
} __bthread_key_t;

typedef struct {
  __bthread_mutex_t once;
} __bthread_once_t;


static inline __bthread_t __bthread_self(void)
{
  // in the future, we should use TLS for this.
  register __bthread_t __id asm("tp");
  return __id;
}

// returns true if there is more than 1 core in the system
static inline int __bthread_threading(void)
{
  return 1;
}

static inline int __bthread_mutex_init(__bthread_mutex_t* lock)
{
  lock->lock = 0;
  return 0;
}

static inline int __bthread_mutex_trylock(__bthread_mutex_t* lock)
{
  return __sync_lock_test_and_set(&lock->lock, 1);
}

static inline int __bthread_mutex_locked(__bthread_mutex_t* lock)
{
  return lock->lock;
}

static inline int __bthread_mutex_lock(__bthread_mutex_t* lock)
{
  do
  {
    while(__bthread_mutex_locked(lock));
  }
  while(__bthread_mutex_trylock(lock));

  return 0;
}

static inline int __bthread_mutex_unlock(__bthread_mutex_t* lock)
{
  lock->lock = 0;
  return 0;
}

static inline int
__bthread_once (__bthread_once_t *__once, void (*__func) (void))
{
  if(!__once || !__func)
    return EINVAL;

  if(__bthread_mutex_locked(&__once->once))
    return 0;

  if(__bthread_mutex_trylock(&__once->once))
    return 0;

  (*__func)();

  return 0;  
}

static inline int 
__bthread_key_create( __bthread_key_t* __key, void (*__dtor) (void*) )
{
  int i;
  for(i = 0; i < __BTHREAD_KEYS_MAX; i++)
  {
    if(!__bthread_mutex_locked(&__bthread_keys[i].busy))
      if(!__bthread_mutex_trylock(&__bthread_keys[i].busy))
        break;
  }

  if(i == __BTHREAD_KEYS_MAX)
    return ENOMEM;

  __bthread_keys[i].dtor = __dtor;
  __key->key = i;

  return 0;
}

static inline int
__bthread_key_valid( __bthread_key_t __key )
{
  if(__bthread_self() >= __BTHREAD_THREADS_MAX)
    return 0;

  if(__key.key >= __BTHREAD_KEYS_MAX)
    return 0;

  if(!__bthread_mutex_locked(&__bthread_keys[__key.key].busy))
    return 0;

  return 1;
}

static inline int
__bthread_key_delete( __bthread_key_t __key )
{
  if(!__bthread_key_valid(__key))
    return EINVAL;

  __bthread_mutex_unlock(&__bthread_keys[__key.key].busy);

  return 0;
}

static inline int
__bthread_setspecific( __bthread_key_t __key, void* __ptr )
{
  if(!__bthread_key_valid(__key))
    return EINVAL;

  __bthread_key_data[__bthread_self()][__key.key] = __ptr;

  return 0;
}

static inline void* 
__bthread_getspecific( __bthread_key_t __key )
{
  if(!__bthread_key_valid(__key))
    return 0;

  return (void*)__bthread_key_data[__bthread_self()][__key.key];
}

#ifdef __cplusplus
}
#endif

// bthread.h

#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef __bthread_key_t __gthread_key_t;
typedef __bthread_once_t __gthread_once_t;
typedef __bthread_mutex_t __gthread_mutex_t;

typedef struct {
  long depth;
  __bthread_t owner;
  __bthread_mutex_t actual;
} __gthread_recursive_mutex_t;

#define __GTHREAD_MUTEX_INIT __BTHREAD_MUTEX_INIT
#define __GTHREAD_ONCE_INIT __BTHREAD_ONCE_INIT

static inline int
__gthread_recursive_mutex_init_function(__gthread_recursive_mutex_t *__mutex);
#define __GTHREAD_RECURSIVE_MUTEX_INIT_FUNCTION __gthread_recursive_mutex_init_function

#define __gthrw(name)
#define __gthrw_(name) name

__gthrw(__bthread_once)
__gthrw(__bthread_key_create)
__gthrw(__bthread_key_delete)
__gthrw(__bthread_getspecific)
__gthrw(__bthread_setspecific)

__gthrw(__bthread_self)

__gthrw(__bthread_mutex_lock)
__gthrw(__bthread_mutex_trylock)
__gthrw(__bthread_mutex_unlock)

__gthrw(__bthread_threading)

static inline int
__gthread_active_p (void)
{
  return (__bthread_threading)();
}

static inline int
__gthread_once (__gthread_once_t *__once, void (*__func) (void))
{
  if (__gthread_active_p ())
    return __gthrw_(__bthread_once) (__once, __func);
  else
    return -1;
}

static inline int
__gthread_key_create (__gthread_key_t *__key, void (*__dtor) (void *))
{
  return __gthrw_(__bthread_key_create) (__key, __dtor);
}

static inline int
__gthread_key_delete (__gthread_key_t __key)
{
  return __gthrw_(__bthread_key_delete) (__key);
}

static inline void *
__gthread_getspecific (__gthread_key_t __key)
{
  return __gthrw_(__bthread_getspecific) (__key);
}

static inline int
__gthread_setspecific (__gthread_key_t __key, void *__ptr)
{
  return __gthrw_(__bthread_setspecific) (__key, __ptr);
}

static inline int
__gthread_mutex_destroy (__gthread_mutex_t *__mutex)
{
  return __mutex == 0 ? EINVAL : 0;
}

static inline int
__gthread_mutex_lock (__gthread_mutex_t *__mutex)
{
  if (__gthread_active_p ())
    return __gthrw_(__bthread_mutex_lock) (__mutex);
  else
    return 0;
}

static inline int
__gthread_mutex_trylock (__gthread_mutex_t *__mutex)
{
  if (__gthread_active_p ())
    return __gthrw_(__bthread_mutex_trylock) (__mutex);
  else
    return 0;
}

static inline int
__gthread_mutex_unlock (__gthread_mutex_t *__mutex)
{
  if (__gthread_active_p ())
    return __gthrw_(__bthread_mutex_unlock) (__mutex);
  else
    return 0;
}

static inline int
__gthread_recursive_mutex_init_function (__gthread_recursive_mutex_t *__mutex)
{
  __mutex->depth = 0;
  __mutex->owner = __gthrw_(__bthread_self) ();
  __bthread_mutex_init(&__mutex->actual);
  return 0;
}

static inline int
__gthread_recursive_mutex_lock (__gthread_recursive_mutex_t *__mutex)
{
  if (__gthread_active_p ())
    {
      __bthread_t __me = __gthrw_(__bthread_self) ();

      if (__mutex->owner != __me)
	{
	  __gthrw_(__bthread_mutex_lock) (&__mutex->actual);
	  __mutex->owner = __me;
	}

      __mutex->depth++;
    }
  return 0;
}

static inline int
__gthread_recursive_mutex_trylock (__gthread_recursive_mutex_t *__mutex)
{
  if (__gthread_active_p ())
    {
      __bthread_t __me = __gthrw_(__bthread_self) ();

      if (__mutex->owner != __me)
	{
	  if (__gthrw_(__bthread_mutex_trylock) (&__mutex->actual))
	    return 1;
	  __mutex->owner = __me;
	}

      __mutex->depth++;
    }
  return 0;
}

// todo: add error checking - what to do if this is called when
// we aren't the owner of the lock?
static inline int
__gthread_recursive_mutex_unlock (__gthread_recursive_mutex_t *__mutex)
{
  if (__gthread_active_p ())
    {
      if (--__mutex->depth == 0)
	  {
	   __mutex->owner = (__bthread_t) 0;
	   __gthrw_(__bthread_mutex_unlock) (&__mutex->actual);
	  }
    }
  return 0;
}

#ifdef __cplusplus
}
#endif

#endif
