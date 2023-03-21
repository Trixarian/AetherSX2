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

#include "PrecompiledHeader.h"
#include "GLLoader.h"
#include "GS.h"
#include "Host.h"

namespace GLExtension
{

	static std::unordered_set<std::string> s_extensions;

	bool Has(const std::string& ext)
	{
		return !!s_extensions.count(ext);
	}

	void Set(const std::string& ext, bool v)
	{
		if (v)
			s_extensions.insert(ext);
		else
			s_extensions.erase(ext);
	}
} // namespace GLExtension

namespace ReplaceGL
{
	void APIENTRY ScissorIndexed(GLuint index, GLint left, GLint bottom, GLsizei width, GLsizei height)
	{
		glScissor(left, bottom, width, height);
	}

	void APIENTRY ViewportIndexedf(GLuint index, GLfloat x, GLfloat y, GLfloat w, GLfloat h)
	{
		glViewport(GLint(x), GLint(y), GLsizei(w), GLsizei(h));
	}

	void APIENTRY TextureBarrier()
	{
	}

} // namespace ReplaceGL

namespace Emulate_DSA
{
	// Texture entry point
	void APIENTRY BindTextureUnit(GLuint unit, GLuint texture)
	{
		glActiveTexture(GL_TEXTURE0 + unit);
		glBindTexture(GL_TEXTURE_2D, texture);
	}

	void APIENTRY CreateTexture(GLenum target, GLsizei n, GLuint* textures)
	{
		glGenTextures(1, textures);
	}

	void APIENTRY TextureStorage(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height)
	{
		BindTextureUnit(7, texture);
		glTexStorage2D(GL_TEXTURE_2D, levels, internalformat, width, height);
	}

	void APIENTRY TextureSubImage(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels)
	{
		BindTextureUnit(7, texture);
		glTexSubImage2D(GL_TEXTURE_2D, level, xoffset, yoffset, width, height, format, type, pixels);
	}

	void APIENTRY CopyTextureSubImage(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
	{
		BindTextureUnit(7, texture);
		glCopyTexSubImage2D(GL_TEXTURE_2D, level, xoffset, yoffset, x, y, width, height);
	}

	void APIENTRY GetTexureImage(GLuint texture, GLint level, GLenum format, GLenum type, GLsizei bufSize, void* pixels)
	{
		BindTextureUnit(7, texture);
		glGetTexImage(GL_TEXTURE_2D, level, format, type, pixels);
	}

	void APIENTRY TextureParameteri(GLuint texture, GLenum pname, GLint param)
	{
		BindTextureUnit(7, texture);
		glTexParameteri(GL_TEXTURE_2D, pname, param);
	}

	void APIENTRY GenerateTextureMipmap(GLuint texture)
	{
		BindTextureUnit(7, texture);
		glGenerateMipmap(GL_TEXTURE_2D);
	}

	// Misc entry point
	// (only purpose is to have a consistent API otherwise it is useless)
	void APIENTRY CreateProgramPipelines(GLsizei n, GLuint* pipelines)
	{
		glGenProgramPipelines(n, pipelines);
	}

	void APIENTRY CreateSamplers(GLsizei n, GLuint* samplers)
	{
		glGenSamplers(n, samplers);
	}

	// Replace function pointer to emulate DSA behavior
	void Init()
	{
		fprintf(stderr, "DSA is not supported. Expect slower performance\n");
		glBindTextureUnit = BindTextureUnit;
		glCreateTextures = CreateTexture;
		glTextureStorage2D = TextureStorage;
		glTextureSubImage2D = TextureSubImage;
		glCopyTextureSubImage2D = CopyTextureSubImage;
		glGetTextureImage = GetTexureImage;
		glTextureParameteri = TextureParameteri;

		glCreateProgramPipelines = CreateProgramPipelines;
		glCreateSamplers = CreateSamplers;
	}
} // namespace Emulate_DSA

namespace GLLoader
{

#define fprintf_once(out, ...)         \
	do                                 \
		if (s_first_load)              \
			fprintf(out, __VA_ARGS__); \
	while (0);

	bool s_first_load = true;

	bool vendor_id_amd = false;
	bool vendor_id_nvidia = false;
	bool vendor_id_intel = false;
	bool vendor_id_arm = false;
	bool vendor_id_qualcomm = false;
	bool vendor_id_powervr = false;
	bool mesa_driver = false;
	bool in_replayer = false;
	bool buggy_pbo = false;

