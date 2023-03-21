/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2021 PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "common/GL/Context.h"
#include "common/GL/StreamBuffer.h"
#include "common/GL/Program.h"
#include "common/GL/ShaderCache.h"
#include "common/HashCombine.h"
#include "GS/Renderers/Common/GSDevice.h"
#include "GSTextureOGL.h"
#include "GS/GS.h"
#include "GSUniformBufferOGL.h"
#include "GSShaderOGL.h"
#include "GLState.h"

#ifdef ENABLE_OGL_DEBUG_MEM_BW
extern uint64 g_real_texture_upload_byte;
extern uint64 g_vertex_upload_byte;
#endif

class GSDepthStencilOGL
{
	bool m_depth_enable;
	GLenum m_depth_func;
	bool m_depth_mask;
	// Note front face and back might be split but it seems they have same parameter configuration
	bool m_stencil_enable;
	GLenum m_stencil_func;
	GLenum m_stencil_spass_dpass_op;

public:
	GSDepthStencilOGL()
		: m_depth_enable(false)
		, m_depth_func(GL_ALWAYS)
		, m_depth_mask(0)
		, m_stencil_enable(false)
		, m_stencil_func(0)
		, m_stencil_spass_dpass_op(GL_KEEP)
	{
	}

	void EnableDepth() { m_depth_enable = true; }
	void EnableStencil() { m_stencil_enable = true; }

	void SetDepth(GLenum func, bool mask)
	{
		m_depth_func = func;
		m_depth_mask = mask;
	}
	void SetStencil(GLenum func, GLenum pass)
	{
		m_stencil_func = func;
		m_stencil_spass_dpass_op = pass;
	}

	void SetupDepth()
	{
		if (GLState::depth != m_depth_enable)
		{
			GLState::depth = m_depth_enable;
			if (m_depth_enable)
				glEnable(GL_DEPTH_TEST);
			else
				glDisable(GL_DEPTH_TEST);
		}

		if (m_depth_enable)
		{
			if (GLState::depth_func != m_depth_func)
			{
				GLState::depth_func = m_depth_func;
				glDepthFunc(m_depth_func);
			}
			if (GLState::depth_mask != m_depth_mask)
			{
				GLState::depth_mask = m_depth_mask;
				glDepthMask((GLboolean)m_depth_mask);
			}
		}
	}

	void SetupStencil()
	{
		if (GLState::stencil != m_stencil_enable)
		{
			GLState::stencil = m_stencil_enable;
			if (m_stencil_enable)
				glEnable(GL_STENCIL_TEST);
			else
				glDisable(GL_STENCIL_TEST);
		}

		if (m_stencil_enable)
		{
			// Note: here the mask control which bitplane is considered by the operation
			if (GLState::stencil_func != m_stencil_func)
			{
				GLState::stencil_func = m_stencil_func;
				glStencilFunc(m_stencil_func, 1, 1);
			}
			if (GLState::stencil_pass != m_stencil_spass_dpass_op)
			{
				GLState::stencil_pass = m_stencil_spass_dpass_op;
				glStencilOp(GL_KEEP, GL_KEEP, m_stencil_spass_dpass_op);
			}
		}
	}

	bool IsMaskEnable() { return m_depth_mask != GL_FALSE; }
};

class GSDeviceOGL final : public GSDevice
{
public:
	struct alignas(32) VSConstantBuffer
	{
		GSVector4 Vertex_Scale_Offset;

		GSVector4 TextureOffset;

		GSVector2 PointSize;
		GSVector2i MaxDepth;

		VSConstantBuffer()
		{
			Vertex_Scale_Offset = GSVector4::zero();
			TextureOffset       = GSVector4::zero();
			PointSize           = GSVector2(0);
			MaxDepth            = GSVector2i(0);
		}

		__forceinline bool Update(const VSConstantBuffer* cb)
		{
			GSVector4i* a = (GSVector4i*)this;
			GSVector4i* b = (GSVector4i*)cb;

			if (!((a[0] == b[0]) & (a[1] == b[1]) & (a[2] == b[2])).alltrue())
			{
				a[0] = b[0];
				a[1] = b[1];
				a[2] = b[2];

				return true;
			}

			return false;
		}
	};

