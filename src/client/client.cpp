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

	static constexpr vec2i s_window_size = { 420, 420 };
	static constexpr vec2i s_frame_padding = { 10, 10 };
	static constexpr auto s_projection = ortho<float>(0, s_window_size[0], 0, s_window_size[0]);


	class history_buffer
	{
	private:
		static constexpr std::size_t buffer_count = 50;
		static constexpr std::size_t size_in_samples = buffer_size * buffer_count;
		std::size_t _current_idx = 0;
		std::array<sample_t, size_in_samples> _buffer{};
		
		std::size_t wrap(const std::size_t i) const { return i % size_in_samples; }
		
	public:
		using value_type = sample_t;

		class iterator_sentinel {};

		class iterator
		{
		private:
			history_buffer& _owner;
			std::size_t _start, _offset;
		public:
			iterator(history_buffer& owner, const std::size_t start, const std::size_t offset) : 
				_start(start), _owner(owner), _offset(offset) {}

			sample_t& operator*() { return _owner.get(_start + _offset); }

			iterator& operator++() {
				++_offset;
				return *this;
			}

			bool operator==(iterator_sentinel) const { return _offset == size_in_samples; }
			bool operator!=(iterator_sentinel) const { return _offset != size_in_samples; }

		};

		sample_t& operator[](const std::size_t idx) { return get(idx); }
		const sample_t& operator[](const std::size_t idx) const { return get(idx); }

		sample_t& get(const std::size_t idx) { return _buffer[wrap(idx + _current_idx)]; }
		const sample_t& get(const std::size_t idx) const { return _buffer[wrap(idx + _current_idx)]; }

		auto begin() { return iterator{ *this, _current_idx, 0 }; }
		auto end() { return iterator_sentinel{}; }

		void push_back(const sample_t s) 
		{ 
			get(_current_idx) = s; 
			_current_idx = wrap(_current_idx + 1);
		}

		sample_t sample(const float age) const 
		{ 
			const std::size_t start = ((_current_idx / buffer_size + 1) * buffer_size) % size_in_samples;
			return get(start + static_cast<std::size_t>(age * size_in_samples)); 
		}

	};

	struct voice_chat_scene_impl 
	{


		static constexpr std::size_t network_buffer_count = 20;

		voice_chat_config config;
		ma_device device{};
		asio::io_context ctx{};
		tcp::socket socket;
		std::thread read_thread;


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

		if (ma_device_init(NULL, &config, &_impl->device) != MA_SUCCESS) {
			throw std::runtime_error("Can't initialize audio device!");
		}

		ma_device_start(&_impl->device);     // The device is sleeping by default so you'll need to start it manually.


		// Connection
		_impl->socket.connect(tcp::endpoint(asio::ip::address_v4::from_string("127.0.0.1"), 3000));

		_impl->read_thread = std::thread([&] {

			buffer_t buffer;

			auto& socket = _impl->socket;
			auto& incoming_audio = _impl->incoming_audio;

			while (socket.is_open())
			{
				try {
					


					const auto bytes_read = asio::read(socket, asio::buffer(buffer));

					std::ranges::copy(buffer, std::back_inserter(_impl->output_history));

					if (bytes_read > 0)
					{
						auto [lock, res] = incoming_audio.get();
						std::ranges::copy(buffer, std::back_inserter(res));
					}

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
		

		// Gfx
		app::set_framebuffer_srgb(true);

		_tx_background = texture2d::load_from_file("assets/ui_bg.png", texture_format::rgba8, texture_filter_mode::linear);
		_tx_frame = texture2d::load_from_file("assets/ui_frame.png", texture_format::rgba8, texture_filter_mode::linear);

		_fb_bloom.create({ texture_format::rgba32f }, s_window_size[0], s_window_size[1]);
		_fx_bloom = std::make_unique<effects::bloom>();
		_fx_bloom->kick = 0.25f;
		_fx_bloom->threshold = 1.0f;

	}

	void voice_chat_scene::draw_wave_forms()
	{
		app::reset_context();
		app::viewport(s_window_size - s_frame_padding);
		app::set_projection(s_projection);

		_fb_bloom.bind();

		app::clear(vec4f{ 0.0f, 0.0f, 0.0f, 0.0f });

		// Wave form
		app::with([&] {
			app::color(colors::blue);
			app::begin(app::primitive_type::line_strip);
			for (auto x : std::views::iota(0, s_window_size[0]))
			{
				static constexpr auto normalizer = static_cast<float>(std::numeric_limits<i16>::max());
				const float age = x / float(s_window_size[0]);
				//const float y = s_window_size[1] * .45f + _impl->sample_incoming_audio(x / float(s_window_size[0])) / normalizer * s_window_size[1] * .5f;
				const float y = s_window_size[1] * .4f + _impl->output_history.sample(age) / normalizer * s_window_size[1] * .5f;
				app::vertex(vec2f{ x, y });
			}
			app::end();
			});



		// Wave form
		app::with([&] {
			app::color(colors::red);
			app::begin(app::primitive_type::line_strip);
			for (auto x : std::views::iota(0, s_window_size[0]))
			{
				static constexpr auto normalizer = static_cast<float>(std::numeric_limits<i16>::max());
				const float age = x / float(s_window_size[0]);
				const float y = s_window_size[1] * .6f + _impl->input_history.sample(age) / normalizer * s_window_size[1] * .5f;
				app::vertex(vec2f{ x, y });
			}
			app::end();
			});

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

		_ui.capture_input();

		// Window dragging
		vec2f drag_delta;
		if (_ui.window_drag(drag_delta))
			app::set_window_pos(app::get_window_pos() + vec2i{ drag_delta });

		draw_wave_forms();

		app::reset_context();
		app::viewport(ws);
		app::set_projection(s_projection);
		app::clear(vec4f{ 0.0f, 0.0f, 0.0f, 0.0f });

		// Background
		
		app::with([&] {
			app::color(1.0f);
			app::pivot({ 0, 0 });
			app::texture(_tx_background);
			app::quad({ 0, ws[1] - _tx_background.get_height() }, {_tx_background.get_width(), _tx_background.get_height()});
			app::no_texture();
		});
		app::flush();
		

		// Blit wave forms
		glBlendFunc(GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA);
		app::with([&] {
			app::pivot({ 0, 0 });
			app::color(1.0f);
			app::texture(_fx_bloom->get_result());
			app::quad({ 0, 0 }, vec2f{ ws });
			app::no_texture();
		});
		app::flush();
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		

		// Mute button
		app::color(_impl->config.input_volume == 0.0f ? vec3f{ 1.0f, 0.0f, 0.0f } : vec3f{ 0.0f, 1.0f, 0.0f });
		if (_ui.round_button(1, { ws[0] / 2.0f, ws[1] / 4.0f }, 50)) {
			_impl->config.input_volume = (1.0f - _impl->config.input_volume);
		}


		// Draw frame
		app::with([&] {
			app::color(1.0f);
			app::pivot({ 0, 0 });
			app::texture(_tx_frame);
			app::quad({ 0, ws[1] - _tx_frame.get_height() }, {_tx_frame.get_width(), _tx_frame.get_height()});
			app::no_texture();
		});

		// Flush
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