  bool is_gles = false;
	bool has_dual_source_blend = false;
	bool has_clip_control = true;
	bool found_framebuffer_fetch = false;
	bool found_geometry_shader = true; // we require GL3.3 so geometry must be supported by default
	bool found_GL_ARB_clear_texture = false;
	// DX11 GPU
	bool found_GL_ARB_gpu_shader5 = false;             // Require IvyBridge
	bool found_GL_ARB_shader_image_load_store = false; // Intel IB. Nvidia/AMD miss Mesa implementation.

	// In case sparse2 isn't supported
	bool found_compatible_GL_ARB_sparse_texture2 = false;
	bool found_compatible_sparse_depth = false;

	// Not yet used
#ifdef GL_EXT_TEX_SUB_IMAGE
	bool found_GL_ARB_get_texture_sub_image = false;
#endif

	static void mandatory(const std::string& ext)
	{
		if (!GLExtension::Has(ext))
		{
			fprintf(stderr, "ERROR: %s is NOT SUPPORTED\n", ext.c_str());
			throw GSRecoverableError();
		}

		return;
	}

	static bool optional(const std::string& name)
	{
		bool found = GLExtension::Has(name);

		if (!found)
		{
			fprintf_once(stdout, "INFO: %s is NOT SUPPORTED\n", name.c_str());
		}
		else
		{
			fprintf_once(stdout, "INFO: %s is available\n", name.c_str());
		}

		std::string opt("override_");
		opt += name;

		if (theApp.GetConfigI(opt.c_str()) != -1)
		{
			found = theApp.GetConfigB(opt.c_str());
			fprintf(stderr, "Override %s detection (%s)\n", name.c_str(), found ? "Enabled" : "Disabled");
			GLExtension::Set(name, found);
		}

		return found;
	}

  void check_gl_version()
	{
		const char* vendor = (const char*)glGetString(GL_VENDOR);
		const char* renderer = (const char*)glGetString(GL_RENDERER);
		if (strstr(vendor, "Advanced Micro Devices") || strstr(vendor, "ATI Technologies Inc.") || strstr(vendor, "ATI"))
			vendor_id_amd = true;
		else if (strstr(vendor, "NVIDIA Corporation"))
			vendor_id_nvidia = true;
		else if (strstr(vendor, "ARM"))
			vendor_id_arm = true;
		else if (strstr(vendor, "Qualcomm"))
			vendor_id_qualcomm = true;
		else if (strstr(vendor, "Qualcomm"))
			vendor_id_qualcomm = true;
		else if (strstr(vendor, "Imagination Technologies") || strstr(renderer, "PowerVR"))
			vendor_id_powervr = true;

#ifdef _WIN32
		else if (strstr(vendor, "Intel"))
			vendor_id_intel = true;
#else
		// On linux assumes the free driver if it isn't nvidia or amd pro driver
		mesa_driver = !vendor_id_nvidia && !vendor_id_amd;
#endif

		if (vendor_id_powervr || vendor_id_qualcomm || vendor_id_arm)
		{
			Host::AddOSDMessage("Disabling PBO texture uploads and geometry shaders.", 5.0f);

			// PBO uploads with texture sub image are broken on Adreno.
			// geometry shaders are broken on qualcomm (index out of range error) due to gl_in[]
			found_geometry_shader = false;
			buggy_pbo = true;
		}

		if (theApp.GetConfigI("override_geometry_shader") != -1)
		{
			found_geometry_shader = theApp.GetConfigB("override_geometry_shader");
			GLExtension::Set("GL_ARB_geometry_shader4", found_geometry_shader);
			fprintf(stderr, "Overriding geometry shaders detection\n");
		}

		GLint major_gl = 0;
		GLint minor_gl = 0;
		glGetIntegerv(GL_MAJOR_VERSION, &major_gl);
		glGetIntegerv(GL_MINOR_VERSION, &minor_gl);
    if (!GLAD_GL_VERSION_3_3 && !GLAD_GL_ES_VERSION_3_1)
		{
      fprintf(stderr, "OpenGL is not supported. Only OpenGL %d.%d\n was found", major_gl, minor_gl);
			throw GSRecoverableError();
		}
	}