	struct VSSelector
	{
		union
		{
			struct
			{
				uint32 int_fst : 1;
				uint32 _free : 31;
			};

			uint32 key;
		};

		operator uint32() const { return key; }

		VSSelector()
			: key(0)
		{
		}
		VSSelector(uint32 k)
			: key(k)
		{
		}
	};

	struct GSSelector
	{
		union
		{
			struct
			{
				uint32 sprite : 1;
				uint32 point  : 1;
				uint32 line   : 1;

				uint32 _free : 29;
			};

			uint32 key;
		};

		operator uint32() const { return key; }

		GSSelector()
			: key(0)
		{
		}
		GSSelector(uint32 k)
			: key(k)
		{
		}
	};

	struct alignas(32) PSConstantBuffer
	{
		GSVector4 FogColor_AREF;
		GSVector4 WH;
		GSVector4 TA_Af;
		GSVector4i MskFix;
		GSVector4i FbMask;

		GSVector4 HalfTexel;
		GSVector4 MinMax;
		GSVector4 TC_OH_TS;
		GSVector4 MaxDepth;

		GSVector4 DitherMatrix[4];

		PSConstantBuffer()
		{
			FogColor_AREF = GSVector4::zero();
			HalfTexel     = GSVector4::zero();
			WH            = GSVector4::zero();
			TA_Af         = GSVector4::zero();
			MinMax        = GSVector4::zero();
			MskFix        = GSVector4i::zero();
			TC_OH_TS      = GSVector4::zero();
			FbMask        = GSVector4i::zero();
			MaxDepth      = GSVector4::zero();

			DitherMatrix[0] = GSVector4::zero();
			DitherMatrix[1] = GSVector4::zero();
			DitherMatrix[2] = GSVector4::zero();
			DitherMatrix[3] = GSVector4::zero();
		}

		__forceinline bool Update(const PSConstantBuffer* cb)
		{
			GSVector4i* a = (GSVector4i*)this;
			GSVector4i* b = (GSVector4i*)cb;

			// if WH matches both HalfTexel and TC_OH_TS do too
			if (!((a[0] == b[0]) & (a[1] == b[1]) & (a[2] == b[2]) & (a[3] == b[3]) & (a[4] == b[4]) & (a[6] == b[6])
				& (a[8] == b[8]) & (a[9] == b[9]) & (a[10] == b[10]) & (a[11] == b[11]) & (a[12] == b[12])).alltrue())
			{
				// Note previous check uses SSE already, a plain copy will be faster than any memcpy
				a[0] = b[0];
				a[1] = b[1];
				a[2] = b[2];
				a[3] = b[3];
				a[4] = b[4];
				a[5] = b[5];
				a[6] = b[6];

				a[8] = b[8];

				a[9] = b[9];
				a[10] = b[10];
				a[11] = b[11];
				a[12] = b[12];

				return true;
			}

			return false;
		}
	};

	struct PSSelector
	{
		// Performance note: there are too many shader combinations
		// It might hurt the performance due to frequent toggling worse it could consume
		// a lots of memory.
		union
		{
			struct
			{
				// *** Word 1
				// Format
				uint32 tex_fmt   : 4;
				uint32 dfmt      : 2;
				uint32 depth_fmt : 2;
				// Alpha extension/Correction
				uint32 aem : 1;
				uint32 fba : 1;
				// Fog
				uint32 fog : 1;
				// Flat/goround shading
				uint32 iip : 1;
				// Pixel test
				uint32 date : 3;
				uint32 atst : 3;
				// Color sampling
				uint32 fst : 1; // Investigate to do it on the VS
				uint32 tfx : 3;
				uint32 tcc : 1;
				uint32 wms : 2;
				uint32 wmt : 2;
				uint32 ltf : 1;
				// Shuffle and fbmask effect
				uint32 shuffle  : 1;
				uint32 read_ba  : 1;
				uint32 write_rg : 1;
				uint32 fbmask   : 1;

