#include <iostream>
#include <cstdlib>
#include <string>
#include <ctime>

#include "set.h"
#include "set_fgs.hpp"
#include "set_os.hpp"

//Global scope pointers
Set<int>* p_set1 = NULL;
Set<int>* p_set2 = NULL;
Set<int>* working_set = NULL;

//Test options
size_t writers = 10;
size_t readers = 10;
size_t entries = 10;

//Data for speed tests
int* shared_data = NULL;
int* complex_test_data = NULL;
size_t tests_n = 10;
size_t complex_readers_block_size = 0;

//Error handler
void on_error(const char* msg)
{
  std::cerr << msg << std::endl;
  if (p_set1)
    delete p_set1;
  if (p_set2)
    delete p_set2;
  exit(1);
}

//Result of a set test
struct TestResult
{
public:
  friend std::ostream& operator<< (std::ostream& out, const TestResult& result)
  {
    out << (result.success ? "The test succeeded" : "The test failed");
    return out;
  }
  TestResult() : success(false) {}
  TestResult(bool i_success) : success(i_success) {}
  bool success;
};

//Readers test thread routine 
static void* readers_routine(void* parameter)
{
  int* data = reinterpret_cast<int*>(parameter);
  for (size_t i = 0; i < entries; ++i)
    working_set->remove(data[i]);
  pthread_exit(0);
}

//Writers test thread routine
static void* writers_routine(void* parameter)
{
  int* data = reinterpret_cast<int*>(parameter);
  for (size_t i = 0; i < entries; ++i)
    working_set->add(data[i]);
  pthread_exit(0);
}

//Initialize, create and run thread routines (and wait for join)
static void create_and_run_threads(pthread_t* threads, pthread_attr_t* attributes, size_t threads_num, size_t thread_data_entries, void*(*thread_routine)(void*))
{
  for (size_t i = 0; i < threads_num; ++i)
    pthread_attr_init(&(attributes[i]));
  for (size_t i = 0; i < threads_num; ++i)
    if (pthread_create(&(threads[i]), &(attributes[i]), thread_routine, shared_data + i * thread_data_entries) != 0)
      on_error("Too many threads");
  for (size_t i = 0; i < threads_num; ++i)
    pthread_join(threads[i], NULL);
}

static TestResult test_readers()
{
  pthread_t* threads = new pthread_t[readers];
  pthread_attr_t* attributes = new pthread_attr_t[readers];

  if (!threads || !attributes)
    on_error("Memory allocation problem");

  for (size_t i = 0; i < readers * entries; ++i)
    working_set->add(shared_data[i]);

  create_and_run_threads(threads, attributes, readers, entries, readers_routine);

  for (size_t i = 0; i < readers * entries; ++i)
    if (working_set->contains(shared_data[i]))
      return TestResult(false);

  delete[] threads;
  delete[] attributes;
  return TestResult(true);
}

static TestResult test_writers()
{
  pthread_t* threads = new pthread_t[writers];
  pthread_attr_t* attributes = new pthread_attr_t[writers];
  if (!threads || !attributes)
    on_error("Memory allocation problem");

  create_and_run_threads(threads, attributes, writers, entries, writers_routine);

  for (size_t i = 0; i < writers * entries; ++i)
  {
    if (!working_set->contains(shared_data[i]))
      return TestResult(false);
    working_set->remove(shared_data[i]);
  }

  delete[] threads;
  delete[] attributes;
  return TestResult(true);
}

//Readers complex test thread routine 
static void* readers_complex_routine(void* parameter)
{
  int* data = reinterpret_cast<int*>(parameter);
  for (size_t i = 0; i < complex_readers_block_size; ++i)
    if (working_set->contains(data[i]))
      ++complex_test_data[data[i]];
  pthread_exit(0);
}

static void create_and_run_threads_complex_readers(pthread_t* threads, pthread_attr_t* attributes)
{
  for (size_t i = 0; i < readers; ++i)
    pthread_attr_init(&(attributes[i]));
  for (size_t i = 0; i < readers; ++i)
    if (pthread_create(&(threads[i]), &(attributes[i]), readers_complex_routine, shared_data + i * complex_readers_block_size) != 0)
      on_error("Too many threads");
  for (size_t i = 0; i < readers; ++i)
    pthread_join(threads[i], NULL);
}

static TestResult test_complex()
{
  pthread_t* threads = new pthread_t[writers];
  pthread_attr_t* attributes = new pthread_attr_t[writers];
  complex_readers_block_size = entries * ((double)writers / readers);
  complex_test_data = new int[writers * entries];
  if (!complex_test_data || !threads || !attributes)
    on_error("Memory allocation problem");

  for (size_t i = 0; i < writers * entries; ++i)
    complex_test_data[i] = 0;
  create_and_run_threads(threads, attributes, writers, entries, writers_routine);

  delete[] threads;
  delete[] attributes;
  threads = new pthread_t[readers];
  attributes = new pthread_attr_t[readers];
  if (!threads || !attributes)
    on_error("Memory allocation problem");

  create_and_run_threads_complex_readers(threads, attributes);

  size_t test_size = writers * entries - (writers * entries) % complex_readers_block_size;
  for (size_t i = 0; i < test_size; ++i)
    if (complex_test_data[i] != 1)
      return TestResult(false);

  for (size_t i = 0; i < writers * entries; ++i)
    working_set->remove(shared_data[i]);

  delete[] complex_test_data;
  delete[] threads;
  delete[] attributes;
  return TestResult(true);
}

