#pragma once

#include <type_traits>
#include <concepts>
#include <list>
#include <vector>
#include <memory>

namespace ml
{

	class task
	{
	private:
		bool _ended{ false };
	protected:
		void end() { _ended = true; }
	public:

		virtual ~task() = default;
		
		virtual void on_update() = 0;

		bool has_ended() const { return _ended; }

	};

	class task_series : public task
	{
	private:
		std::list<std::shared_ptr<task>> _tasks;
	public:
		task_series() = default;
		task_series(std::initializer_list<std::shared_ptr<task>> tasks) : _tasks(tasks) {}

		void on_update() override;

	};

	class task_par : public task
	{
	private:
		std::vector<std::shared_ptr<task>> _tasks;
	public:
		task_par() = default;
		task_par(std::initializer_list<std::shared_ptr<task>> tasks) : _tasks(tasks) {}

		void on_update() override;

	};



	template<typename UpdateFunc> requires requires(UpdateFunc f) { { f() } -> std::convertible_to<bool>; }
	class task_fn : public task
	{
	private:
		UpdateFunc _update_func;
	public:
		task_fn(UpdateFunc&& f) : _update_func(std::move(f)) {}

		void on_update() override { if (_update_func()) end(); }


	};

	template<typename UpdateFunc>
	auto make_task_fn(UpdateFunc&& fn)
	{
		return std::make_shared<task_fn<UpdateFunc>>(std::move(fn));
	}


	class task_manager
	{
	private:
		std::list<std::shared_ptr<task>> _tasks;
	public:
		task_manager() = default;

		void series(std::initializer_list<std::shared_ptr<task>> tasks);
		void parallel(std::initializer_list<std::shared_ptr<task>> tasks);

		void update();

		bool is_busy() const { return !_tasks.empty(); }

	};

}