	void check_gl_supported_extension()
	{
		int max_ext = 0;
		glGetIntegerv(GL_NUM_EXTENSIONS, &max_ext);
		for (GLint i = 0; i < max_ext; i++)
		{
			std::string ext{(const char*)glGetStringi(GL_EXTENSIONS, i)};
			GLExtension::Set(ext);
			//fprintf(stderr, "DEBUG ext: %s\n", ext.c_str());
		}

		// Mandatory for both renderer
    if (GLAD_GL_VERSION_3_3)
		{
			// GL4.1
			mandatory("GL_ARB_separate_shader_objects");
			// GL4.2
			mandatory("GL_ARB_shading_language_420pack");
			mandatory("GL_ARB_texture_storage");
			// GL4.3
			mandatory("GL_KHR_debug");
			// GL4.4
			mandatory("GL_ARB_buffer_storage");
		}

		// Extra
		{
			// Bonus
			optional("GL_ARB_sparse_texture");
			optional("GL_ARB_sparse_texture2");
			has_clip_control = optional("GL_ARB_clip_control");
			// GL4.0
			found_GL_ARB_gpu_shader5 = optional("GL_ARB_gpu_shader5");
			// GL4.2
			found_GL_ARB_shader_image_load_store = optional("GL_ARB_shader_image_load_store");
			// GL4.4
			found_GL_ARB_clear_texture = optional("GL_ARB_clear_texture");
			// GL4.5
			optional("GL_ARB_direct_state_access");
			// Mandatory for the advance HW renderer effect. Unfortunately Mesa LLVMPIPE/SWR renderers doesn't support this extension.
			// Rendering might be corrupted but it could be good enough for test/virtual machine.
			optional("GL_ARB_texture_barrier");
			// Not yet used
#ifdef GL_EXT_TEX_SUB_IMAGE
			found_GL_ARB_get_texture_sub_image = optional("GL_ARB_get_texture_sub_image");
#endif

			found_framebuffer_fetch = GLAD_GL_EXT_shader_framebuffer_fetch || GLAD_GL_ARM_shader_framebuffer_fetch;
			if (theApp.GetConfigB("disable_framebuffer_fetch"))
				found_framebuffer_fetch = false;
		}

		if (vendor_id_amd)
		{
			fprintf_once(stderr, "The OpenGL hardware renderer is slow on AMD GPUs due to an inefficient driver.\n"
								 "Check out the link below for further information.\n"
								 "https://github.com/PCSX2/pcsx2/wiki/OpenGL-and-AMD-GPUs---All-you-need-to-know\n");
		}

		if (vendor_id_intel && (!GLExtension::Has("GL_ARB_texture_barrier") || !GLExtension::Has("GL_ARB_direct_state_access")))
		{
			// Assume that driver support is good when texture barrier and DSA is supported, disable the log then.
			fprintf_once(stderr, "The OpenGL renderer is inefficient on Intel GPUs due to an inefficient driver.\n"
								 "Check out the link below for further information.\n"
								 "https://github.com/PCSX2/pcsx2/wiki/OpenGL-and-Intel-GPUs-All-you-need-to-know\n");
		}

		if (!GLExtension::Has("GL_ARB_viewport_array"))
		{
			glScissorIndexed = ReplaceGL::ScissorIndexed;
			glViewportIndexedf = ReplaceGL::ViewportIndexedf;
			fprintf_once(stderr, "GL_ARB_viewport_array is not supported! Function pointer will be replaced\n");
		}

		if (!GLExtension::Has("GL_ARB_texture_barrier"))
		{
			glTextureBarrier = ReplaceGL::TextureBarrier;
			fprintf_once(stderr, "GL_ARB_texture_barrier is not supported! Blending emulation will not be supported\n");
		}

		if (is_gles)
		{
			has_dual_source_blend = GLAD_GL_EXT_blend_func_extended || GLAD_GL_ARB_blend_func_extended;
			if (!has_dual_source_blend)
			{
				Host::AddOSDMessage("Dual-source blending is not supported, this will affect performance.", 5.0f);
				Console.Warning("Dual source blending is missing");
			}
			if (!has_dual_source_blend && !found_framebuffer_fetch)
			{
				Host::AddOSDMessage("Both dual source blending and framebuffer fetch are missing, things will be broken.", 10.0f);
				Console.Error("Missing both dual-source blending and framebuffer fetch");
			}
		}
		else
		{
			has_dual_source_blend = true;
		}

		// Thank you Intel for not providing support of basic features on your IGPUs.
    if (!GLAD_GL_ARB_direct_state_access)
		{
			Emulate_DSA::Init();
		}
	}

