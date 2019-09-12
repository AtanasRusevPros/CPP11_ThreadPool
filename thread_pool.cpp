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
*
***********************************************************************************************************************/

#include "thread_pool.h"

#include <thread>
#include <map>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

namespace CTP
{
	//-----------------------------------------------------------------------------
	/// Thread Pool Implementation
	//-----------------------------------------------------------------------------
	class ThreadPool::impl
	{
	public:
		// the main function for initializing the pool and starting the threads
		void Init(size_t threadCount);

		// explicitly shutdown the threads - call this obligatory when wanting 
		// the threads to be stopped. Currently this is performed
		// in the destructor relieving the user from the need to call it himself!
		void Shutdown();

		// the AddJob function takes an Rvalue (double reference) to a std::function object.
		// This std::function object contains a Callable that returns no result and takes no arguments
		void AddJob(std::function<void()>&& job, Priority priority);

	private:
		// this flag is used to control the main loop in the Init function. While it is true the cycle will continue 
		// popping jobs out from the queue. 
		// Initialized as true so that once Init is called the Thread Pool is operational.
		bool m_running = true;

		// m_guard is a mutex that is used while adding a job or extracting one from the queue.
		// Together with the condition variable these control adding jobs to the queue
		// and extracting them so that there are no race conditions.
		std::mutex m_guard;
		std::condition_variable m_cvSleepCtrl;

		// the vector of threads which will process the jobs
		std::vector<std::thread> m_workers;

		// a map of kvp - Key-Value Pair. The pair is the priority level together with it's 
		// corresponding dedicated Queue. This means for each priority we have a separate Queue
		// the last part - std::greater<Priority> - sorts the map in descending order based on the Priority!
		std::map<Priority, std::queue<std::function<void()>>, std::greater<Priority> > m_jobsByPriority;
	};

	// The Constructor simply initializes a single pointer based on the template from the header file in the member:
	// std::unique_ptr<impl> m_impl;
	ThreadPool::ThreadPool(size_t threadCount)
		: m_impl(std::make_unique<ThreadPool::impl>())
	{
		// the only functionality of the Constructor us to call the Init which effectively starts the threads
		// you can of course add more functionality here
		m_impl->Init(threadCount);
	}

	// Destructor 
	ThreadPool::~ThreadPool()
	{
		// Via a call to Shutdown(): Simply notify all threads to finish their work by waking them up.
		// In addition the boolean flag that controlls the execution of the threads is set to false.
		m_impl->Shutdown();
	}

	void ThreadPool::AddJob(std::function<void()> job, Priority priority)
	{
		m_impl->AddJob(std::move(job), priority);
	}

	/***********************************************************************************************************************
	* @brief The main function for initializing the pool and starting the threads.
	* 
	* @details	This is the main function that starts the threads and feeds them with jobs. 
	*		The threads functions are defined by a lambda that is executed inside each new thread
	*		The Queues are 3 and those are sequentially checked in decreasing order for next job to be executed
	*
	* @pre None
	* @post 
	* @param[in]  size_t threadCount - the number of threads you want to start
	* @return None
	*
	* @author Atanas Rusev and Ferai Ali
	*
	* @copyright 2019 Atanas Rusev and Ferai Ali, MIT License. Check the License file in the library.
	*
	***********************************************************************************************************************/
	void ThreadPool::impl::Init(size_t threadCount)
	{
		// First we explicitly initialize the 3 queues
		m_jobsByPriority[Priority::Normal] = {};
		m_jobsByPriority[Priority::High] = {};
		m_jobsByPriority[Priority::Critical] = {};

		// now explicitly reserve for the vector of threads the exact number of threads whished
		m_workers.reserve(threadCount);

		// this is where each thread is created to consume jobs from the queues
		// if we have e.g. only normal jobs - and as we have multiple threads - then 
		// we shall lock when a job is added and when a job is extracted to avoid race conditions
		for (int i = 0; i < threadCount; i++)
		{
            //------------------------------------------------------------------------------------
            // MAIN EXECUTION BLOCK of each thread 
			//------------------------------------------------------------------------------------
            // push back a lambda function for each thread. Capturing "this" pointer inside the lambda 
            // function will automatically capture all the member variables for this object inside the lambda.
            // This means the next code is executed INSIDE the corresponding thread:
			m_workers.push_back(std::thread([this](){

				// the bool m_running is initialized as true upon object creation
                // we check it here to know when to stop consuming jobs
				while (m_running)
				{
					// we create here one empty function wrapper. For std::function we have in cppreference.com:
					/*-------------------------------------------------------------------------------------------------
                    Class template std::function is a general-purpose polymorphic function wrapper. Instances 
					of std::function can store, copy, and invoke any Callable target -- functions, lambda 
					expressions, bind expressions, or other function objects, as well as pointers to member 
					functions and pointers to data members.

					The stored callable object is called the target of std::function. If a std::function contains 
					no target, it is called empty. Invoking the target of an empty std::function results in 
					std::bad_function_call exception being thrown.
					---------------------------------------------------------------------------------------------------*/
					std::function<void()> job;
                    
					{   // here follows the part that needs to be locked - so we create a unique_lock class object
                        // and pass to it our mutex.
						std::unique_lock<std::mutex> ul(m_guard);

                        // once we have the lock mutex object we can wait on it via the condition variable
                        // wait causes the current thread to block until the condition variable is 
                        // notified or a spurious wakeup occurs, optionally looping until some predicate is satisfied. 
                        // in our case we wait on the lambda return result. We pass again "this" to capture the member variables
                        // for this object inside the lambda
						// the template for wait accepts a mutex AND a predicate. If the wait should be continued - the 
						// predicate (i.e. th lambda) shall return false
						m_cvSleepCtrl.wait(ul, [this](){

							// if the flag is set to false explicitly stop processing jobs from the queue
							if (false == m_running)
							{
								return true;
							}

                            // the given thread loops here through all queues
							bool allQueuesEmpty = true;
							for (const auto& kvp : m_jobsByPriority) // kvp - acronym for Key-Value pair
							{
								allQueuesEmpty &= kvp.second.empty();
							}
							// we return here the result - if all queues ARE EMPTY we return false and the 
							// condition variable will continue waiting
							return !allQueuesEmpty;
						});

						// once we are done waiting - we loop through a Key-Value Pair based on priority to get
						// next job from the Queues. Remember - those are sorted in descending order upon map creation!
						for (auto& kvp : m_jobsByPriority)
						{
							auto& jobs = kvp.second; // we take here the Queue based on the Priority
							if (jobs.empty())		// if the current Queue is empty - we go the next Queue
							{
								continue;
							}
							job = std::move(jobs.front()); // once we know the current queue has a job we move it
							jobs.pop();						// and we pop one element from this Queue
							break;		
						}
					}

					// and finally we execute the job
					if (job != nullptr)
					{
						job();
					}
				}
			}));
		}
	}

