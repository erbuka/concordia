
#include <core/application.h>

#include "client.h"

int main() 
{
	ml::app::goto_scene(std::make_shared<cnc::voice_chat_scene>());
	return ml::app::run({ 
		.transparent = true,
		.decorated = false,
		.resizable = false
	});
}