#include <asio.hpp>

#include <format>
#include <iostream>
#include <array>
#include <ranges>
#include <algorithm>

#include <log.h>
#include "connected_client.h"

using asio::ip::tcp;


int main()
{
	using namespace cnc;

	constexpr u32 port = 3000;

	asio::io_context ctx;
	tcp::acceptor listener(ctx, tcp::endpoint(tcp::v4(), 3000));

	CNC_INFO(std::format("Server listening on port {}", port));


	std::vector<connected_client> clients;
	std::mutex clients_mutex;


	auto accept_thread = std::thread([&] {
		bool done = false;

		u32 next_id = 1;

		while (!done)
		{
			try
			{
				auto peer = listener.accept();

				{
					std::scoped_lock lock(clients_mutex);
					clients.push_back(connected_client(next_id++, std::move(peer)));
				}

				CNC_INFO("Client accepted");
			}
			catch (std::exception& ex)
			{
				CNC_ERROR(ex.what());
			}
			std::this_thread::yield();
		}
		});

	auto read_thread = std::thread([&] {

		while (true)
		{
			{
				std::scoped_lock lock(clients_mutex);

				// TODO: Maybe async read/write?
				// Read in
				ctx.restart();
				for (auto& cli : clients | not_destroyed)
				{

					try { cli.async_read(); }
					catch (std::exception& ex)
					{
						cli.destroy();
						CNC_INFO("Client disconnected");
					}
				}
				ctx.run();

				// Mix the audio
				

				ctx.restart();
				for (auto& c0 : clients | not_destroyed)
				{
					buffer_t write_buffer = { 0 };
					auto filter_fn = [&](const connected_client& c) { return c != c0; };
					for (auto& c1 : clients | not_destroyed | std::views::filter(filter_fn))
					{
						const auto& read_buf = c1.get_read_buffer();

						for (std::size_t i = 0; i < read_buf.size(); ++i)
							write_buffer[i] += read_buf[i];
					}
					

					try { c0.async_write(write_buffer); }
					catch (std::exception& ex)
					{
						c0.destroy();
						CNC_INFO("Client disconnected");
					}
				}

				ctx.run();

				std::erase_if(clients, [](const connected_client& c) { return c.is_destroyed(); });

			}

			std::this_thread::yield();
		}
	});

	accept_thread.join();
	read_thread.join();

}