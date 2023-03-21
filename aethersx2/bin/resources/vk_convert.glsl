#ifndef PS_SCALE_FACTOR
#define PS_SCALE_FACTOR 1
#endif

#ifdef VERTEX_SHADER

layout(location = 0) in vec4 a_pos;
layout(location = 1) in vec2 a_tex;
layout(location = 2) in vec4 a_color;

layout(location = 0) out vec2 v_tex;
layout(location = 1) out vec4 v_color;

void main()
{
	gl_Position = vec4(a_pos.x, -a_pos.y, a_pos.z, a_pos.w);
	v_tex = a_tex;
	v_color = a_color;
}

#endif

#ifdef FRAGMENT_SHADER

layout(location = 0) in vec2 v_tex;
layout(location = 1) in vec4 v_color;

#if defined(ps_main1) || defined(ps_main10)
layout(location = 0) out uint o_col0;
#elif !defined(ps_main13) && !defined(ps_main14) && !defined(ps_main15) && !defined(ps_main16)
layout(location = 0) out vec4 o_col0;
#endif

layout(set = 0, binding = 0) uniform sampler2D samp0;

vec4 sample_c(vec2 uv)
{
	return texture(samp0, uv);
}

#ifdef ps_main0
void ps_main0()
{
	o_col0 = sample_c(v_tex);
}
#endif

#ifdef ps_main7
void ps_main7()
{
	vec4 c = sample_c(v_tex);
	
	c.a = dot(c.rgb, vec3(0.299, 0.587, 0.114));

	o_col0 = c;
}
#endif

vec4 ps_crt(uint i)
{
		vec4 mask[4] = vec4[4]
				(
				 vec4(1, 0, 0, 0),
				 vec4(0, 1, 0, 0),
				 vec4(0, 0, 1, 0),
				 vec4(1, 1, 1, 0)
				);
		return sample_c(v_tex) * clamp((mask[i] + 0.5f), 0.0f, 1.0f);
}

vec4 ps_scanlines(uint i)
{
		vec4 mask[2] =
		{
				vec4(1, 1, 1, 0),
				vec4(0, 0, 0, 0)
		};

		return sample_c(v_tex) * clamp((mask[i] + 0.5f), 0.0f, 1.0f);
}

#ifdef ps_main1
void ps_main1()
{
	vec4 c = sample_c(v_tex);

	c.a *= 256.0f / 127; // hm, 0.5 won't give us 1.0 if we just multiply with 2

	uvec4 i = uvec4(c * vec4(0x001f, 0x03e0, 0x7c00, 0x8000));

	o_col0 = (i.x & 0x001f) | (i.y & 0x03e0) | (i.z & 0x7c00) | (i.w & 0x8000);	
}
#endif

#ifdef ps_main2
void ps_main2()
{
	o_col0 = vec4(0, 0, 0, 0);

	if(sample_c(v_tex).a < (127.5f / 255.0f)) // >= 0x80 pass
		discard;

}
#endif

#ifdef ps_main3
void ps_main3()
{
	o_col0 = vec4(0, 0, 0, 0);

	if((127.5f / 255.0f) < sample_c(v_tex).a) // < 0x80 pass (== 0x80 should not pass)
		discard;
}
#endif

#ifdef ps_main4
void ps_main4()
{
	vec4 c = roundEven(sample_c(v_tex) * 255);
	// We use 2 fmod to avoid negative value.
	vec4 fmod1 = mod(c, 256) + 256;
	vec4 fmod2 = mod(fmod1, 256);

	o_col0 = fmod2 / 255.0f;
}
#endif

#ifdef ps_main5
void ps_main5() // scanlines
{
	uvec4 p = uvec4(gl_FragCoord);

	o_col0 = ps_scanlines(p.y % 2);
}
#endif

#ifdef ps_main6
void ps_main6() // diagonal
{
	uvec4 p = uvec4(gl_FragCoord);
	o_col0 = ps_crt((p.x + (p.y % 3)) % 3);
}
#endif

#ifdef ps_main8
void ps_main8() // triangular
{
	uvec4 p = uvec4(gl_FragCoord);

	// output.c = ps_crt(input, ((p.x + (p.y & 1) * 3) >> 1) % 3); 
	o_col0 = ps_crt(((p.x + ((p.y >> 1) & 1) * 3) >> 1) % 3);
}
#endif

#ifdef ps_main9
void ps_main9() // triangular
{
	const float PI = 3.14159265359f;
	vec2 texdim = vec2(textureSize(samp0, 0));
	if (dFdy(v_tex.y) * texdim.y > 0.5) 
		o_col0 = sample_c(v_tex); 
	else
		o_col0 = (0.9 - 0.4 * cos(2 * PI * v_tex.y * texdim.y)) * sample_c(vec2(v_tex.x, (floor(v_tex.y * texdim.y) + 0.5) / texdim.y));
}
#endif

#ifdef ps_main10
void ps_main10()
{
	// Convert a vec32 depth texture into a 32 bits UINT texture
	o_col0 = uint(exp2(32.0f) * sample_c(v_tex).r);
}
#endif

#ifdef ps_main11
void ps_main11()
{
	// Convert a vec32 depth texture into a RGBA color texture
	const vec4 bitSh = vec4(exp2(24.0f), exp2(16.0f), exp2(8.0f), exp2(0.0f));
	const vec4 bitMsk = vec4(0.0, 1.0 / 256.0, 1.0 / 256.0, 1.0 / 256.0);

	vec4 res = fract(vec4(sample_c(v_tex).rrrr) * bitSh);

	o_col0 = (res - res.xxyz * bitMsk) * 256.0f / 255.0f;
}
#endif

