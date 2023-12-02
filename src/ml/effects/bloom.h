#pragma once

#include "core/application.h"

namespace ml::effects
{
	class bloom
	{
	private:

		static constexpr std::uint32_t s_passes = 8;

		
		framebuffer _fb_prefilter, _fb_combine;
		std::array<framebuffer, s_passes> _fb_downsample;
		std::array<framebuffer, s_passes - 1> _fb_upsample;
		shader_program _prg_prefiler, _prg_downsample, _prg_upsample, _prg_combine;
	public:
		
		float exposure{ 1.0f };
		float threshold{ 1.0f };
		float kick{ 0.1f };

		bloom();

		texture2d& apply(const texture2d& src);

		const texture2d& get_result() const;

	};


}