//Prepare shared data for a test
static void prepare_shared_data(size_t threads_num, size_t thread_data_entries)
{
  shared_data = new int[threads_num * thread_data_entries];
  if (!shared_data)
    on_error("Memory allocation problem");
  for (size_t i = 0; i < threads_num * thread_data_entries; ++i)
    shared_data[i] = (int)i;
}

//Rearrange elements in the array
static void shuffle(int* arr, size_t size)
{
  for (size_t i = 0; i < size; ++i)
  {
    size_t ind = rand() % size;
    int tmp = arr[ind];
    arr[ind] = arr[i];
    arr[i] = tmp;
  }
}

//Prepare shared data for the randomized test
static void prepare_shared_data_random(size_t threads_num, size_t threads_data_entries)
{
  shared_data = new int[threads_num * threads_data_entries];
  if (!shared_data)
    on_error("Memory allocation problem");
  for (size_t i = 0; i < threads_num * threads_data_entries; ++i)
    shared_data[i] = (int)i;
  shuffle(shared_data, threads_num * threads_data_entries);
}

//Prepare shared data for the fixed test
static void prepare_shared_data_fixed(size_t threads_num, size_t threads_data_entries)
{
  shared_data = new int[threads_num * threads_data_entries];
  if (!shared_data)
    on_error("Memory allocation problem");
  for (size_t i = 0; i < threads_num; ++i)
    for (size_t j = 0; j < threads_data_entries; ++j)
      shared_data[i * threads_data_entries + j] = (int)(i + j * threads_num);
}

static void test_set(Set<int>* p_set)
{
  working_set = p_set;

  prepare_shared_data(writers, entries);
  std::cout << "Test Writers...\n" << test_writers() << std::endl;
  delete[] shared_data;

  prepare_shared_data(readers, entries);
  std::cout << "Test Readers...\n" << test_readers() << std::endl;
  delete[] shared_data;

  prepare_shared_data(writers, entries);
  std::cout << "Test Complex...\n" << test_complex() << std::endl;
  delete[] shared_data;
}

//Return execution time for test(...) on p_set
static double test_speed_generic(Set<int>* p_set, TestResult(*test)())
{
  clock_t begin, end;
  working_set = p_set;
  begin = clock();
  for (size_t i = 0; i < tests_n; ++i)
    test();
  end = clock();
  return (double)(end - begin) / CLOCKS_PER_SEC / tests_n;
}

//Perform speed test with shared_data building rules in data_creator(...)
static void test_speed_common(Set<int>* p_set1, Set<int>* p_set2, void(*data_creator)(size_t, size_t))
{
  //Test writers
  std::cout << "Test Writers" << std::endl;
  data_creator(writers, entries);
  std::cout << "FGS: Execution time: " << test_speed_generic(p_set1, test_writers) << std::endl;
  std::cout << "OS: Execution time: " << test_speed_generic(p_set2, test_writers) << std::endl;
  delete[] shared_data;

  //Test readers
  std::cout << "Test Readers" << std::endl;
  data_creator(readers, entries);
  std::cout << "FGS: Execution time: " << test_speed_generic(p_set1, test_readers) << std::endl;
  std::cout << "OS: Execution time: " << test_speed_generic(p_set2, test_readers) << std::endl;
  delete[] shared_data;

  //Test complex
  std::cout << "Test Complex" << std::endl;
  data_creator(writers, entries);
  std::cout << "FGS: Execution time: " << test_speed_generic(p_set1, test_complex) << std::endl;
  std::cout << "OS: Execution time: " << test_speed_generic(p_set2, test_complex) << std::endl;
  delete[] shared_data;

}

static void test_speed(Set<int>* p_set1, Set<int>* p_set2)
{
  std::cout << "Randomized" << std::endl;
  test_speed_common(p_set1, p_set2, prepare_shared_data_random);
  std::cout << "Fixed" << std::endl;
  test_speed_common(p_set1, p_set2, prepare_shared_data_fixed);
}

int main(int argc, char** argv)
{
  srand(time(NULL));
  if (argc != 1 && argc != 4)
    on_error("USAGE: app [readers, writers, entries]");

  //Read command-line arguments if there are any
  if (argc == 4)
  {
    readers = atoi(argv[1]);
    writers = atoi(argv[2]);
    entries = atoi(argv[3]);
    if (!readers || !writers || !entries)
      on_error("USAGE: app [readers, writers, entries]");
  }

  //Set error handlers
  SetFGS<int>::set_error_handler(on_error);
  SetOS<int>::set_error_handler(on_error);

  //Create sets
  p_set1 = new SetFGS<int>();
  p_set2 = new SetOS<int>();
  if (!p_set1 || !p_set2)
    on_error("Memory allocation problem");

  //Perform tests
  std::cout << "\nFine-grained sync set:" << std::endl;
  test_set(p_set1);
  std::cout << "\nOptimistic sync set:" << std::endl;
  test_set(p_set2);
  std::cout << "\nSpeed test:" << std::endl;
  test_speed(p_set1, p_set2);
  delete p_set1;
  delete p_set2;
  return 0;
}
