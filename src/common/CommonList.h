#ifndef _COMMON_LIST_H_
#define _COMMON_LIST_H_

#include <pthread.h>

namespace common
{

template <class T>
struct ListNode
{
  T data;
  ListNode *next;
};

/**
 * simple list, not thread safe
 * push and pop from head
 */
template <class T>
class List
{
public:
  List();
  ~List();

  bool pushHead(const T &node);

  bool popHead(T &ret);

  inline bool empty() { return head->next == NULL; }

private:
  // head is sentinel node
  ListNode<T> *head;
};

template <class T>
struct DequeueNode
{
  T data;
  DequeueNode *next;
  DequeueNode *prev;
};

/**
 * thread safe dequeue, need two versions
 * 1. spin lock version
 * 2. mutex version
 */
template <class T>
class ConcurrentDequeue
{
public:
  ConcurrentDequeue();
  ~ConcurrentDequeue();

public:
  bool tryPushHead(const T &data);
  bool tryPushHead(DequeueNode<T> *node);
  bool pushHead(const T &data);
  bool pushHead(DequeueNode<T> *node);
  bool tryPopTail(T &ret);
  bool popTail(T &ret);

private:
  // head and tail is sentinel node
  DequeueNode<T> *head;
  DequeueNode<T> *tail;
  pthread_spinlock_t spinlock;
};

template <class T>
class simple_dequeue
{
};

// implementations
template <class T>
List<T>::List()
{
  head = new ListNode<T>();
  head->next = NULL;
}

template <class T>
List<T>::~List()
{
  while (head->next)
  {
    ListNode<T> *tmp = head;
    head = head->next;
    delete tmp;
  }
  delete head;
}

template <class T>
bool List<T>::pushHead(const T &data)
{
  ListNode<T> *node = new ListNode<T>();
  node->data = data;

  node->next = head->next;
  head->next = node;
  return true;
}

template <class T>
bool List<T>::popHead(T &ret)
{
  ListNode<T> *tmp = head->next;
  if (tmp)
  {
    ret = tmp->data;
    head->next = tmp->next;
    delete tmp;
    return true;
  }
  else
  {
    return false;
  }
}

template <class T>
ConcurrentDequeue<T>::ConcurrentDequeue()
{
  pthread_spin_init(&spinlock, 0);
  head = new DequeueNode<T>();
  tail = new DequeueNode<T>();
  head->prev = NULL;
  head->next = tail;
  tail->prev = head;
  tail->next = NULL;
}

template <class T>
ConcurrentDequeue<T>::~ConcurrentDequeue()
{
  int status = pthread_spin_lock(&spinlock);
  if (status != 0)
  {
    // log, unrecoverable
    // EDEADLK: the calling thread already has lock
  }
  // delete all node from head
  while (head->next)
  {
    DequeueNode<T> *tmp = head;
    head = head->next;
    head->prev = NULL;
    delete tmp;
  }
  delete head;
  pthread_spin_unlock(&spinlock);
  pthread_spin_destroy(&spinlock);
}

template <class T>
bool ConcurrentDequeue<T>::tryPushHead(const T &data)
{
  DequeueNode<T> *node = new DequeueNode<T>();
  node->data = data;
  int status = pthread_spin_trylock(&spinlock);
  if (status)
  {
    // lock fail, release mem
    delete node;
    return false;
  }
  else
  {
    node->next = head->next;
    head->next = node;
    node->next->prev = node;
    node->prev = head;
    pthread_spin_unlock(&spinlock);
    return true;
  }
}

template <class T>
bool ConcurrentDequeue<T>::tryPushHead(DequeueNode<T> *node)
{
  int status = pthread_spin_trylock(&spinlock);
  if (status)
  {
    return false;
  }
  else
  {
    node->next = head->next;
    head->next = node;
    node->next->prev = node;
    node->prev = head;
    pthread_spin_unlock(&spinlock);
    return true;
  }
}

template <class T>
bool ConcurrentDequeue<T>::pushHead(const T &data)
{
  DequeueNode<T> *node = new DequeueNode<T>();
  node->data = data;
  int status = pthread_spin_lock(&spinlock);
  if (status)
  {
    // lock fail, release mem
    delete node;
    return false;
  }
  else
  {
    node->next = head->next;
    head->next = node;
    node->next->prev = node;
    node->prev = head;
    pthread_spin_unlock(&spinlock);
    return true;
  }
}

template <class T>
bool ConcurrentDequeue<T>::pushHead(DequeueNode<T> *node)
{
  int status = pthread_spin_lock(&spinlock);
  if (status)
  {
    // log
    return false;
  }
  else
  {
    node->next = head->next;
    head->next = node;
    node->next->prev = node;
    node->prev = head;
    pthread_spin_unlock(&spinlock);
    return true;
  }
}

template <class T>
bool ConcurrentDequeue<T>::tryPopTail(T &ret)
{
  int status = pthread_spin_trylock(&spinlock);
  if (status)
  {
    return false;
  }
  else
  {
    if (tail->prev == head)
    {
      pthread_spin_unlock(&spinlock);
      return false; // empty queue
    }
    DequeueNode<T> *node = tail->prev;
    tail->prev = node->prev;
    tail->prev->next = tail;
    pthread_spin_unlock(&spinlock);

    ret = node->data;
    return true;
  }
}

template <class T>
bool ConcurrentDequeue<T>::popTail(T &ret)
{
  int status = pthread_spin_lock(&spinlock);
  if (status)
  {
    return false;
  }
  else
  {
    if (tail->prev == head)
    {
      pthread_spin_unlock(&spinlock);
      return false; // empty queue
    }
    DequeueNode<T> *node = tail->prev;
    tail->prev = node->prev;
    tail->prev->next = tail;
    pthread_spin_unlock(&spinlock);

    ret = node->data;
    delete node;
    return true;
  }
}
}

#endif
