#include "task.h"

#include <algorithm>
#include <ranges>

namespace ml
{
	void task_series::on_update()
	{
		if (_tasks.empty())
		{
			end();
			return;
		}

		auto& current = _tasks.front();
		current->on_update();
		if (current->has_ended())
			_tasks.pop_front();

	}
	void task_par::on_update()
	{
		if (std::ranges::all_of(_tasks, [](const auto& ts) { return ts->has_ended(); }))
		{
			end();
			return;
		}

		for (auto& ts : _tasks | std::views::filter([](const auto& ts) { return !ts->has_ended(); }))
			ts->on_update();

	}


	void task_manager::update()
	{
		for (auto& ts : _tasks)
			if (!ts->has_ended())
				ts->on_update();
		_tasks.erase(std::remove_if(_tasks.begin(), _tasks.end(), [](const auto& ts) { return ts->has_ended(); }), _tasks.end());
	}

	void task_manager::series(std::initializer_list<std::shared_ptr<task>> tasks)
	{
		_tasks.push_back(std::make_shared<task_series>(tasks));
	}

	void task_manager::parallel(std::initializer_list<std::shared_ptr<task>> tasks)
	{
		_tasks.push_back(std::make_shared<task_par>(tasks));
	}


}