#include "miniaudio.h"

#include <asio.hpp>

#include <iostream>
#include <numeric>
#include <algorithm>
#include <limits>
#include <ranges>
#include <chrono>
#include <cmath>
#include <format>

#include <windows.h>
#undef max
#undef min

#include <log.h>

using asio::ip::tcp;


struct config
{
	bool muted{ false };
};

config app_config;
std::uint32_t current_frame = 0;
asio::io_context ctx;
tcp::socket client_socket(ctx);
std::mutex incoming_mutex;
std::vector<std::int16_t> incoming_audio;



void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
	// In playback mode copy data to pOutput. In capture mode read data from pInput. In full-duplex mode, both
	// pOutput and pInput will be valid and you can move data from pInput into pOutput. Never process more than
	// frameCount frames.
	const std::int16_t* input = static_cast<const std::int16_t*>(pInput);
	const std::int16_t* output = static_cast<const std::int16_t*>(pOutput);
	const auto bytes_per_frame = ma_get_bytes_per_frame(pDevice->capture.format, pDevice->capture.channels);
	const auto bytes_max = bytes_per_frame * frameCount;

	if (incoming_audio.size() >= frameCount * 2)
	{
		
		std::scoped_lock lock(incoming_mutex);
		memcpy(pOutput, incoming_audio.data(), bytes_max);
		incoming_audio.erase(incoming_audio.begin(), incoming_audio.begin() + frameCount * 2);
		//incoming_audio.clear();
		//CNC_INFO(std::format("Audio Queue: {}", incoming_audio.size()));
	}

	//memcpy(pOutput, pInput, frameCount * bytes_per_frame);

	if (client_socket.is_open())
	{
		try 
		{ 
			if (app_config.muted)
			{
				std::vector<uint8_t> data(bytes_max, 0);
				asio::write(client_socket, asio::buffer(data));
			}
			else
			{
				//client_socket.send(asio::buffer(pInput, frameCount * bytes_per_frame)); 
				asio::write(client_socket, asio::buffer(pInput, frameCount * bytes_per_frame));
			}
		}
		catch (std::exception& ex) { CNC_ERROR(ex.what()); client_socket.close(); }
	}

}

int main()
{
	ma_result result;
	ma_device_config deviceConfig;
	ma_device device;

	deviceConfig = ma_device_config_init(ma_device_type_duplex);
	deviceConfig.sampleRate = 44100;
	deviceConfig.capture.pDeviceID = NULL;
	deviceConfig.capture.format = ma_format_s16;
	deviceConfig.capture.channels = 2;
	deviceConfig.capture.shareMode = ma_share_mode_shared;
	deviceConfig.playback.pDeviceID = NULL;
	deviceConfig.playback.format = ma_format_s16;
	deviceConfig.playback.channels = 2;
	deviceConfig.dataCallback = data_callback;
	result = ma_device_init(NULL, &deviceConfig, &device);
	if (result != MA_SUCCESS) {
		return result;
	}

	ma_device_start(&device);     // The device is sleeping by default so you'll need to start it manually.

	client_socket.connect(tcp::endpoint(asio::ip::address_v4::from_string("127.0.0.1"), 3000));

	// Do something here. Probably your program's main loop.

	bool done = false;

	auto read_thread = std::thread([&] {
		std::array<std::int16_t, 1024> buffer;

		while (client_socket.is_open())
		{
			try {
				const auto bytes_read = asio::read(client_socket, asio::buffer(buffer));
				if (bytes_read > 0)
				{
					std::scoped_lock lock(incoming_mutex);
					std::ranges::copy(buffer, std::back_inserter(incoming_audio));
				}
				// CNC_INFO(std::format("Incoming size: {}", incoming_audio.size()));
			}
			catch (std::exception& ex)
			{
				CNC_ERROR(ex.what());
				client_socket.close();
			}
		}

		std::this_thread::yield();

	});

	while (!done)
	{
		const auto ch = getchar();
		
		switch (ch)
		{
		case 'm':
			app_config.muted = !app_config.muted;
			CNC_INFO(std::format("Muted: {}", app_config.muted));
			break;
		case 'q':
			done = true;
			CNC_INFO("Quit");
			break;
		default:
			continue;
		}

	}




	ma_device_uninit(&device);

	client_socket.close();

	return 0;
}