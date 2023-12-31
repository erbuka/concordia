#pragma once

#include <memory>

#include <core/font.h>
#include <core/scene.h>
#include <core/texture.h>
#include <effects/bloom.h>

#include "ui.h"

namespace cnc
{
	struct voice_chat_scene_impl;

	struct voice_chat_config
	{
		std::string host;
		u32 port;
		float input_volume{ 1.0f };
		float output_volume{ 1.0f };
	};

	enum class connection_state 
	{
		disconnected, connected
	};

	class voice_chat_scene: public ml::scene
	{
	public:
		voice_chat_scene(voice_chat_config cfg);
		~voice_chat_scene();
		void on_attach() override;
		void on_update() override;
		void on_detach() override;
	private:
		voice_chat_config _config;
		voice_chat_scene_impl* _impl;
		ui _ui;
		
		connection_state _state{ connection_state::disconnected };

		font _font;
		texture2d _tx_background, _tx_frame, _tx_volume, _tx_microphone;
		framebuffer _fb_bloom;
		std::unique_ptr<effects::bloom> _fx_bloom;

		void init();
		void draw_screen();

	};
}