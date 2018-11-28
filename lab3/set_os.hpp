#ifndef SET_OS_EDDDDO__
#define SET_OS_EDDDDO__

#include <functional>
#include <pthread.h>
#include <bits/stdc++.h>

#include "set.h"

//Optimistic synchronization set
template <class T>
class SetOS: public Set<T>
{
public:
  SetOS()
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

  ~SetOS() 
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
    size_t key = generate_hash(item);
    while (true)
    {
      //Update _current and _previous so we're in the key position
      Node* _previous = head;
      Node* _current = head->next;
      while (_current->key < key)
      {
        _previous = _current;
        _current = _current->next;
      }

      //Lock the nodes under the position
      _previous->lock();
      _current->lock();
      //Check that _previous points to _current and is reachable from the head
      if (validate(_previous, _current))
      {
        if (_current->key == key)
        {
          _previous->unlock();
          _current->unlock();  
          return false;
        }
        else
        {
          //Insert a new node
          Node* to_insert = new Node(item);
          if (!to_insert)
            error_handler(g_msg_err_node_create);
          to_insert->next = _current;
          _previous->next = to_insert;
          _previous->unlock();
          _current->unlock();
          return true;
        }
      }
      _previous->unlock();
      _current->unlock();
    }
  }

  bool remove(const T& item)
  {
    size_t key = generate_hash(item);
    while (true)
    {
      //Update _current and _previous so we're in the key position
      Node* _previous = head;
      Node* _current = head->next;
      while (_current->key < key)
      {
        _previous = _current;
        _current = _current->next;
      }

      _previous->lock();
      _current->lock();
      //Check that _previous points to _current and is reachable from the head
      if (validate(_previous, _current))
      {
        if (_current->key == key)
        {
          //Delete if found
          _previous->next = _current->next;
          delete _current;
          _previous->unlock();
          _current->unlock();
          return true;
        }
        else
        {
          _previous->unlock();
          _current->unlock();
          return false;
        }
      }
      _previous->unlock();
      _current->unlock();
    }
  }

  bool contains(const T& item)
  {
    size_t key = generate_hash(item);
    while (true)
    {
      Node* _previous = head;
      Node* _current = head->next;
      while (_current->key < key)
      {
        _previous = _current;
        _current = _current->next;
      }

      _previous->lock();
      _current->lock();
      if (validate(_previous, _current))
      {
        _previous->unlock();
        _current->unlock();
        return _current->key == key;
      }
      _previous->unlock();
      _current->unlock();
    }
  }

  static void set_error_handler(void (*handler)(const char*))
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
    pthread_mutex_t mutex; //Mutex for a node lock
  };

  Node* head; //Head of the list
  static void (*error_handler)(const char*); //Fatal errors handler

  //Validate that _previous points to _current and is reachable from the head
  bool validate(Node* previous, Node* current)
  {
    Node* node = head;
    while (node->key <= previous->key)
    {
      if (node == previous)
        return previous->next == current;
      node = node->next;
    }
    return false;
  }

  size_t generate_hash(const T& item)
  {
    return std::hash<T>()(item);
  }
};

template <class T>
void (*SetOS<T>::error_handler)(const char*) = nullptr;

#endif


