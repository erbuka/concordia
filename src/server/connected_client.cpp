#include "connected_client.h"

#include <algorithm>
#include <ranges>

#include "log.h"

namespace cnc
{
	connected_client::connected_client(const u32 id, tcp::socket&& socket) :
		_id(id),
		_socket(std::move(socket))
	{
	}
	connected_client::~connected_client()
	{
		if (_socket.is_open())
			_socket.close();
	}

	void connected_client::async_read()
	{

		// Empty the queue
		while (_socket.available() > max_queue_size_in_bytes)
		{
			const auto discared_bytes = asio::read(_socket, asio::buffer(_read_buffer));
			CNC_INFO(std::format("Discarded {} bytes from client {}", discared_bytes, get_id()));
		}

		std::ranges::fill(_read_buffer, 0);

		asio::async_read(_socket, asio::buffer(_read_buffer), [this](const asio::error_code error, const std::size_t bytes_read) {
			if (error)
			{
				CNC_ERROR(std::format("Destroying client {}: {}", get_id(), error.message()));
				destroy();
			}
		});
	}

	void connected_client::async_write(const buffer_t& buf)
	{
		std::ranges::copy(buf, _write_buffer.begin());
		asio::async_write(_socket, asio::buffer(_write_buffer), [this](const asio::error_code error, const std::size_t bytes_written) {
			if (error)
			{
				CNC_ERROR(std::format("Destroying client {}: {}", get_id(), error.message()));
				destroy();
			}
		});
	}

	void connected_client::read()
	{
		std::ranges::fill(_read_buffer, 0);
		const auto read_bytes = asio::read(_socket, asio::buffer(_read_buffer));
	}

	void connected_client::write(const buffer_t& buf)
	{
		asio::write(_socket, asio::buffer(buf));
	}

}