				//uint32 _free1:0;

				// *** Word 2
				// Blend and Colclip
				uint32 blend_a : 2;
				uint32 blend_b : 2;
				uint32 blend_c : 2;
				uint32 blend_d : 2;
				uint32 clr1    : 1; // useful?
				uint32 hdr     : 1;
				uint32 colclip : 1;
				uint32 pabe    : 1;

				// Others ways to fetch the texture
				uint32 channel : 3;

				// Dithering
				uint32 dither : 2;

				// Depth clamp
				uint32 zclamp : 1;

				// Hack
				uint32 tcoffsethack : 1;
				uint32 urban_chaos_hle : 1;
				uint32 tales_of_abyss_hle : 1;
				uint32 tex_is_fb : 1; // Jak Shadows
				uint32 automatic_lod : 1;
				uint32 manual_lod : 1;
				uint32 point_sampler : 1;
				uint32 invalid_tex0 : 1; // Lupin the 3rd

				uint32 blend_premultiply : 2;

				uint32 _free2 : 4;
			};

			uint64 key;
		};

		// FIXME is the & useful ?
		operator uint64() const { return key; }

		PSSelector()
			: key(0)
		{
		}
	};

	struct PSSamplerSelector
	{
		union
		{
			struct
			{
				uint32 tau   : 1;
				uint32 tav   : 1;
				uint32 biln  : 1;
				uint32 triln : 3;
				uint32 aniso : 1;

				uint32 _free : 25;
			};

			uint32 key;
		};

		operator uint32() { return key; }

		PSSamplerSelector()
			: key(0)
		{
		}
		PSSamplerSelector(uint32 k)
			: key(k)
		{
		}
	};

	struct OMDepthStencilSelector
	{
		union
		{
			struct
			{
				uint32 ztst : 2;
				uint32 zwe  : 1;
				uint32 date : 1;
				uint32 date_one : 1;

				uint32 _free : 27;
			};

			uint32 key;
		};

		// FIXME is the & useful ?
		operator uint32() { return key; }

		OMDepthStencilSelector()
			: key(0)
		{
		}
		OMDepthStencilSelector(uint32 k)
			: key(k)
		{
		}
	};

	struct OMColorMaskSelector
	{
		union
		{
			struct
			{
				uint32 wr : 1;
				uint32 wg : 1;
				uint32 wb : 1;
				uint32 wa : 1;

				uint32 _free : 28;
			};

			struct
			{
				uint32 wrgba : 4;
			};

			uint32 key;
		};

		// FIXME is the & useful ?
		operator uint32() { return key & 0xf; }

		OMColorMaskSelector()
			: key(0xF)
		{
		}
		OMColorMaskSelector(uint32 c) { wrgba = c; }
	};

	struct alignas(32) MiscConstantBuffer
	{
		GSVector4i ScalingFactor;
		GSVector4i ChannelShuffle;
		GSVector4i EMOD_AC;

		MiscConstantBuffer() { memset(this, 0, sizeof(*this)); }
	};

	struct ProgramSelector
	{
		VSSelector vs;
		GSSelector gs;
		PSSelector ps;

		__fi bool operator==(const ProgramSelector& p) const { return vs.key == p.vs.key && gs.key == p.gs.key && ps.key == p.ps.key; }
		__fi bool operator!=(const ProgramSelector& p) const { return vs.key != p.vs.key || gs.key != p.gs.key || ps.key != p.ps.key; }
	};

	struct ProgramSelectorHash
	{
		__fi std::size_t operator()(const ProgramSelector& p) const noexcept
		{
			std::size_t h = 0;
			hash_combine(h, p.vs.key, p.gs.key, p.ps.key);
			return h;
		}
	};

	static int m_shader_inst;
	static int m_shader_reg;

private:
	int m_force_texture_clear;
	int m_mipmap;
	TriFiltering m_filter;

	static FILE* m_debug_gl_file;

	bool m_disable_hw_gl_draw;

