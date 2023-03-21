//#version 420 // Keep it for editor detection


#ifdef VERTEX_SHADER

layout(location = 0) in vec2 POSITION;
layout(location = 1) in vec2 TEXCOORD0;
layout(location = 7) in vec4 COLOR;

// FIXME set the interpolation (don't know what dx do)
// flat means that there is no interpolation. The value given to the fragment shader is based on the provoking vertex conventions.
//
// noperspective means that there will be linear interpolation in window-space. This is usually not what you want, but it can have its uses.
//
// smooth, the default, means to do perspective-correct interpolation.
//
// The centroid qualifier only matters when multisampling. If this qualifier is not present, then the value is interpolated to the pixel's center, anywhere in the pixel, or to one of the pixel's samples. This sample may lie outside of the actual primitive being rendered, since a primitive can cover only part of a pixel's area. The centroid qualifier is used to prevent this; the interpolation point must fall within both the pixel's area and the primitive's area.
out vec4 PSin_p;
out vec2 PSin_t;
out vec4 PSin_c;

void vs_main()
{
    PSin_p = vec4(POSITION, 0.5f, 1.0f);
    PSin_t = TEXCOORD0;
    PSin_c = COLOR;
    gl_Position = vec4(POSITION, 0.5f, 1.0f); // NOTE I don't know if it is possible to merge POSITION_OUT and gl_Position
}

#endif

#ifdef FRAGMENT_SHADER

in vec4 PSin_p;
in vec2 PSin_t;
in vec4 PSin_c;

// Give a different name so I remember there is a special case!
#if defined(ps_main1) || defined(ps_main10)
layout(location = 0) out uint SV_Target1;
#else
layout(location = 0) out vec4 SV_Target0;
#endif

vec4 sample_c()
{
    return texture(TextureSampler, PSin_t);
}

float sample_depth()
{
  return unscale_depth(sample_c().r);
}

vec4 ps_crt(uint i)
{
    vec4 mask[4] = vec4[4]
        (
         vec4(1, 0, 0, 0),
         vec4(0, 1, 0, 0),
         vec4(0, 0, 1, 0),
         vec4(1, 1, 1, 0)
        );
    return sample_c() * clamp((mask[i] + 0.5f), 0.0f, 1.0f);
}

#ifdef ps_main0
void ps_main0()
{
    SV_Target0 = sample_c();
}
#endif

#ifdef ps_main1
void ps_main1()
{
    // Input Color is RGBA8

    // We want to output a pixel on the PSMCT16* format
    // A1-BGR5

#if 0
    // Note: dot is a good idea from pseudo. However we must be careful about float accuraccy.
    // Here a global idea example:
    //
    // SV_Target1 = dot(round(sample_c() * vec4(31.f, 31.f, 31.f, 1.f)), vec4(1.f, 32.f, 1024.f, 32768.f));
    //

    // For me this code is more accurate but it will require some tests

    vec4 c = sample_c() * 255.0f + 0.5f; // Denormalize value to avoid float precision issue

    // shift Red: -3
    // shift Green: -3 + 5
    // shift Blue: -3 + 10
    // shift Alpha: -7 + 15
    highp uvec4 i = uvec4(c * vec4(1/8.0f, 4.0f, 128.0f, 256.0f)); // Shift value

    // bit field operation requires GL4 HW. Could be nice to merge it with step/mix below
    SV_Target1 = (i.r & uint(0x001f)) | (i.g & uint(0x03e0)) | (i.b & uint(0x7c00)) | (i.a & uint(0x8000));

#else
    // Old code which is likely wrong.

    vec4 c = sample_c();

    c.a *= 256.0f / 127.0f; // hm, 0.5 won't give us 1.0 if we just multiply with 2

    highp uvec4 i = uvec4(c * vec4(uint(0x001f), uint(0x03e0), uint(0x7c00), uint(0x8000)));

    // bit field operation requires GL4 HW.
    SV_Target1 = (i.x & uint(0x001f)) | (i.y & uint(0x03e0)) | (i.z & uint(0x7c00)) | (i.w & uint(0x8000));
#endif


}
#endif

