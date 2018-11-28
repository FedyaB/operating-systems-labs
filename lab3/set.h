#ifndef SET_KFOEFV__
#define SET_KFOEFV__

//Error messages
static const char* g_msg_err_mutex_lock = "Mutex lock error";
static const char* g_msg_err_mutex_unlock = "Mutex unlock error";
static const char* g_msg_err_node_create = "Node creation error";
static const char* g_msg_err_error_handler = "Error handler was not initialized";

//Set class
template <class T>
class Set
{
public:
  virtual ~Set() {}

  //Add an element (true if was added)
	virtual bool add(const T& item) = 0;
	//Remove an element (true if was deleted)
  virtual bool remove(const T& item) = 0;
  //Check whether an element is in the set
	virtual bool contains(const T& item) = 0;
};

#endif

