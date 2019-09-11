/***********************************************************************************************************************
* @file main.cpp
*
* @brief Test code for the C++11 Thread Pool of Atanas Rusev and Ferai Ali
*
* @details	 
*
*
*  The code is based completely on C++11 features. The purpose is to be able to integrate it
*  in older projects which have not yet reached C++14 or higher. If you need newer features
*  fork the code and get it to the next level yourself.
*
* @author Atanas Rusev and Ferai Ali
*
* @copyright 2019 Atanas Rusev and Ferai Ali, MIT License. Check the License.h file in the library.
*
***********************************************************************************************************************/

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <chrono>
#include "thread_pool.h"

using namespace std::chrono_literals;
using namespace std;

void print(std::string text)
{
	std::cout << text << std::endl;
}


/***********************************************************************************************************************
* @brief A function to test the thread pool with longer tasks
*
* @details	Based on lambdas we add 3000 jobs that are with default priority and 3000 Critical jobs.
*		Then we add a sleep for the main thread that will wait until the jobs are executed for sure. 
*
* @pre Thread pool creation
* @post 
* @param[in]  CTP::ThreadPool &thread_pool
* @return None
*
* @author Atanas Rusev and Ferai Ali
*
* @copyright 2019 Atanas Rusev and Ferai Ali, MIT License. Check the License file in the library.
*
***********************************************************************************************************************/
void run_long_tasks(CTP::ThreadPool &thread_pool)
{
	for (int i = 0; i < 3000; i++)
	{
		// Schedule a lambda that will first wait 10 milliseconds and then print "Hello World i : ", 
		// where i is the for cycle index
		thread_pool.Schedule([i]()
		{
			const auto start = std::chrono::system_clock::now();
			while (start + 10ms > std::chrono::system_clock::now()) {}
			std::cout << "NORM: " << i << std::endl;			
		});

		// Schedule a lambda that will first wait 10 milliseconds and then print "First ! i", where
		// i is the for cycle index
		thread_pool.Schedule(CTP::Priority::Critical, [i]()
		{
			const auto start = std::chrono::system_clock::now();
			while (start + 10ms > std::chrono::system_clock::now()) {}
			std::cout << "CRIT: " << i << std::endl;
		});
	}

	// simply block the main thread to wait a bit
	std::this_thread::sleep_for(15s);
}

/***********************************************************************************************************************
* @brief A function to test the thread pool with shorter tasks
*
* @details	Based on lambdas we add few jobs that are with default priority.
*		Then we add a sleep for the main thread that will wait until the jobs are executed for sure.
*
* @pre Thread pool creation
* @post
* @param[in]  CTP::ThreadPool &thread_pool
* @return None
*
* @author Atanas Rusev and Ferai Ali
*
* @copyright 2019 Atanas Rusev and Ferai Ali, MIT License. Check the License file in the library.
*
***********************************************************************************************************************/
void run_small_tasks(CTP::ThreadPool &thread_pool)
{
	for (int i = 0; i < 2; i++)
	{
		thread_pool.Schedule([i]()
		{
			const auto start = std::chrono::system_clock::now();
			while (start + 2000ms > std::chrono::system_clock::now()) {}
			std::cout << "MINI: " << i << std::endl;
		});
	}
	std::this_thread::sleep_for(2s);
}

/***********************************************************************************************************************
* @brief Main that creates a thread pool and tests it.
*
* @details	Tested with default number of threads (the return of std::thread::hardware_concurrency). Tested also 
* with 1, 4 and 40 threads. Test Machine - HP ZBook 15 G5 with 16 GB RAM and Intel® Core™ i7-8750H Processor.
* Built initially with Microsoft Visual Studio 2015 as a console application.
*
* @author Atanas Rusev and Ferai Ali
*
* @copyright 2019 Atanas Rusev and Ferai Ali, MIT License. Check the License file in the library.
*
***********************************************************************************************************************/
int main()
{
	CTP::ThreadPool thread_pool;
	// CTP::ThreadPool thread_pool(1);
	// CTP::ThreadPool thread_pool(4);
	// CTP::ThreadPool thread_pool(40);

	// example with a short lambda from here:
	auto resultOf34 = thread_pool.Schedule([]()
	{
		int i = 3 * 4;
		print(std::to_string(i));
		return i;
	});

	// example for waiting on a future
	if (resultOf34.wait_for(0ms) == std::future_status::ready)
	{
		auto res = resultOf34.get();
	}

	auto res = resultOf34.get();

	for(int i = 0; i < 2; i++) run_long_tasks(thread_pool);
	
	for (int i = 0; i < 2; i++) run_small_tasks(thread_pool);
	std::this_thread::sleep_for(5s);

	return 0;
}
