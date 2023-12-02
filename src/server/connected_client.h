#pragma once

#include <array>
#include <cinttypes>
#include <ranges>

#include <common.h>

#include <asio.hpp>

namespace cnc
{
	using asio::ip::tcp;

	class connected_client
	{
	private:

		buffer_t _read_buffer;
		buffer_t _write_buffer;

		u32 _id{};
		bool _destroyed{ false };
		tcp::socket _socket;

	public:

		explicit connected_client(const u32 id, tcp::socket&& socket);
		~connected_client();

		auto& get_read_buffer() { return _read_buffer; }
		auto& get_write_buffer() { return _write_buffer; }

		void async_read();
		void async_write(const buffer_t& buf);

		void read();
		void write(const buffer_t& buf);

		connected_client(const connected_client&) = delete;
		connected_client operator=(connected_client&) = delete;

		connected_client(connected_client&&) = default;
		connected_client& operator=(connected_client&&) = default;

		tcp::socket& get_socket() { return _socket; }

		void destroy() { _destroyed = true; }
		bool is_destroyed() const { return _destroyed; }

		u32 get_id() const { return _id; }

		bool operator==(const connected_client& other) const { return _id == other._id; }
		bool operator!=(const connected_client& other) const { return _id != other._id; }

	};

	constexpr auto not_destroyed = std::views::filter([](const connected_client& c) { return !c.is_destroyed(); });


}