	/***********************************************************************************************************************
	* @brief explicitly shutdown the threads - call this obligatory when wanting the threads to be stopped.
	* 
	* @details	Currently this is performed in the destructor relieving the user from the need to call it himself!
	*	Once the function is called - each thread will finish it's current job and will not take a new one.
	*	Calling it in the destructor means we will perform it exactly at program termination.
	*	And of course it is expected, that then we have waited all future objects to be consumed, 
	*	and so it is both safe and thread exit is correctly and undoubtedly waited at program termination.
	*	
	* @pre None
	* @post None
	* @param[in]  None
	* @param[out]  None
	* @return None
	*	
	* @author Atanas Rusev and Ferai Ali
	*
	* @copyright 2019 Atanas Rusev and Ferai Ali, MIT License. Check the License file in the library.
	*
	***********************************************************************************************************************/
	void ThreadPool::impl::Shutdown()
	{
		// set the global flag for disabling any thread to continue extracting jobs and execute the main loop code
		m_running = false;

		// now notify all threads (effectively waking them up) so that they either execute their last job 
		// and/or directly stop working as the main flag is false
		m_cvSleepCtrl.notify_all();

		// finally join all threads to ensure all of them are waited to finish before destroying the thread pool
		for (auto& worker : m_workers)
		{
			if (worker.joinable())
			{
				worker.join();
			}
		}
	}


	/***********************************************************************************************************************
	* @brief explicitly shutdown the threads - call this obligatory when wanting the threads to be stopped.
	*
	* @details	Currently this is performed in the destructor relieving the user from the need to call it himself!
	*	Once the function is called - each thread will finish it's current job and will not take a new one.
	*	Calling it in the destructor means we will perform it exactly at program termination.
	*	And of course it is expected, that then we have waited all future objects to be consumed,
	*	and so it is both safe and thread exit is correctly and undoubtedly waited at program termination.
	*
	* @pre None
	* @post None
	* @param[in]  None
	* @param[out]  None
	* @return None
	*
	* @author Atanas Rusev and Ferai Ali
	*
	* @copyright 2019 Atanas Rusev and Ferai Ali, MIT License. Check the License file in the library.
	*
	***********************************************************************************************************************/
	void ThreadPool::impl::AddJob(std::function<void()>&& job, Priority priority)
	{
		// first we lock our "one single common" thread pool mutex to ensure no overlapping (race condition)
		// at job addition
		std::unique_lock<std::mutex> ul(m_guard);

		// then we add the new job
		m_jobsByPriority[priority].emplace(std::move(job));
		
		// finally we notify at least one thread
		m_cvSleepCtrl.notify_one();
	}
} //end of namespace CTP
