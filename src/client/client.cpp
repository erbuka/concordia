#include "client.h"

#include <span>

#include <core/vecmath.h>
#include <core/application.h>
#include <common.h>

#include <miniaudio.h>
#include <asio.hpp>

#include "log.h"

#undef max

namespace cnc
{
	using namespace ml;
	using asio::ip::tcp;

	static constexpr vec2i s_window_size = { 400, 400 };
	static constexpr auto s_projection = ortho<float>(0, s_window_size[0], 0, s_window_size[0]);

	struct voice_chat_scene_impl 
	{
		static constexpr std::size_t network_buffer_count = 20;

		voice_chat_config config;
		ma_device device{};
		asio::io_context ctx{};
		tcp::socket socket;
		std::thread read_thread;


		exclusive_resource<std::vector<sample_t>> incoming_audio;

		std::array<buffer_t, network_buffer_count> network_buffers;
		std::atomic_size_t current_network_buffer_idx{ 0 };

		voice_chat_scene_impl() : socket(ctx) {};

		auto& get_current_network_buffer() { return network_buffers[current_network_buffer_idx]; }
		void next_network_buffer() { current_network_buffer_idx = (current_network_buffer_idx + 1) % network_buffers.size(); }

		sample_t sample_incoming_audio(const float age)
		{
			static constexpr auto total_samples = network_buffer_count * buffer_size;
			const std::size_t offset = buffer_size * current_network_buffer_idx;
			const std::size_t target_sample_idx = (offset + static_cast<std::size_t>(age * total_samples)) % total_samples;
			/*
			const auto buffer_idx = target_sample_idx / buffer_size;
			const auto idx = target_sample_idx % buffer_size;
			return network_buffers[buffer_idx][idx];
			*/

			return *(static_cast<sample_t*>((void*)network_buffers.data()) + target_sample_idx);

		}


	};

	static void data_callback(ma_device* device, void* raw_output, const void* raw_input, ma_uint32 frame_count)
	{
		static std::vector<sample_t> processed_input;

		auto& impl = *(static_cast<voice_chat_scene_impl*>(device->pUserData));

		std::span<const sample_t> input(static_cast<const sample_t*>(raw_input), frame_count * audio_channels);
		std::span<sample_t> output(static_cast<sample_t*>(raw_output), frame_count * audio_channels);


		const auto bytes_per_frame = ma_get_bytes_per_frame(impl.device.capture.format, audio_channels);
		const auto bytes_max = bytes_per_frame * frame_count;

		impl.incoming_audio.use([&](auto& ia) {
			if (ia.size() >= frame_count * audio_channels)
			{
				std::copy(ia.begin(), ia.begin() + frame_count * audio_channels, output.begin());
				// memcpy(raw_output, ia.data(), bytes_max);
				ia.erase(ia.begin(), ia.begin() + frame_count * audio_channels);
			}
		});

		//memcpy(pOutput, pInput, frameCount * bytes_per_frame);

		if (impl.socket.is_open())
		{
			processed_input.resize(input.size());
			std::ranges::transform(input, processed_input.begin(), [vol = impl.config.input_volume](const sample_t s) { return s * vol; });

			try { asio::write(impl.socket, asio::buffer(processed_input)); }
			catch (std::exception& ex) { CNC_ERROR(ex.what()); impl.socket.close(); }
		}

	}

	void voice_chat_scene::init()
	{
		// Audio device
		ma_device_config config;

		config = ma_device_config_init(ma_device_type_duplex);
		config.sampleRate = audio_sample_rate;
		config.capture.pDeviceID = NULL;
		config.capture.format = ma_format_s16;
		config.capture.channels = audio_channels;
		config.capture.shareMode = ma_share_mode_shared;
		config.playback.pDeviceID = NULL;
		config.playback.format = ma_format_s16;
		config.playback.channels = audio_channels;
		config.pUserData = _impl;
		config.dataCallback = data_callback;

		if (ma_device_init(NULL, &config, &_impl->device) != MA_SUCCESS) {
			throw std::runtime_error("Can't initialize audio device!");
		}

		ma_device_start(&_impl->device);     // The device is sleeping by default so you'll need to start it manually.


		// Connection
		_impl->socket.connect(tcp::endpoint(asio::ip::address_v4::from_string("127.0.0.1"), 3000));

		_impl->read_thread = std::thread([&] {

			auto& socket = _impl->socket;
			auto& incoming_audio = _impl->incoming_audio;

			while (socket.is_open())
			{
				try {
					

					auto& buffer = _impl->get_current_network_buffer();

					const auto bytes_read = asio::read(socket, asio::buffer(buffer));
					
					if (bytes_read > 0)
					{
						auto [lock, res] = incoming_audio.get();
						std::ranges::copy(buffer, std::back_inserter(res));
					}
					_impl->next_network_buffer();

					// CNC_INFO(std::format("Incoming size: {}", incoming_audio.size()));
				}
				catch (std::exception& ex)
				{
					CNC_ERROR(ex.what());
					socket.close();
				}

				std::this_thread::yield();
			}

		});


		app::set_framebuffer_srgb(true);
	}

	voice_chat_scene::~voice_chat_scene() = default;

	void voice_chat_scene::on_attach()
	{
		_impl = new voice_chat_scene_impl();

		_tx_background = texture2d::load_from_file("assets/ui_bg.png", texture_format::rgba8, texture_filter_mode::linear);

		app::set_window_size(s_window_size);
		
		init();
	}

	void voice_chat_scene::on_update()
	{

		static constexpr auto ws = s_window_size;

		_ui.capture_input();

		app::set_projection(s_projection);
		app::clear(vec4f{ 0.0f, 0.0f, 0.0f, 0.0f });

		// Background
		app::push();
		//app::color(8.0f);
		app::pivot({ 0, 0 });
		app::texture(_tx_background);
		app::quad({ 0, 0, }, vec2f{ ws });
		app::no_texture();
		app::pop();
		
		// Wave form
		app::begin(app::primitive_type::line_strip);
		for (auto x : std::views::iota(0, s_window_size[0]))
		{
			static constexpr auto normalizer = static_cast<float>(std::numeric_limits<i16>::max());
			const float y = s_window_size[1] * .5f + _impl->sample_incoming_audio(x / float(s_window_size[0])) / normalizer * s_window_size[1] * .5f;
			app::vertex(vec2f{ x, y });
		}
		app::end();

		// Mute button
		app::color(_impl->config.input_volume == 0.0f ? vec3f{ 1.0f, 0.0f, 0.0f } : vec3f{ 0.0f, 1.0f, 0.0f });
		if (_ui.round_button(1, { ws[0] / 2.0f, ws[1] / 4.0f }, 50)) {
			_impl->config.input_volume = (1.0f - _impl->config.input_volume);
		}

		// Window dragging
		vec2f drag_delta;
		if (_ui.window_drag(drag_delta))
			app::set_window_pos(app::get_window_pos() + vec2i{ drag_delta });

		app::flush();
	}

	void voice_chat_scene::on_detach()
	{
		ma_device_uninit(&_impl->device);
		_impl->socket.close();
		_impl->read_thread.join();
		delete _impl;
	}

}