#ifdef ps_main10
void ps_main10()
{
    // Convert a GL_FLOAT32 depth texture into a 32 bits UINT texture
#if HAS_CLIP_CONTROL
    SV_Target1 = uint(exp2(32.0f) * sample_c().r);
#else
    SV_Target1 = uint(exp2(24.0f) * sample_c().r);
#endif
}
#endif

#ifdef ps_main11
void ps_main11()
{
    // Convert a GL_FLOAT32 depth texture into a RGBA color texture
    const vec4 bitSh = vec4(exp2(24.0f), exp2(16.0f), exp2(8.0f), exp2(0.0f));
    const vec4 bitMsk = vec4(0.0, 1.0/256.0, 1.0/256.0, 1.0/256.0);

    vec4 res = fract(vec4(sample_depth()) * bitSh);

    SV_Target0 = (res - res.xxyz * bitMsk) * 256.0f/255.0f;
}
#endif

#ifdef ps_main12
void ps_main12()
{
    // Convert a GL_FLOAT32 (only 16 lsb) depth into a RGB5A1 color texture
    const vec4 bitSh = vec4(exp2(32.0f), exp2(27.0f), exp2(22.0f), exp2(17.0f));
    const uvec4 bitMsk = uvec4(0x1F, 0x1F, 0x1F, 0x1);
    uvec4 color = uvec4(vec4(sample_depth()) * bitSh) & bitMsk;

    SV_Target0 = vec4(color) / vec4(32.0f, 32.0f, 32.0f, 1.0f);
}
#endif

#ifdef ps_main13
void ps_main13()
{
    // Convert a RRGBA texture into a float depth texture
    // FIXME: I'm afraid of the accuracy
    const vec4 bitSh = vec4(exp2(-32.0f), exp2(-24.0f), exp2(-16.0f), exp2(-8.0f)) * vec4(255.0);
    gl_FragDepth = scale_depth(dot(sample_c(), bitSh));
}
#endif

#ifdef ps_main14
void ps_main14()
{
    // Same as above but without the alpha channel (24 bits Z)

    // Convert a RRGBA texture into a float depth texture
    // FIXME: I'm afraid of the accuracy
    const vec3 bitSh = vec3(exp2(-32.0f), exp2(-24.0f), exp2(-16.0f)) * vec3(255.0);
    gl_FragDepth = scale_depth(dot(sample_c().rgb, bitSh));
}
#endif

#ifdef ps_main15
void ps_main15()
{
    // Same as above but without the A/B channels (16 bits Z)

    // Convert a RRGBA texture into a float depth texture
    // FIXME: I'm afraid of the accuracy
    const vec2 bitSh = vec2(exp2(-32.0f), exp2(-24.0f)) * vec2(255.0);
    gl_FragDepth = scale_depth(dot(sample_c().rg, bitSh));
}
#endif

#ifdef ps_main16
void ps_main16()
{
    // Convert a RGB5A1 (saved as RGBA8) color to a 16 bit Z
    // FIXME: I'm afraid of the accuracy
    const vec4 bitSh = vec4(exp2(-32.0f), exp2(-27.0f), exp2(-22.0f), exp2(-17.0f));
    // Trunc color to drop useless lsb
    vec4 color = trunc(sample_c() * vec4(255.0f) / vec4(8.0f, 8.0f, 8.0f, 128.0f));
    gl_FragDepth = scale_depth(dot(vec4(color), bitSh));
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

    txN *= ScalingFactor.x;
    txH *= ScalingFactor.x;
    ty  *= ScalingFactor.y;

    // TODO investigate texture gather
    vec4 cN = texelFetch(TextureSampler, ivec2(txN, ty), 0);
    vec4 cH = texelFetch(TextureSampler, ivec2(txH, ty), 0);


    if ((sel.y & 4u) == 0u) {
        // Column 0 and 2
#ifdef ONLY_BLUE
        c = cN.b;
#else
        if ((sel.y & 3u) < 2u) {
            // first 2 lines of the col
            if (sel.x < 8u)
                c = cN.r;
            else
                c = cN.b;
        } else {
            if (sel.x < 8u)
                c = cH.g;
            else
                c = cH.a;
        }
#endif
    } else {
#ifdef ONLY_BLUE
        c = cH.b;
#else
        // Column 1 and 3
        if ((sel.y & 3u) < 2u) {
            // first 2 lines of the col
            if (sel.x < 8u)
                c = cH.r;
            else
                c = cH.b;
        } else {
            if (sel.x < 8u)
                c = cN.g;
            else
                c = cN.a;
        }
#endif
    }


    SV_Target0 = vec4(c);
}
#endif

