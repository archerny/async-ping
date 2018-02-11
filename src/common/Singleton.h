#ifndef _COMMON_SINGLETON_H_
#define _COMMON_SINGLETON_H_

#include <pthread.h>
#include <stdlib.h>

template <class T>
class Singleton
{
  public:
    static T &getInstance()
    {
        if (!value)
        {
            pthread_mutex_lock(&mutex);
            if (!value)
            {
                T* p = static_cast<T*>(operator new(sizeof(T)));
                new (p) T();
                // mem barrier
                __sync_synchronize();
                value = p;
            }
            pthread_mutex_unlock(&mutex);
        }
        return *value;
    }

  private:
    Singleton();
    ~Singleton();

    static T *value;
    static pthread_mutex_t mutex;
};

template <class T>
T *Singleton<T>::value = NULL;

template <class T>
pthread_mutex_t Singleton<T>::mutex = PTHREAD_MUTEX_INITIALIZER;

#endif