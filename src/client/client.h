#pragma once

#include <core/scene.h>
#include <core/texture.h>

#include "ui.h"

namespace cnc
{
	struct voice_chat_scene_impl;

	struct voice_chat_config
	{
		float input_volume{ 1.0f };
	};

	class voice_chat_scene: public ml::scene
	{
	public:
		~voice_chat_scene();
		void on_attach() override;
		void on_update() override;
		void on_detach() override;
	private:
		voice_chat_scene_impl* _impl;
		ui _ui;

		texture2d _tx_background;

		void init();
	};
}