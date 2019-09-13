/***********************************************************************************************************************
* @file thread_pool.cpp
*
* @brief Template based Thread Pool - the Pimpl concept implementation. It accepts 3 types of jobs by priority.
*
* @details	 This Thread pool is created as a class with template based functions to ensure
*	different possible input job types - a lambda, a class method, a functor or a function.
*
*	It is based on the Pimpl paradigm:
*	"Pointer to implementation" or "pImpl" is a C++ programming technique[1] that removes
*  implementation details of a class from its object representation by placing them in a
*  separate class, accessed through an opaque pointer
* 
*  The jobs insertion and extraction is kept safe via one single mutex to avoid race conditions.
* 
*  The Queues are 3 - Critical (2), High (1), and Normal(0) Priority
* 
*  Each Thread sequentially checks the Queues from a map of Key-Value Pairs - a pair fo the priority and 
*  an element from a vector of threads.
*  
*  Once all queues are empty - the current thread is blocked until notified via a condition variable.
*  
*  The Condition variable wait is blocking the thread in a sleep mode.
*  
*  There is a shutdown function which ensures all threads will stop taking new jobs based on a boolean flag.
*  It is called in the destructor. It will join all threads and wait for the end of each of them to execute
*  and exit.
*  
*  The code is based completely on C++11 features. The purpose is to be able to integrate it
*  in older projects which have not yet reached C++14 or higher. If you need newer features
*  fork the code and get it to the next level yourself.
*
* @author Atanas Rusev and Ferai Ali
*
* @copyright 2019 Atanas Rusev and Ferai Ali, MIT License. Check the License.h file in the library.
*/

# Usage: 
Create an object in the beginning of your program with optional number of threads:
CTP::ThreadPool thread_pool(optional);

If no parameter is given - the Thread Pool will create X threads, where X is the number of supported hardware threads as reported by std::thread::hardware_concurrency()

Further simply call the Thread Pool thread_pool.Schedule(xxx) function with a lambda or a function.

The main.cpp in the project illustrates how it was tested and how it works.

# More Information: 
About a thread pool you can check this short article I wrote: 
http://atanasrusev.com/2019/09/13/thread-pool-design-pattern/

Following is a list of most of the important C++ elements and concepts we used in our code that can help you understand it fully:

https://en.cppreference.com/w/cpp/thread/thread/hardware_concurrency
https://en.cppreference.com/w/cpp/thread/unique_lock
https://en.cppreference.com/w/cpp/thread/sleep_for
https://en.cppreference.com/w/cpp/thread/future
https://en.cppreference.com/w/cpp/utility/move
https://en.cppreference.com/w/cpp/container/map/emplace
https://en.cppreference.com/w/cpp/container/map
https://www.geeksforgeeks.org/descending-order-map-multimap-c-stl/
https://en.cppreference.com/w/cpp/memory/unique_ptr
https://en.cppreference.com/w/cpp/utility/functional/function
https://en.cppreference.com/w/cpp/language/default_constructor
https://en.cppreference.com/w/cpp/utility/functional/bind
https://en.cppreference.com/w/cpp/thread/condition_variable/wait
https://en.cppreference.com/w/cpp/language/pimpl
https://en.cppreference.com/w/cpp/memory/shared_ptr
https://en.cppreference.com/w/cpp/memory/shared_ptr/make_shared
https://en.cppreference.com/w/cpp/thread/packaged_task
