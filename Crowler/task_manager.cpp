#include "task_manager.h"

task_manager::task_manager( unsigned int thread_count )
{
	for ( unsigned int i = 0; i < thread_count; ++i )
	{
		threads_.emplace_back( [this](){ work(); } );
	}
}

task_manager::~task_manager()
{
	stop_.store( true );
	cv_.notify_all();
	cv_wait_.notify_all();

	for ( auto& t : threads_ ) 
	{
		if ( t.joinable() )
			t.join();
	}
}

void task_manager::wait()
{
	std::unique_lock<std::mutex> lock( mtx_ );
	cv_wait_.wait( lock, [this]() 
		{
			return tasks_.empty() && active_tasks_ == 0;
		} );
}

void task_manager::push_task( const std::function<void()>& task )
{
	{
		std::lock_guard<std::mutex> lock( mtx_ );
		tasks_.push( task );
	}
	cv_.notify_one();
}

void task_manager::push_tasks( const std::vector<std::function<void()>>& tasks )
{
	{
		std::lock_guard<std::mutex> lock( mtx_ );
		for ( const auto& task : tasks )
		{
			tasks_.push( task );
		}
	}
	cv_.notify_all();
}

void task_manager::work()
{
	std::unique_lock<std::mutex> lock( mtx_ );
	while ( true ) 
	{
		cv_.wait( lock, [this]() {
			return stop_.load() || !tasks_.empty();
			} );

		if ( stop_.load() && tasks_.empty() )
			break;

		if ( !tasks_.empty() ) 
		{
			auto task = tasks_.front();
			tasks_.pop();

			active_tasks_++;

			lock.unlock();
			try
			{
				task();
			}
			catch ( ... ) {
			}
			lock.lock();

			active_tasks_--;

			if ( tasks_.empty() && active_tasks_ == 0 )
				cv_wait_.notify_all();
		}
	}
}