#ifdef ps_main19
void ps_main19()
{
    SV_Target0 = PSin_c * vec4(1.0, 1.0, 1.0, sample_c().r);
}
#endif

#ifdef ps_main7
void ps_main7()
{
    vec4 c = sample_c();

    c.a = dot(c.rgb, vec3(0.299, 0.587, 0.114));

    SV_Target0 = c;
}
#endif

#ifdef ps_main5
vec4 ps_scanlines(uint i)
{
    vec4 mask[2] = vec4[2]
    (
        vec4(1, 1, 1, 0),
        vec4(0, 0, 0, 0)
    );

    return sample_c() * clamp((mask[i] + 0.5f), 0.0f, 1.0f);
}

void ps_main5() // scanlines
{
    highp uvec4 p = uvec4(gl_FragCoord);

    vec4 c = ps_scanlines(p.y % 2u);

    SV_Target0 = c;
}
#endif

#ifdef ps_main6
void ps_main6() // diagonal
{
    highp uvec4 p = uvec4(gl_FragCoord);

    vec4 c = ps_crt((p.x + (p.y % 3u)) % 3u);

    SV_Target0 = c;
}
#endif

#ifdef ps_main8
void ps_main8() // triangular
{
    highp uvec4 p = uvec4(gl_FragCoord);

    vec4 c = ps_crt(((p.x + ((p.y >> 1u) & 1u) * 3u) >> 1u) % 3u);

    SV_Target0 = c;
}
#endif

#ifdef ps_main9
void ps_main9()
{

    const float PI = 3.14159265359f;

    vec2 texdim = vec2(textureSize(TextureSampler, 0));

    vec4 c;
    if (dFdy(PSin_t.y) * PSin_t.y > 0.5f) {
        c = sample_c();
    } else {
        float factor = (0.9f - 0.4f * cos(2.0f * PI * PSin_t.y * texdim.y));
        c =  factor * texture(TextureSampler, vec2(PSin_t.x, (floor(PSin_t.y * texdim.y) + 0.5f) / texdim.y));
    }

    SV_Target0 = c;
}
#endif

// Used for DATE (stencil)
// DATM == 1
#ifdef ps_main2
void ps_main2()
{
    if(sample_c().a < (127.5f / 255.0f)) // >= 0x80 pass
        discard;
}
#endif

// Used for DATE (stencil)
// DATM == 0
#ifdef ps_main3
void ps_main3()
{
    if((127.5f / 255.0f) < sample_c().a) // < 0x80 pass (== 0x80 should not pass)
        discard;
}
#endif

#ifdef ps_main4
void ps_main4()
{
    SV_Target0 = mod(round(sample_c() * 255.0f), 256.0f) / 255.0f;
}
#endif

#ifdef ps_main18
void ps_main18()
{
    vec4 i = sample_c();
    vec4 o;

    mat3 rgb2yuv; // Value from GS manual
    rgb2yuv[0] = vec3(0.587, -0.311, -0.419);
    rgb2yuv[1] = vec3(0.114, 0.500, -0.081);
    rgb2yuv[2] = vec3(0.299, -0.169, 0.500);

    vec3 yuv = rgb2yuv * i.gbr;

    float Y = float(0xDB)/255.0f * yuv.x + float(0x10)/255.0f;
    float Cr = float(0xE0)/255.0f * yuv.y + float(0x80)/255.0f;
    float Cb = float(0xE0)/255.0f * yuv.z + float(0x80)/255.0f;

    switch(EMODA) {
        case 0:
            o.a = i.a;
            break;
        case 1:
            o.a = Y;
            break;
        case 2:
            o.a = Y/2.0f;
            break;
        case 3:
            o.a = 0.0f;
            break;
    }

    switch(EMODC) {
        case 0:
            o.rgb = i.rgb;
            break;
        case 1:
            o.rgb = vec3(Y);
            break;
        case 2:
            o.rgb = vec3(Y, Cb, Cr);
            break;
        case 3:
            o.rgb = vec3(i.a);
            break;
    }

    SV_Target0 = o;
}
#endif

#endif
