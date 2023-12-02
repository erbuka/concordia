#include <asio.hpp>

#include <format>
#include <iostream>
#include <array>
#include <ranges>
#include <algorithm>

#include <log.h>

using asio::ip::tcp;

using buffer_t = std::array<std::int16_t, 1024>;

class connected_client
{
public:

private:

	buffer_t _read_buffer;
	buffer_t _write_buffer;

	std::uint32_t _id{};
	bool _destroyed{ false };
	tcp::socket _socket;

public:

	explicit connected_client(const std::uint32_t id, tcp::socket&& socket) :
		_id(id),
		_socket(std::move(socket))
	{
	}
	~connected_client()
	{
		if (_socket.is_open())
			_socket.close();
	}

	auto& get_read_buffer() { return _read_buffer; }
	auto& get_write_buffer() { return _write_buffer; }

	connected_client(const connected_client&) = delete;
	connected_client operator=(connected_client&) = delete;

	connected_client(connected_client&&) = default;
	connected_client& operator=(connected_client&&) = default;

	tcp::socket& get_socket() { return _socket; }

	void destroy() { _destroyed = true; }
	bool is_destroyed() const { return _destroyed; }

	std::uint32_t get_id() const { return _id; }

	bool operator==(const connected_client& other) const { return _id == other._id; }
	bool operator!=(const connected_client& other) const { return _id != other._id; }

};

int main()
{
	using namespace cnc;

	constexpr std::uint16_t port = 3000;

	asio::io_context ctx;
	tcp::acceptor listener(ctx, tcp::endpoint(tcp::v4(), 3000));

	CNC_INFO(std::format("Server listening on port {}", port));


	std::vector<connected_client> clients;
	std::mutex clients_mutex;


	auto accept_thread = std::thread([&] {
		bool done = false;

		std::uint32_t next_id = 1;

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
				for (auto& cli : clients)
				{

					std::ranges::fill(cli.get_read_buffer(), 0);
					try { asio::read(cli.get_socket(), asio::buffer(cli.get_read_buffer())); }
					catch (std::exception& ex)
					{
						cli.destroy();
						CNC_INFO("Client disconnected");
					}
				}

				// Mix the audio
				for (auto& c0 : clients)
				{
					std::ranges::fill(c0.get_write_buffer(), 0);
					auto filter_fn = [&](const connected_client& c) { return c != c0; };
					for (auto& c1 : clients | std::views::filter(filter_fn))
					{
						auto& write_buf = c0.get_write_buffer();
						const auto& read_buf = c1.get_read_buffer();

						for (std::size_t i = 0; i < read_buf.size(); ++i)
							write_buf[i] += read_buf[i];
					}
				}

				// Write out
				for (auto& cli : clients)
				{
					try { asio::write(cli.get_socket(), asio::buffer(cli.get_write_buffer())); }
					catch (std::exception& ex)
					{
						cli.destroy();
						CNC_INFO("Client disconnected");
					}

				}

				std::erase_if(clients, [](const connected_client& c) { return c.is_destroyed(); });

			}

			std::this_thread::yield();
		}
		});

	accept_thread.join();
	read_thread.join();

}