	// Place holder for the GLSL shader code (to avoid useless reload)
	std::string m_shader_common_header;
	std::string m_shader_tfx_vgs;
	std::string m_shader_tfx_fs;

	GLuint m_fbo; // frame buffer container
	GLuint m_fbo_read; // frame buffer container only for reading
	GLuint m_fbo_write;	// frame buffer container only for writing

	std::unique_ptr<GL::StreamBuffer> m_vertex_stream_buffer;
	std::unique_ptr<GL::StreamBuffer> m_index_stream_buffer;
	GLuint m_vertex_array_object = 0;
	GLenum m_draw_topology = 0;

	std::unique_ptr<GL::StreamBuffer> m_vertex_uniform_stream_buffer;
	std::unique_ptr<GL::StreamBuffer> m_fragment_uniform_stream_buffer;
	GLint m_uniform_buffer_alignment = 0;

	struct
	{
		GL::Program ps[2]; // program object
	} m_merge_obj;

	struct
	{
		GL::Program ps[4]; // program object
	} m_interlace;

	struct
	{
		std::string vs;
		GL::Program ps[ShaderConvert_Count]; // program object
		GLuint ln; // sampler object
		GLuint pt; // sampler object
		GSDepthStencilOGL* dss;
		GSDepthStencilOGL* dss_write;
		GSUniformBufferOGL* cb;
	} m_convert;

	struct
	{
		GL::Program ps;
		GSUniformBufferOGL* cb;
	} m_fxaa;

#ifndef PCSX2_CORE
	struct
	{
		GL::Program ps;
		GSUniformBufferOGL* cb;
	} m_shaderfx;
#endif

	struct
	{
		GSDepthStencilOGL* dss;
		GSTexture* t;
	} m_date;

	struct
	{
		GL::Program ps;
	} m_shadeboost;

	struct
	{
		uint16 last_query;
		GLuint timer_query[1 << 16];

		GLuint timer() { return timer_query[last_query]; }
	} m_profiler;

	GLuint m_ps_ss[1 << 7];
	GSDepthStencilOGL* m_om_dss[1 << 5];
	std::unordered_map<ProgramSelector, GL::Program, ProgramSelectorHash> m_programs;
	GL::ShaderCache m_shader_cache;

	GLuint m_palette_ss;

	VSConstantBuffer m_vs_cb_cache;
	PSConstantBuffer m_ps_cb_cache;
	MiscConstantBuffer m_misc_cb_cache;

	GSTexture* CreateSurface(int type, int w, int h, int format) final;
	GSTexture* FetchSurface(int type, int w, int h, int format) final;

	void DoMerge(GSTexture* sTex[3], GSVector4* sRect, GSTexture* dTex, GSVector4* dRect, const GSRegPMODE& PMODE, const GSRegEXTBUF& EXTBUF, const GSVector4& c) final;
	void DoInterlace(GSTexture* sTex, GSTexture* dTex, int shader, bool linear, float yoffset = 0) final;
	void DoFXAA(GSTexture* sTex, GSTexture* dTex) final;
	void DoShadeBoost(GSTexture* sTex, GSTexture* dTex) final;
	void DoExternalFX(GSTexture* sTex, GSTexture* dTex) final;

	void OMAttachRt(GSTextureOGL* rt = NULL);
	void OMAttachDs(GSTextureOGL* ds = NULL);
	void OMSetFBO(GLuint fbo);

	uint16 ConvertBlendEnum(uint16 generic) final;

	void DrawStretchRect(const GSVector4& sRect, const GSVector4& dRect, const GSVector2i& ds);

public:
	GSDeviceOGL();
	virtual ~GSDeviceOGL();

	void GenerateProfilerData();

	// Used by OpenGL, so the same calling convention is required.
	static void APIENTRY DebugOutputToFile(GLenum gl_source, GLenum gl_type, GLuint id, GLenum gl_severity, GLsizei gl_length, const GLchar* gl_message, const void* userParam);

	bool Create(HostDisplay* display) override;