	bool is_sparse2_compatible(const char* name, GLenum internal_fmt, int x_max, int y_max)
	{
		GLint index_count = 0;
		glGetInternalformativ(GL_TEXTURE_2D, internal_fmt, GL_NUM_VIRTUAL_PAGE_SIZES_ARB, 1, &index_count);
		if (!index_count)
		{
			fprintf_once(stdout, "%s isn't sparse compatible. No index found\n", name);
			return false;
		}

		GLint x, y;
		glGetInternalformativ(GL_TEXTURE_2D, internal_fmt, GL_VIRTUAL_PAGE_SIZE_X_ARB, 1, &x);
		glGetInternalformativ(GL_TEXTURE_2D, internal_fmt, GL_VIRTUAL_PAGE_SIZE_Y_ARB, 1, &y);
		if (x > x_max && y > y_max)
		{
			fprintf_once(stdout, "%s isn't sparse compatible. Page size (%d,%d) is too big (%d, %d)\n",
						 name, x, y, x_max, y_max);
			return false;
		}

		return true;
	}

	static void check_sparse_compatibility()
	{
		if (!GLExtension::Has("GL_ARB_sparse_texture") ||
			!GLExtension::Has("GL_EXT_direct_state_access") ||
			theApp.GetConfigI("override_GL_ARB_sparse_texture") != 1)
		{
			found_compatible_GL_ARB_sparse_texture2 = false;
			found_compatible_sparse_depth = false;

			return;
		}

		found_compatible_GL_ARB_sparse_texture2 = true;
		if (!GLExtension::Has("GL_ARB_sparse_texture2"))
		{
			// Only check format from GSTextureOGL
			found_compatible_GL_ARB_sparse_texture2 &= is_sparse2_compatible("GL_R8", GL_R8, 256, 256);

			found_compatible_GL_ARB_sparse_texture2 &= is_sparse2_compatible("GL_R16UI", GL_R16UI, 256, 128);

			found_compatible_GL_ARB_sparse_texture2 &= is_sparse2_compatible("GL_R32UI", GL_R32UI, 128, 128);
			found_compatible_GL_ARB_sparse_texture2 &= is_sparse2_compatible("GL_R32I", GL_R32I, 128, 128);
			found_compatible_GL_ARB_sparse_texture2 &= is_sparse2_compatible("GL_RGBA8", GL_RGBA8, 128, 128);

			found_compatible_GL_ARB_sparse_texture2 &= is_sparse2_compatible("GL_RGBA16", GL_RGBA16, 128, 64);
			found_compatible_GL_ARB_sparse_texture2 &= is_sparse2_compatible("GL_RGBA16I", GL_RGBA16I, 128, 64);
			found_compatible_GL_ARB_sparse_texture2 &= is_sparse2_compatible("GL_RGBA16UI", GL_RGBA16UI, 128, 64);
			found_compatible_GL_ARB_sparse_texture2 &= is_sparse2_compatible("GL_RGBA16F", GL_RGBA16F, 128, 64);

			found_compatible_GL_ARB_sparse_texture2 &= is_sparse2_compatible("GL_RGBA32F", GL_RGBA32F, 64, 64);
		}

		// Can fit in 128x64 but 128x128 is enough
		// Disable sparse depth for AMD. Bad driver strikes again.
		// driver reports a compatible sparse format for depth texture but it isn't attachable to a frame buffer.
		found_compatible_sparse_depth = !vendor_id_amd && is_sparse2_compatible("GL_DEPTH32F_STENCIL8", GL_DEPTH32F_STENCIL8, 128, 128);

		fprintf_once(stdout, "INFO: sparse color texture is %s\n", found_compatible_GL_ARB_sparse_texture2 ? "available" : "NOT SUPPORTED");
		fprintf_once(stdout, "INFO: sparse depth texture is %s\n", found_compatible_sparse_depth ? "available" : "NOT SUPPORTED");
	}

  void check_gl_requirements()
  {
    check_gl_version();

		check_gl_supported_extension();

		// Bonus for sparse texture
		check_sparse_compatibility();

		fprintf_once(stdout, "\n");

		s_first_load = false;
	}
} // namespace GLLoader
