#include "client.h"

#include <span>

#include <glad/glad.h>

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

	namespace colors
	{
		constexpr vec3f blue = vec3f{ .8f, 1.2f, 2.0f };
		constexpr vec3f red = vec3f{ 2.0f, .8f, 1.2f };
	}

	static constexpr vec2i s_window_size = { 400, 400 };
	static constexpr vec2f s_slider_size = { 168, 10 };

	static constexpr auto s_projection = ortho<float>(0, s_window_size[0], 0, s_window_size[0]);


	struct voice_chat_scene_impl 
	{

		voice_chat_config config;

		float saved_input_volume{1.f}, saved_output_volume{1.f};


		ma_device device{};
		asio::io_context ctx{};
		tcp::socket socket;


		float bandwidth_in{ 0 };
		float bandwidth_out{ 0 };

		std::thread read_thread, stats_thread;

		std::atomic_bool running{ true };

		exclusive_resource<std::vector<sample_t>> incoming_audio;

		history_buffer input_history;
		history_buffer output_history;
		
		voice_chat_scene_impl() : socket(ctx) {};


	};

	static void data_callback(ma_device* device, void* raw_output, const void* raw_input, ma_uint32 frame_count)
	{
		static std::vector<sample_t> processed_input;

		auto& impl = *(static_cast<voice_chat_scene_impl*>(device->pUserData));

		std::span<const sample_t> input(static_cast<const sample_t*>(raw_input), frame_count * audio_channels);
		std::span<sample_t> output(static_cast<sample_t*>(raw_output), frame_count * audio_channels);


		impl.incoming_audio.use([&](auto& ia) {
			if (ia.size() >= frame_count * audio_channels)
			{
				std::copy(ia.begin(), ia.begin() + frame_count * audio_channels, output.begin());
				ia.erase(ia.begin(), ia.begin() + frame_count * audio_channels);
			}
		});

		if (impl.socket.is_open())
		{
			processed_input.resize(input.size());
			
			std::ranges::transform(input, processed_input.begin(), [vol = impl.config.input_volume](const sample_t s) { return s * vol; });
			std::ranges::copy(processed_input, std::back_inserter(impl.input_history));
			
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

		if (ma_device_init(NULL, &config, &_impl->device) != MA_SUCCESS) 
		{
			throw std::runtime_error("Can't initialize audio device!");
		}

		ma_device_start(&_impl->device);     // The device is sleeping by default so you'll need to start it manually.


		// Connection

		_impl->read_thread = std::thread([this] {

			buffer_t buffer;

			auto& socket = _impl->socket;
			auto& incoming_audio = _impl->incoming_audio;

			while (_impl->running)
			{
				try {
					
					if (socket.is_open())
					{
						_state = connection_state::connected;
						const auto bytes_read = asio::read(socket, asio::buffer(buffer));

						std::ranges::for_each(buffer, [vol = _impl->config.output_volume](sample_t& s) { s *= vol; });
						std::ranges::copy(buffer, std::back_inserter(_impl->output_history));

						if (bytes_read > 0)
						{
							auto [lock, res] = incoming_audio.get();
							std::ranges::copy(buffer, std::back_inserter(res));
						}
					}
					else
					{
						_state = connection_state::disconnected;
						asio::error_code error;
						_impl->socket.connect(tcp::endpoint(asio::ip::address_v4::from_string("127.0.0.1"), 3000), error);
						if (error)
						{
							std::this_thread::sleep_for(std::chrono::milliseconds(1000));
						}
					}

				}
				catch (std::exception& ex)
				{
					CNC_ERROR(ex.what());
					socket.close();
				}

				std::this_thread::yield();
			}

			CNC_INFO("Network thread exiting");

		});
		
		_impl->stats_thread = std::thread([&] {
			while (_impl->running)
			{
				_impl->bandwidth_in = _impl->input_history.collect_inserted_bytes() / 1024.0f;
				_impl->bandwidth_out = _impl->output_history.collect_inserted_bytes() / 1024.0f;
				std::this_thread::sleep_for(std::chrono::seconds(1));
			}
			CNC_INFO("Stats thread exiting");
		});

		// Gfx
		app::set_framebuffer_srgb(true);


		_tx_background = texture2d::load_from_file("assets/ui_bg.png", texture_format::rgba8, texture_filter_mode::linear);
		_tx_frame = texture2d::load_from_file("assets/ui_frame.png", texture_format::rgba8, texture_filter_mode::linear);
		_tx_volume = texture2d::load_from_file("assets/ui_volume.png", texture_format::rgba8, texture_filter_mode::linear);
		_tx_microphone = texture2d::load_from_file("assets/ui_mic.png", texture_format::rgba8, texture_filter_mode::linear);

		_fb_bloom.create({ texture_format::rgba32f }, _tx_background.get_width(), _tx_background.get_height());
		_fx_bloom = std::make_unique<effects::bloom>();
		_fx_bloom->kick = 0.25f;
		_fx_bloom->threshold = 1.0f;

		_font.load_from_file("assets/cour.ttf");
		
	}

	void voice_chat_scene::draw_screen()
	{
		constexpr auto ws = s_window_size;
		const vec2f vs{ _tx_background.get_width(), _tx_background.get_height() };

		app::reset_context();
		app::viewport({ _tx_background.get_width(), _tx_background.get_height() });
		app::set_projection(s_projection);

		_fb_bloom.bind();

		app::clear(vec4f{ 0.0f, 0.0f, 0.0f, 0.0f });

		if (_state == connection_state::connected)
		{
			// Output Wave
			app::with([&] {

				const float y_base = s_window_size[1] * .55f;

				app::color(colors::blue);

				app::with([&] {
					app::pivot({ 0, 0 });
					app::translate({ 100, y_base + 16.0f, 0 });
					app::draw_text(_font, std::format("{:.2f} Kb/s", _impl->bandwidth_out), 16.0f);
					});

				app::begin(app::primitive_type::line_strip);
				for (auto x : std::views::iota(0u, vs[0]))
				{
					static constexpr auto normalizer = static_cast<float>(std::numeric_limits<i16>::max());
					const float age = x / float(vs[0]);
					const float y = y_base + _impl->output_history.sample(age) / normalizer * s_window_size[1] * .25f;
					app::vertex(vec2f{ x, y });
				}
				app::end();
				});


			// Input Wave
			app::with([&] {

				const float y_base = s_window_size[1] * .7f;

				app::color(colors::red);

				app::with([&] {
					app::pivot({ 0, 0 });
					app::translate({ 100, y_base + 16.0f, 0 });
					app::draw_text(_font, std::format("{:.2f} Kb/s", _impl->bandwidth_in), 16.0f);
					});

				app::begin(app::primitive_type::line_strip);
				for (auto x : std::views::iota(0u, vs[0]))
				{
					static constexpr auto normalizer = static_cast<float>(std::numeric_limits<i16>::max());
					const float age = x / float(vs[0]);
					const float y = s_window_size[1] * .7f + _impl->input_history.sample(age) / normalizer * s_window_size[1] * .25f;
					app::vertex(vec2f{ x, y });
				}
				app::end();
				});

			// Mic
			app::with([&] {

				static constexpr vec2f pos{ 96, ws[1] - 268 };
				static constexpr vec2f slider_pos{ 136, ws[1] - 257 };


				app::pivot({ 0, 0 });
				app::color(_impl->config.input_volume == 0.0f ? vec3f{ .5f } : colors::red);

				if (_ui.button(__LINE__, pos, _tx_microphone)) {
					if (_impl->config.input_volume == 0.0f)
						_impl->config.input_volume = _impl->saved_input_volume;
					else
						_impl->saved_input_volume = std::exchange(_impl->config.input_volume, 0.0f);
				}

				_ui.slider(__LINE__, slider_pos, s_slider_size, { 0, 1 }, _impl->config.input_volume);

				});

			// Speaker
			app::with([&] {
				static constexpr vec2f button_pos{ 96, ws[1] - 310 };
				static constexpr vec2f slider_pos{ 136, ws[1] - 306 };

				app::pivot({ 0, 0 });

				app::color(_impl->config.output_volume == 0.0f ? vec3f{ .5f } : colors::blue);

				if (_ui.button(__LINE__, button_pos, _tx_volume)) {
					if (_impl->config.output_volume == 0.0f)
						_impl->config.output_volume = _impl->saved_output_volume;
					else
						_impl->saved_output_volume = std::exchange(_impl->config.output_volume, 0);
				}

				_ui.slider(__LINE__, slider_pos, s_slider_size, { 0, 1 }, _impl->config.output_volume);

				});

		}
		else
		{
			app::with([&] {
				const auto secs = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch());
				const std::string dots(secs.count() % 3 + 1, '.');
				app::translate(vec3f{ vs / 2.0f, 0.0f });
				app::pivot({ 0.5f, 0.5f });
				app::color(colors::red);
				app::draw_text(_font, std::format("Connecting{}", dots), 16.0f);
			});
		}

		// Bloom
		app::flush();
		_fb_bloom.unbind();

		_fx_bloom->apply(_fb_bloom.get_attachment(0));
	}

	voice_chat_scene::~voice_chat_scene() = default;

	void voice_chat_scene::on_attach()
	{
		_impl = new voice_chat_scene_impl();
		app::set_window_size(s_window_size);
		init();
	}

	void voice_chat_scene::on_update()
	{

		static constexpr auto ws = s_window_size;

		_ui.begin_frame();

		// Window dragging
		vec2f drag_delta;
		if (_ui.window_drag(drag_delta))
			app::set_window_pos(app::get_window_pos() + vec2i{ drag_delta });


		draw_screen();

		app::reset_context();
		app::viewport(ws);
		app::set_projection(s_projection);
		app::clear(vec4f{ 0.0f, 0.0f, 0.0f, 0.0f });


		// Background

		app::with([&] {
			app::color(1.0f);
			app::pivot({ 0, 1 });
			app::texture(_tx_background);
			app::quad({ 0, ws[1] }, { _tx_background.get_width(), _tx_background.get_height() });
			app::no_texture();
			});
		app::flush();


		// Blit wave forms
		glBlendFunc(GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA);
		app::with([&] {
			app::pivot({ 0, 1 });
			app::color(1.0f);
			app::texture(_fx_bloom->get_result());
			app::quad({ 0, ws[1] }, vec2f{ ws });
			app::no_texture();
			});
		app::flush();
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


		

		// Draw frame
		app::with([&] {
			app::color(1.0f);
			app::pivot({ 0, 0 });
			app::texture(_tx_frame);
			app::quad({ 0, 0 }, {_tx_frame.get_width(), _tx_frame.get_height()});
			app::no_texture();
		});

		// Close button
		if (_ui.button_area(__LINE__, { 332, ws[1] - 67 }, { 40, 40 })) {
			app::quit();
		}


		// Flush
		app::flush();

		_ui.end_frame();
	}

	void voice_chat_scene::on_detach()
	{
		ma_device_uninit(&_impl->device);
		_impl->running = false;
		_impl->socket.close();
		_impl->read_thread.join();
		_impl->stats_thread.join();
		delete _impl;
	}

}