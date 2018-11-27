#include <iostream>
#include <cstdlib>
#include <string>

#include "set.h"
#include "set_fgs.hpp"
#include "set_os.hpp"

Set<int>* p_set1 = NULL;
Set<int>* p_set2 = NULL;
Set<int>* working_set = NULL;

size_t writers = 10;
size_t readers = 10;
size_t entries = 10;

int* shared_data = NULL;
size_t tests_n = 10;

void on_error(const char* msg)
{
  std::cerr << msg << std::endl;
  if (p_set1)
    delete p_set1;
  if (p_set2)
    delete p_set2;
  exit(1);
}

struct TestResult
{
public:
  friend std::ostream& operator<< (std::ostream& out, const TestResult& result)
  {
    if (result.success)
    {
      out << "The test succeeded";
      if (result.time)
        out << ". Exceeded time: " << result.time;
    }
    else
      out << "The test failed";
    return out;
  }

  TestResult() : success(false), time(0) {}
  TestResult(bool i_success) : success(i_success), time(0) {}
  bool success;
  int time;
};

class TestEngine
{
public:
  void test_set(Set<int>* p_set);
  void test_speed(Set<int>* p_set1, Set<int>* p_set2);
private:
  TestResult test_readers();
  TestResult test_writers();
  TestResult test_complex();

  void test_speed_randomized(Set<int>* p_set1, Set<int>* p_set2);
  void test_speed_fixed(Set<int>* p_set1, Set<int>* p_set2);
};

void* readers_routine(void* parameter)
{
  int* data = reinterpret_cast<int*>(parameter);
  for (size_t i = 0; i < entries; ++i)
    working_set->remove(data[i]);
  return nullptr;
}

TestResult TestEngine::test_readers()
{
  pthread_t* threads = new pthread_t[readers];
  pthread_attr_t* attributes = new pthread_attr_t[readers];

  if (!threads || !attributes)
    on_error("Memory allocation problem");

  for (size_t i = 0; i < readers * entries; ++i)
    working_set->add(shared_data[i]);

  for (size_t i = 0; i < readers; ++i)
    pthread_attr_init(&(attributes[i]));
  for (size_t i = 0; i < readers; ++i)
    pthread_create(&(threads[i]), &(attributes[i]), readers_routine, shared_data + i * entries);
  for (size_t i = 0; i < readers; ++i)
    pthread_join(threads[i], NULL);

  for (size_t i = 0; i < readers * entries; ++i)
    if (working_set->contains(shared_data[i]))
      return TestResult(false);

  delete[] threads;
  delete[] attributes;
  return TestResult(true);
}

void* writers_routine(void* parameter)
{
  int* data = reinterpret_cast<int*>(parameter);
  for (size_t i = 0; i < entries; ++i)
    working_set->add(data[i]);
  return nullptr;
}

TestResult TestEngine::test_writers()
{
  pthread_t* threads = new pthread_t[writers];
  pthread_attr_t* attributes = new pthread_attr_t[writers];

  if (!threads || !attributes)
    on_error("Memory allocation problem");

  for (size_t i = 0; i < writers; ++i)
    pthread_attr_init(&(attributes[i]));
  for (size_t i = 0; i < writers; ++i)
    pthread_create(&(threads[i]), &(attributes[i]), writers_routine, shared_data + i * entries);
  for (size_t i = 0; i < writers; ++i)
    pthread_join(threads[i], NULL);

  for (size_t i = 0; i < writers * entries; ++i)
    if (!working_set->contains(shared_data[i]))
      return TestResult(false);

  for (size_t i = 0; i < writers * entries; ++i)
    working_set->remove(i);

  delete[] threads;
  delete[] attributes;
  return TestResult(true);
}

TestResult TestEngine::test_complex()
{
  return TestResult(true);
}

void TestEngine::test_set(Set<int>* p_set)
{
  working_set = p_set;

  shared_data = new int[writers * entries];
  if (!shared_data)
    on_error("Memory allocation problem");
  for (size_t i = 0; i < writers * entries; ++i)
    shared_data[i] = (int)i;
  std::cout << "Test Writers...\n" << test_writers() << std::endl;
  delete[] shared_data;

  shared_data = new int[readers * entries];
  if (!shared_data)
    on_error("Memory allocation problem");
  for (size_t i = 0; i < readers * entries; ++i)
    shared_data[i] = (int)i;
  std::cout << "Test Readers...\n" << test_readers() << std::endl;
  delete[] shared_data;

  //TODO Test complex
  std::cout << "Test Complex...\n" << test_complex() << std::endl;
}

void TestEngine::test_speed(Set<int>* p_set1, Set<int>* p_set2)
{
  std::cout << "Randomized" << std::endl;
  test_speed_randomized(p_set1, p_set2);
  std::cout << "Fixed" << std::endl;
  test_speed_fixed(p_set1, p_set2);
}

void TestEngine::test_speed_randomized(Set<int>* p_set1, Set<int>* p_set2)
{
  //TODO Randomized test
  std::cout << "" << std::endl;
}

void TestEngine::test_speed_fixed(Set<int>* p_set1, Set<int>* p_set2)
{
  //TODO Fixed test
  std::cout << "" << std::endl;
}

int main(int argc, char** argv)
{
  //TODO Command-line args
  if (argc != 1 && argc != 4)
    on_error("USAGE: app [readers, writers, entries]");

  SetFGS<int>::set_error_handler(on_error);
  SetOS<int>::set_error_handler(on_error);

  p_set1 = new SetFGS<int>();
  p_set2 = new SetOS<int>();
  if (!p_set1 || !p_set2)
    on_error("Memory allocation problem");

  TestEngine engine;
  std::cout << "\nFine-grained sync set:" << std::endl;
  engine.test_set(p_set1);
  std::cout << "\nOptimistic sync set:" << std::endl;
  engine.test_set(p_set2);
  std::cout << "\nSpeed test:" << std::endl;
  engine.test_speed(p_set1, p_set2);
  delete p_set1;
  delete p_set2;
  return 0;
}

