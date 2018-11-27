#include <iostream>
#include <cstdlib>
#include <string>

#include "set.h"
#include "set_fgs.hpp"
#include "set_os.hpp"

Set<int>* p_set1 = NULL;
Set<int>* p_set2 = NULL;

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
  bool success;
  int time;
};

class TestEngine
{
public:
  TestEngine() : writers(0), readers(0), entries(0) {}
  TestEngine(size_t i_writers, size_t i_readers, size_t i_entries) : writers(i_writers), readers(i_readers), entries(i_entries) {}

  void test_set(Set<int>* p_set);
  void test_speed(Set<int>* p_set1, Set<int>* p_set2);
private:
  size_t writers;
  size_t readers;
  size_t entries;

  TestResult test_readers(Set<int>* p_set);
  TestResult test_writers(Set<int>* p_set);
  TestResult test_complex(Set<int>* p_set);
};

TestResult TestEngine::test_readers(Set<int>* p_set)
{
  TestResult result;
  result.success = true;
  return result;
}

TestResult TestEngine::test_writers(Set<int>* p_set)
{
  TestResult result;
  result.success = true;
  return result;
}

TestResult TestEngine::test_complex(Set<int>* p_set)
{
  TestResult result;
  result.success = true;
  return result;
}

void TestEngine::test_set(Set<int>* p_set)
{
  std::cout << "Test Readers...\n" << test_readers(p_set) << std::endl;
  std::cout << "Test Writers...\n" << test_writers(p_set) << std::endl;
  std::cout << "Test Complex...\n" << test_complex(p_set) << std::endl;
}

void TestEngine::test_speed(Set<int>* p_set1, Set<int>* p_set2)
{
  std::cout << "" << std::endl;
}

void on_error(const char* msg)
{
  std::cerr << msg << std::endl;
  if (p_set1)
    delete p_set1;
  exit(1);
}

int main(int argc, char** argv)
{
  if (argc != 1 && argc != 4)
    on_error("USAGE: app [readers, writers, entries]");

  SetFGS<int>::set_error_handler(on_error);
  SetOS<int>::set_error_handler(on_error);

  p_set1 = new SetFGS<int>();
  p_set2 = new SetOS<int>();
  if (!p_set1 || !p_set2)
    on_error("Memory allocation problem");

  std::cout << p_set1->contains(0) << std::endl;
  p_set1->add(1);
  p_set1->add(0);
  p_set1->remove(2);
  p_set1->add(2);
  std::cout << p_set1->contains(0) << std::endl;

  std::cout << p_set2->contains(0) << std::endl;
  p_set2->add(0);
  p_set2->add(1);
  p_set2->remove(3);
  std::cout << p_set2->contains(0) << std::endl;

  TestEngine engine;
  engine.test_set(p_set1);

  delete p_set1;
  delete p_set2;
  return 0;
}

