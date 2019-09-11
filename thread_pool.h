/***********************************************************************************************************************
* @file thread_pool.h
*
* @brief Template based Thread Pool with Pimpl concept implementation. It accepts 3 types of jobs by priority.
*
* @details	 This Thread pool is created as a class with template based functions to ensure
*	different possible input job types - a lambda, a class method, or a function.
*
*	It is based on the Pimpl paradigm:
*	"Pointer to implementation" or "pImpl" is a C++ programming technique[1] that removes
*  implementation details of a class from its object representation by placing them in a
*  separate class, accessed through an opaque pointer
*
*  The Queues are 3 - Critical (2), High (1), and Normal(0) Priority
*
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
#pragma once
#ifndef CTP_THREAD_POOL_H
#define CTP_THREAD_POOL_H

#include <future>
#include <functional>

namespace CTP
{
	template<typename F, typename... Args>
	using JobReturnType = typename std::result_of<F(Args...)>::type;

	// this is the priority of the jobs. Most jobs shall be ran as Normal priority. 
	enum class Priority : size_t
	{
		Normal,
		High,
		Critical
	};

	class ThreadPool
	{
	public:
		// with this constructor we take by default the number of hardware threads possible. 
		// pay attenttion - an Intel CPU with Hyperthreading will report double the number of HW cores
		// if you want to explicitly limit the number of threads to the number of cores and NOT use hyperthreading - 
		// you have to write a Windows, MAC or Linux specific code!
		ThreadPool(size_t threadCount = std::thread::hardware_concurrency());
		
		// Defaulted default constructor: the compiler will define the implicit default constructor even 
		// if other constructors are present.
		ThreadPool(ThreadPool&&) = default;
		ThreadPool& operator=(ThreadPool&&) = default;

		~ThreadPool();

		// explicitly forbid copy constructors by reference or asignment, so that the thread pool is only one!
		ThreadPool(const ThreadPool&) = delete;
		ThreadPool& operator=(const ThreadPool&) = delete;

		//-----------------------------------------------------------------------------
		/// Adds a job for a given priority level. Returns a future.
		//
		// This is a template function that takes a function of implementation defined
		// type, hence we are freed from the necessity to define overloaded versions
		// for different input. It is transferred as an Rvalue (double reference)
		// The arguments are provided as variadic template args.
		// The return type is a trailing return type. Reason - different functions may 
		// have  different return types. In addition we recieve an std::future to be 
		// able to get notification for the job done.
		//-----------------------------------------------------------------------------
		template <typename F, typename... Args>
		auto Schedule(Priority priority, F&& f, Args&&... args)
			->std::future<JobReturnType<F, Args...>>
		{
			auto job = std::make_shared<std::packaged_task<JobReturnType<F, Args...>()>>
				(
					std::bind(std::forward<F>(f), std::forward<Args>(args)...)
					);

			AddJob([job] { (*job)(); }, priority);
			return job->get_future();
		}

		//-----------------------------------------------------------------------------
		/// Adds a job with DEFAULT priority level (Normal). Returns a future.
		//-----------------------------------------------------------------------------
		template <typename F, typename... Args>
		auto Schedule(F&& f, Args&&... args)
			->std::future<JobReturnType<F, Args...>>
		{
			return Schedule(Priority::Normal, std::forward<F>(f), std::forward<Args>(args)...);
		}

	private:
		// internally a job is a void function with no arguments
		// 
		void AddJob(std::function<void()> job, Priority priority);

		// we use the Pimpl technique, so we need an implementation class
		// and a unique pointer to it. The class definition and declaration are separated from the template
		// thus serving the Pimpl concept.
		class impl;
		// the pointer is based on the std::unique_ptr<...> template. This is a smart pointer that owns and 
		// manages another object through a pointer and disposes of that object when the unique_ptr goes out of scope.
		// The object is disposed of using the associated deleter when either of the following happens :
		//	- the managing unique_ptr object is destroyed
		//	- the managing unique_ptr object is assigned another pointer via operator= or reset().
		std::unique_ptr<impl> m_impl;
	};

} // end of namespace CTP

#endif CTP_THREAD_POOL_H