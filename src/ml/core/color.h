#pragma once

#include "vecmath.h"


namespace ml
{

	constexpr vec4f from_abgr(const uint32_t val)
	{
		return {
			static_cast<float>((val >> 0) & 0xFF) / 255.0f,
			static_cast<float>((val >> 8) & 0xFF) / 255.0f,
			static_cast<float>((val >> 16) & 0xFF) / 255.0f,
			static_cast<float>((val >> 24) & 0xFF) / 255.0f
		};
	}


	constexpr vec4f from_rgba(const uint32_t val)
	{
		return {
			static_cast<float>((val >> 24) & 0xFF) / 255.0f,
			static_cast<float>((val >> 16) & 0xFF) / 255.0f,
			static_cast<float>((val >> 8) & 0xFF) / 255.0f,
			static_cast<float>((val >> 0) & 0xFF) / 255.0f
		};
	}

	namespace constants
	{
		static constexpr vec4f red = from_rgba(0xcb5656ff);
		static constexpr vec4f blue = from_rgba(0x569ecbff);
	}



}

