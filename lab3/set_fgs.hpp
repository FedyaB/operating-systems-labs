#ifndef SET_FG_EOQWOO__
#define SET_FG_EOQWOO__

#include <functional>
#include <pthread.h>
#include <bits/stdc++.h>

#include "set.h"

//Fine-grained synchronization set
template <class T>
class SetFGS : public Set<T>
{
public:
  SetFGS()
  {
    if (!error_handler) //Error handler needs to be specified before
      error_handler(g_msg_err_error_handler);
    //Initialize head of the list and it's next element
    head = new Node(0);
    if (!head)
      error_handler(g_msg_err_node_create);
    head->next = new Node(INT_MAX);
    if (!head->next)
      error_handler(g_msg_err_node_create);
  }

  ~SetFGS()
  {
    //Delete all elements from the list
    for (Node *current = head, *next = nullptr; current != nullptr; current = next)
    {
      next = current->next;
      delete current;
    }
  }

  bool add(const T& item)
  {
    //Generate hash for a provided item
    size_t key = generate_hash(item);
    //Update _current and _previous so we're in the key position
    head->lock();
    Node* _previous = head;
    Node* _current = head->next;
    _current->lock();
    while (_current->key < key)
    {
      _previous->unlock();
      _previous = _current;
      _current = _current->next;
      _current->lock();
    }
    //If the element is in list then do nothing 
    if (_current->key == key)
    {
      _current->unlock();
      _previous->unlock();
      return false;
    }

    //Insert new element between _previous and _current
    Node* to_insert = new Node(item);
    if (!to_insert)
      error_handler(g_msg_err_error_handler);
    to_insert->next = _current;
    _previous->next = to_insert;
    _current->unlock();
    _previous->unlock();
    return true;
  }

  bool remove(const T& item)
  {
    //Generate hash for a provided item
    size_t key = generate_hash(item);
    //Update _current and _previous so we're in the key position
    head->lock();
    Node* _previous = head;
    Node* _current = head->next;
    _current->lock();
    while (_current->key < key)
    {
      _previous->unlock();
      _previous = _current;
      _current = _current->next;
      _current->lock();
    }

    if (_current->key == key)
    {
      //Delete if found
      _previous->next = _current->next;
      _current->unlock();
      _previous->unlock();
      delete _current;
      return true;
    }
    _current->unlock();
    _previous->unlock();
    return false;
  }

  bool contains(const T& item)
  {
    //Generate hash for a provided item
    size_t key = generate_hash(item);
    head->lock();
    //Update _current and _previous so we're in the key position
    Node* _previous = head;
    Node* _current = head->next;
    _current->lock();
    while (_current->key < key)
    {
      _previous->unlock();
      _previous = _current;
      _current = _current->next;
      _current->lock();
    }

    _current->unlock();
    _previous->unlock();
    //Return true if the element was found
    return _current->key == key;
  }

  static void set_error_handler(void(*handler)(const char*))
  {
    error_handler = handler;
  }

private:
  class Node
  {
  public:
    Node(T init_value) : item(init_value), key(std::hash<T>()(init_value)), next(nullptr), mutex(PTHREAD_MUTEX_INITIALIZER) {}

    T item; //Raw data
    size_t key; //Data key (hash)
    Node* next; //Pointer to the next node

                //Lock the node
    void lock()
    {
      if (pthread_mutex_lock(&mutex) != 0)
        error_handler(g_msg_err_mutex_lock);
    }

    //Unlock the node
    void unlock()
    {
      if (pthread_mutex_unlock(&mutex) != 0)
        error_handler(g_msg_err_mutex_unlock);
    }

  private:
    pthread_mutex_t mutex; //Mutex for fine-grained sync
  };

  Node* head; //Head of the list
  static void(*error_handler)(const char*); //Fatal errors handler

  size_t generate_hash(const T& item)
  {
    return std::hash<T>()(item);
  }
};

template <class T>
void(*SetFGS<T>::error_handler)(const char*) = nullptr;

#endif