	void ResetAPIState() override;
	void RestoreAPIState() override;

	void DrawPrimitive() final;
	void DrawIndexedPrimitive() final;
	void DrawIndexedPrimitive(int offset, int count) final;

	void ClearRenderTarget(GSTexture* t, const GSVector4& c) final;
	void ClearRenderTarget(GSTexture* t, uint32 c) final;
	void ClearDepth(GSTexture* t) final;
	void ClearStencil(GSTexture* t, uint8 c) final;

	void InitPrimDateTexture(GSTexture* rt, const GSVector4i& area);
	void RecycleDateTexture();

	GSTexture* CopyOffscreen(GSTexture* src, const GSVector4& sRect, int w, int h, int format = 0, int ps_shader = 0) final;

	void CopyRect(GSTexture* sTex, GSTexture* dTex, const GSVector4i& r) final;

	// BlitRect *does* mess with GL state, be sure to re-bind.
	void BlitRect(GSTexture* sTex, const GSVector4i& r, const GSVector2i& dsize, bool at_origin, bool linear);

	void StretchRect(GSTexture* sTex, const GSVector4& sRect, GSTexture* dTex, const GSVector4& dRect, int shader = 0, bool linear = true) final;
	void StretchRect(GSTexture* sTex, const GSVector4& sRect, GSTexture* dTex, const GSVector4& dRect, const GL::Program& ps, bool linear = true);
	void StretchRect(GSTexture* sTex, const GSVector4& sRect, GSTexture* dTex, const GSVector4& dRect, bool red, bool green, bool blue, bool alpha) final;
	void StretchRect(GSTexture* sTex, const GSVector4& sRect, GSTexture* dTex, const GSVector4& dRect, const GL::Program& ps, int bs, OMColorMaskSelector cms, bool linear = true);

	void SetupDATE(GSTexture* rt, GSTexture* ds, const GSVertexPT1* vertices, bool datm);

	void IASetPrimitiveTopology(GLenum topology);
	void IASetVertexBuffer(const void* vertices, size_t count);
	void IASetIndexBuffer(const void* index, size_t count);

	void PSSetShaderResource(int i, GSTexture* sr) final;
	void PSSetShaderResources(GSTexture* sr0, GSTexture* sr1) final;
	void PSSetSamplerState(GLuint ss);

	void OMSetDepthStencilState(GSDepthStencilOGL* dss);
	void OMSetBlendState(uint8 blend_index = 0, uint8 blend_factor = 0, bool is_blend_constant = false, bool accumulation_blend = false);
	void OMSetRenderTargets(GSTexture* rt, GSTexture* ds, const GSVector4i* scissor = NULL) final;
	void OMSetColorMaskState(OMColorMaskSelector sel = OMColorMaskSelector());

	bool HasColorSparse() final { return GLLoader::found_compatible_GL_ARB_sparse_texture2; }
	bool HasDepthSparse() final { return GLLoader::found_compatible_sparse_depth; }

	bool CreateTextureFX();
	std::string GetShaderSource(const std::string_view& entry, GLenum type, const std::string_view& common_header, const std::string_view& glsl_h_code, const std::string_view& macro_sel);
	std::string GenGlslHeader(const std::string_view& entry, GLenum type, const std::string_view& macro);
	std::string GetVSSource(VSSelector sel);
	std::string GetGSSource(GSSelector sel);
	std::string GetPSSource(PSSelector sel);
	GLuint CreateSampler(PSSamplerSelector sel);
	GSDepthStencilOGL* CreateDepthStencil(OMDepthStencilSelector dssel);

	void SetupPipeline(const ProgramSelector& psel);
	void SetupCB(const VSConstantBuffer* vs_cb, const PSConstantBuffer* ps_cb);
	void SetupCBMisc(const GSVector4i& channel);
	void SetupSampler(PSSamplerSelector ssel);
	void SetupOM(OMDepthStencilSelector dssel);
	GLuint GetSamplerID(PSSamplerSelector ssel);
	GLuint GetPaletteSamplerID();

	void Barrier(GLbitfield b);
};
