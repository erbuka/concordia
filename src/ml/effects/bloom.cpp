#include "bloom.h"

#include <string_view>

namespace ml::effects
{
	static constexpr std::string_view s_bloom_prefilter = R"glsl(
        #version 450 core

        uniform sampler2D uSource;
        uniform float uThreshold;
        uniform float uKick;

        in vec2 vUv;

        out vec4 oColor;

        void main() {
            vec4 smpl = texture(uSource, vUv);
            vec3 color = smpl.rgb * smpl.a;
            float luma = dot(vec3(0.299, 0.587, 0.114), color);
            oColor = vec4(smoothstep(uThreshold - uKick, uThreshold + uKick, luma) * color, 1.0);
        }
	)glsl";


    static constexpr std::string_view s_bloom_downsample = R"glsl(
        #version 450 core

        uniform sampler2D uSource;

        in vec2 vUv;
        out vec4 oColor;

        void main() {
            vec2 s = 1.0 / vec2(textureSize(uSource, 0));

            vec3 tl = texture(uSource, vUv + vec2(-s.x, +s.y)).rgb;
            vec3 tr = texture(uSource, vUv + vec2(+s.x, +s.y)).rgb;
            vec3 bl = texture(uSource, vUv + vec2(-s.x, -s.y)).rgb;
            vec3 br = texture(uSource, vUv + vec2(+s.x, -s.y)).rgb;
      
            oColor = vec4((tl + tr + bl + br) / 4.0,  1.0);
        }
    )glsl";

    static constexpr std::string_view s_bloom_upsample = R"glsl(
        #version 450 core

        uniform sampler2D uPrevious;
        uniform sampler2D uUpsample;

        in vec2 vUv;

        out vec4 oColor;

        void main() {

            vec2 s = 1.0 / vec2(textureSize(uUpsample, 0));

            vec3 upsampleColor = vec3(0.0);

            upsampleColor += 1.0 * texture(uUpsample, vUv + vec2(-s.x, +s.y)).rgb;
            upsampleColor += 2.0 * texture(uUpsample, vUv + vec2(+0.0, +s.y)).rgb;
            upsampleColor += 1.0 * texture(uUpsample, vUv + vec2(+s.x, +s.y)).rgb;
            upsampleColor += 2.0 * texture(uUpsample, vUv + vec2(-s.x, +0.0)).rgb;
            upsampleColor += 4.0 * texture(uUpsample, vUv + vec2(+0.0, +0.0)).rgb;
            upsampleColor += 2.0 * texture(uUpsample, vUv + vec2(+s.x, +0.0)).rgb;
            upsampleColor += 1.0 * texture(uUpsample, vUv + vec2(-s.x, -s.y)).rgb;
            upsampleColor += 2.0 * texture(uUpsample, vUv + vec2(+0.0, -s.y)).rgb;
            upsampleColor += 1.0 * texture(uUpsample, vUv + vec2(+s.x, -s.y)).rgb;
    
            oColor = vec4(upsampleColor / 16.0 + texture(uPrevious, vUv).rgb, 1.0);

        }

    )glsl";

    static constexpr std::string_view s_bloom_combine = R"glsl(
        #version 450 core


        uniform float uExposure;
        uniform sampler2D uColor;
        uniform sampler2D uBloom;

        in vec2 vUv;

        out vec4 oColor;

        void main() {
            vec4 smpl = texture(uColor, vUv);
            vec3 color = smpl.rgb * smpl.a;
			vec3 bloom = texture(uBloom, vUv).rgb;
            vec3 mapped = vec3(1.0) - exp(-(color + bloom) * uExposure);
			oColor = vec4(mapped, 1.0);
        }

    )glsl";


    bloom::bloom()
    {
        _fb_prefilter.create({ texture_format::rgb32f }, 1, 1);
        _fb_combine.create({ texture_format::rgb32f }, 1, 1);

        for(auto& fb : _fb_downsample)
            fb.create({ texture_format::rgb32f }, 1, 1);

        for(auto& fb : _fb_upsample)
            fb.create({ texture_format::rgb32f }, 1, 1);


        _prg_prefiler = shader_program::load(s_bloom_prefilter);
        _prg_downsample = shader_program::load(s_bloom_downsample);
        _prg_upsample = shader_program::load(s_bloom_upsample);
        _prg_combine = shader_program::load(s_bloom_combine);
    }

    texture2d& bloom::apply(const texture2d& src)
    {

        static constexpr auto fullscreen_quad = [] {
            app::with([] {
                app::pivot({ 0.5f, 0.5f });
                app::quad({ 0, 0 }, { 2, 2 });
            });
        };



        const auto w = src.get_width(); 
        const auto h = src.get_height();
        const auto w2 = w / 2;
        const auto h2 = h / 2;

 
        _fb_combine.resize(w, h);
        _fb_prefilter.resize(w2, h2);
            
        for(std::uint32_t i = 0; i < _fb_downsample.size(); ++i)
            _fb_downsample[i].resize(std::max(1u, w2 / (2 << i)), std::max(1u, h2 / (2 << i)));

        for (std::uint32_t i = 0; i < _fb_upsample.size(); ++i)
            _fb_upsample[i].resize(std::max(1u, w2 / (2 << i)), std::max(1u, h2 / (2 << i)));

        app::reset_context();
        
        // Prefilter
        _fb_prefilter.bind();
        app::clear();
        app::use_program(_prg_prefiler);


        _prg_prefiler.uniform("uSource", app::texture(src));
        _prg_prefiler.uniform("uThreshold", threshold);
        _prg_prefiler.uniform("uKick", kick);

        fullscreen_quad();
        app::flush();
        _fb_prefilter.unbind();

        // Copy to downsample
        _fb_downsample[0].bind();
        app::default_program();
        app::clear();
        app::texture(_fb_prefilter.get_attachment(0));
        fullscreen_quad();
        app::flush();
        _fb_downsample[0].unbind();


        // Downsample
        for (const auto i : std::ranges::iota_view(1u, s_passes))
        {
            app::use_program(_prg_downsample);
            _fb_downsample[i].bind();
            app::clear();

            _prg_downsample.uniform("uSource", app::texture(_fb_downsample[i - 1].get_attachment(0)));
            fullscreen_quad();
            app::flush();
            _fb_downsample[i].unbind();

        }
        

        // Upsample
        for (const auto i : std::ranges::iota_view(0u, s_passes - 2) | std::views::reverse)
        {
            app::use_program(_prg_upsample);
            _fb_upsample[i].bind();
            app::clear();

            _prg_upsample.uniform("uPrevious", app::texture(_fb_upsample[i + 1].get_attachment(0)));
            _prg_upsample.uniform("uUpsample", app::texture(_fb_downsample[i].get_attachment(0)));
            fullscreen_quad();
            app::flush();
            _fb_upsample[i].unbind();

        }

        // Combine
        app::use_program(_prg_combine);
        _fb_combine.bind();
        app::clear();
        
        _prg_combine.uniform("uExposure", exposure);
        _prg_combine.uniform("uColor", app::texture(src));
        _prg_combine.uniform("uBloom", app::texture(_fb_upsample[0].get_attachment(0)));
        fullscreen_quad();

        app::flush();
        _fb_combine.unbind();

        
        return _fb_combine.get_attachment(0);
        //return _fb_upsample.get_attachment(0);

    }

    const texture2d& bloom::get_result() const
    {
        return _fb_combine.get_attachment(0);
    }

}