#ifdef ps_main12
void ps_main12()
{
	// Convert a vec32 (only 16 lsb) depth into a RGB5A1 color texture
	const vec4 bitSh = vec4(exp2(32.0f), exp2(27.0f), exp2(22.0f), exp2(17.0f));
	const uvec4 bitMsk = uvec4(0x1F, 0x1F, 0x1F, 0x1);
	uvec4 color = uvec4(vec4(sample_c(v_tex).rrrr) * bitSh) & bitMsk;

	o_col0 = vec4(color) / vec4(32.0f, 32.0f, 32.0f, 1.0f);
}
#endif

#ifdef ps_main13
void ps_main13()
{
	// Convert a RRGBA texture into a float depth texture
	// FIXME: I'm afraid of the accuracy
	const vec4 bitSh = vec4(exp2(-32.0f), exp2(-24.0f), exp2(-16.0f), exp2(-8.0f)) * vec4(255.0);

	gl_FragDepth = dot(sample_c(v_tex), bitSh);
}
#endif

#ifdef ps_main14
void ps_main14()
{
	// Same as above but without the alpha channel (24 bits Z)

	// Convert a RRGBA texture into a float depth texture
	const vec3 bitSh = vec3(exp2(-32.0f), exp2(-24.0f), exp2(-16.0f)) * vec3(255.0);

	gl_FragDepth = dot(sample_c(v_tex).rgb, bitSh);
}
#endif

#ifdef ps_main15
void ps_main15()
{
	// Same as above but without the A/B channels (16 bits Z)

	// Convert a RRGBA texture into a float depth texture
	// FIXME: I'm afraid of the accuracy
	const vec2 bitSh = vec2(exp2(-32.0f), exp2(-24.0f)) * vec2(255.0);

	gl_FragDepth = dot(sample_c(v_tex).rg, bitSh);
}
#endif

#ifdef ps_main16
void ps_main16()
{
	// Convert a RGB5A1 (saved as RGBA8) color to a 16 bit Z
	// FIXME: I'm afraid of the accuracy
	const vec4 bitSh = vec4(exp2(-32.0f), exp2(-27.0f), exp2(-22.0f), exp2(-17.0f));
	// Trunc color to drop useless lsb
	vec4 color = trunc(sample_c(v_tex) * vec4(255.0f) / vec4(8.0f, 8.0f, 8.0f, 128.0f));

	gl_FragDepth = dot(vec4(color), bitSh);
}
#endif

#ifdef ps_main17
void ps_main17()
{
	// Potential speed optimization. There is a high probability that
	// game only want to extract a single channel (blue). It will allow
	// to remove most of the conditional operation and yield a +2/3 fps
	// boost on MGS3
	//
	// Hypothesis wrong in Prince of Persia ... Seriously WTF !
	//#define ONLY_BLUE;

	// Convert a RGBA texture into a 8 bits packed texture
	// Input column: 8x2 RGBA pixels
	// 0: 8 RGBA
	// 1: 8 RGBA
	// Output column: 16x4 Index pixels
	// 0: 8 R | 8 B
	// 1: 8 R | 8 B
	// 2: 8 G | 8 A
	// 3: 8 G | 8 A
	float c;

	uvec2 sel = uvec2(gl_FragCoord.xy) % uvec2(16u, 16u);
	ivec2 tb  = ((ivec2(gl_FragCoord.xy) & ~ivec2(15, 3)) >> 1);

	int ty   = tb.y | (int(gl_FragCoord.y) & 1);
	int txN  = tb.x | (int(gl_FragCoord.x) & 7);
	int txH  = tb.x | ((int(gl_FragCoord.x) + 4) & 7);

	txN *= PS_SCALE_FACTOR;
	txH *= PS_SCALE_FACTOR;
	ty  *= PS_SCALE_FACTOR;

	// TODO investigate texture gather
	vec4 cN = texelFetch(samp0, ivec2(txN, ty), 0);
	vec4 cH = texelFetch(samp0, ivec2(txH, ty), 0);


	if ((sel.y & 4u) == 0u)
	{
#ifdef ONLY_BLUE
		c = cN.b;
#else
		// Column 0 and 2
		if ((sel.y & 3u) < 2u)
		{
			// First 2 lines of the col
			if (sel.x < 8u)
				c = cN.r;
			else
				c = cN.b;
		}
		else
		{
			if (sel.x < 8u)
				c = cH.g;
			else
				c = cH.a;
		}
#endif
	}
	else
	{
#ifdef ONLY_BLUE
		c = cH.b;
#else
		// Column 1 and 3
		if ((sel.y & 3u) < 2u)
		{
			// First 2 lines of the col
			if (sel.x < 8u)
				c = cH.r;
			else
				c = cH.b;
		}
		else
		{
			if (sel.x < 8u)
				c = cN.g;
			else
				c = cN.a;
		}
#endif
	}

	o_col0 = vec4(c); // Divide by something here?
}
#endif

// DUMMY
#ifdef ps_main18
void ps_main18()
{
	o_col0 = gl_FragCoord;
}
#endif

#ifdef ps_main19
void ps_main19()
{
	o_col0 = v_color * vec4(1.0, 1.0, 1.0, sample_c(v_tex).r);
}
#endif

#endif
