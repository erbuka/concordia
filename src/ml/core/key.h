#pragma once

#include <cinttypes>
#include <string_view>

namespace ml
{
    enum class key: std::uint32_t {
        none,
        enter,
        backspace,
        escape,
        left,
        right,
        up,
        down,
        zero,
        one,
        two,
        three,
        four,
        five,
        six,
        seven,
        eight,
        nine,
        a,
        b,
        c,
        d,
        e,
        f,
        g,
        h,
        i,
        j,
        k,
        l,
        m,
        n,
        o,
        p,
        q,
        r,
        s,
        t,
        u,
        v,
        w,
        x,
        y,
        z
    };

	enum class key_state
	{
		pressed, released
	};

	std::string_view to_string(key k);

}