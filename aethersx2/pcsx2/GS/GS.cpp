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
#ifndef PCSX2_CORE
// NOTE: The include order matters - GS.h includes windows.h
#include "GS/Window/GSwxDialog.h"
#endif
#include "GS.h"
#include "GSUtil.h"
#include "Renderers/SW/GSRendererSW.h"
#include "Renderers/Null/GSRendererNull.h"
#include "Renderers/Null/GSDeviceNull.h"
#include "Renderers/OpenGL/GSDeviceOGL.h"
#include "Renderers/OpenGL/GSRendererOGL.h"
#include "GSLzma.h"

#include "common/pxStreams.h"
#include "common/pxStreams.h"
#include "common/Console.h"
#include "common/StringUtil.h"
#include "pcsx2/Config.h"
#include "pcsx2/Host.h"
#include "pcsx2/HostSettings.h"
#include "pcsx2/HostDisplay.h"
#include "pcsx2/GS.h"

#ifdef _WIN32

#include "Renderers/DX11/GSRendererDX11.h"
#include "Renderers/DX11/GSDevice11.h"
#include "GS/Renderers/DX11/D3D.h"


static HRESULT s_hr = E_FAIL;

#endif

#include <fstream>

// do NOT undefine this/put it above includes, as x11 people love to redefine
// things that make obscure compiler bugs, unless you want to run around and
// debug obscure compiler errors --govanify
#undef None

Pcsx2Config::GSOptions GSConfig;

static std::unique_ptr<GSRenderer> s_gs;
static HostDisplay::RenderAPI s_render_api;

int GSinit()
{
	if (!GSUtil::CheckSSE())
	{
		return -1;
	}

	// Vector instructions must be avoided when initialising GS since PCSX2
	// can crash if the CPU does not support the instruction set.
	// Initialise it here instead - it's not ideal since we have to strip the
	// const type qualifier from all the affected variables.
	theApp.SetConfigDir();
	theApp.Init();


	GSUtil::Init();

	if (g_const == nullptr)
		return -1;
	else
		g_const->Init();

#ifdef _WIN32
	s_hr = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
#endif

	return 0;
}

void GSshutdown()
{
	if (s_gs)
	{
		s_gs->Destroy();
		s_gs.reset();
	}

	Host::ReleaseHostDisplay();

#ifdef _WIN32
	if (SUCCEEDED(s_hr))
	{
		::CoUninitialize();

		s_hr = E_FAIL;
	}
#endif
}

void GSclose()
{
	if (s_gs)
	{
		s_gs->Destroy();
		s_gs.reset();
	}

	Host::ReleaseHostDisplay();
}

static HostDisplay::RenderAPI GetAPIForRenderer(GSRendererType renderer)
{
	switch (renderer)
	{
		case GSRendererType::OGL:
#ifndef _WIN32
		default:
#endif
			return HostDisplay::RenderAPI::OpenGL;

		case GSRendererType::VK:
			return HostDisplay::RenderAPI::Vulkan;

#ifdef _WIN32
		case GSRendererType::DX11:
		case GSRendererType::SW:
		default:
			return HostDisplay::RenderAPI::D3D11;
#endif
	}
}

extern std::unique_ptr<GSDevice> CreateGSDeviceVK();
extern std::unique_ptr<GSRenderer> CreateGSRendererVK(std::unique_ptr<GSDevice> dev);

static bool DoGSOpen(GSRendererType renderer, u8* basemem)
{
	HostDisplay* display = Host::GetHostDisplay();
	pxAssert(display);

	s_render_api = Host::GetHostDisplay()->GetRenderAPI();

	std::unique_ptr<GSDevice> dev;
	bool use_software = (renderer == GSRendererType::SW);
	switch (display->GetRenderAPI())
	{
#ifdef _WIN32
		case HostDisplay::RenderAPI::D3D11:
			dev = std::make_unique<GSDevice11>();
			break;
#endif

		case HostDisplay::RenderAPI::OpenGL:
		case HostDisplay::RenderAPI::OpenGLES:
			dev = std::make_unique<GSDeviceOGL>();
			break;

		case HostDisplay::RenderAPI::Vulkan:
			dev = CreateGSDeviceVK();
			break;

		case HostDisplay::RenderAPI::None:
			dev = std::make_unique<GSDeviceNull>();
			use_software = false; // force null renderer below
			break;

		default:
			Console.Error("Unknown render API %u", static_cast<unsigned>(display->GetRenderAPI()));
			return false;
	}

	try
	{
		if (!dev->Create(display))
		{
			dev->Destroy();
			return false;
		}

		if (!use_software)
		{
			switch (display->GetRenderAPI())
			{
#ifdef _WIN32
				case HostDisplay::RenderAPI::D3D11:
					s_gs = std::make_unique<GSRendererDX11>(std::move(dev));
					break;
#endif

				case HostDisplay::RenderAPI::OpenGL:
				case HostDisplay::RenderAPI::OpenGLES:
					s_gs = std::make_unique<GSRendererOGL>(std::move(dev));
					break;

				case HostDisplay::RenderAPI::Vulkan:
					s_gs = CreateGSRendererVK(std::move(dev));
					break;

				case HostDisplay::RenderAPI::None:
					s_gs = std::make_unique<GSRendererNull>(std::move(dev));
					break;

				default:
					break;
			}
		}
		else
		{
			const int threads = theApp.GetConfigI("extrathreads");
			s_gs = std::make_unique<GSRendererSW>(std::move(dev), threads);
		}
	}
	catch (std::exception& ex)
	{
		Console.Error("GS error: Exception caught in GSopen: %s", ex.what());
		return false;
	}

	s_gs->SetRegsMem(basemem);

	display->SetVSync(EmuConfig.GetEffectiveVsyncMode());
	display->SetDisplayMaxFPS(EmuConfig.GetPresentFPSLimit());

	return true;
}

static bool DoReopenGS(bool recreate_display)
{
	Console.WriteLn("Reopening GS with %s display", recreate_display ? "new" : "existing");

	s_gs->Flush();

	freezeData fd = {};
	if (s_gs->Freeze(&fd, true) != 0)
	{
		Console.Error("(DoReopenGS) Failed to get GS freeze size");
		return false;
	}

	std::unique_ptr<u8[]> fd_data = std::make_unique<u8[]>(fd.size);
	fd.data = fd_data.get();
	if (s_gs->Freeze(&fd, false) != 0)
	{
		Console.Error("(DoReopenGS) Failed to freeze GS");
		return false;
	}

	if (recreate_display)
	{
		s_gs->m_dev->ResetAPIState();
		if (Host::BeginPresentFrame(true))
			Host::EndPresentFrame();
	}

	uint8* basemem = s_gs->GetRegsMem();
	const u32 gamecrc = s_gs->GetGameCRC();
	const int gamecrc_options = s_gs->GetGameCRCOptions();
	s_gs->Destroy();
	s_gs.reset();

	if (recreate_display)
	{
		Host::ReleaseHostDisplay();
		if (!Host::AcquireHostDisplay(GetAPIForRenderer(GSConfig.Renderer)))
		{
			pxFailRel("(DoReopenGS) Failed to reacquire host display");
			return false;
		}

		Host::BeginFrame();
	}

	if (!DoGSOpen(GSConfig.Renderer, basemem))
	{
		pxFailRel("(DoReopenGS) Failed to recreate GS");
		return false;
	}

	if (s_gs->Defrost(&fd) != 0)
	{
		pxFailRel("(DoReopenGS) Failed to defrost");
		return false;
	}

	s_gs->SetGameCRC(gamecrc, gamecrc_options);
	return true;
}

bool GSopen(const Pcsx2Config::GSOptions& config, GSRendererType renderer, u8* basemem)
{
	if (renderer == GSRendererType::Auto)
		renderer = GSGetBestRenderer();

	GSConfig = config;
	GSConfig.Renderer = renderer;

	if (!Host::AcquireHostDisplay(GetAPIForRenderer(renderer)))
	{
		Console.Error("Failed to acquire host display");
		return false;
	}

	return DoGSOpen(renderer, basemem);
}

void GSreset()
{
	try
	{
		s_gs->Reset();
	}
	catch (GSRecoverableError)
	{
	}
}

void GSgifSoftReset(uint32 mask)
{
	try
	{
		s_gs->SoftReset(mask);
	}
	catch (GSRecoverableError)
	{
	}
}

void GSwriteCSR(uint32 csr)
{
	try
	{
		s_gs->WriteCSR(csr);
	}
	catch (GSRecoverableError)
	{
	}
}

void GSinitReadFIFO(uint8* mem)
{
	GL_PERF("Init Read FIFO1");
	try
	{
		s_gs->InitReadFIFO(mem, 1);
	}
	catch (GSRecoverableError)
	{
	}
	catch (const std::bad_alloc&)
	{
		fprintf(stderr, "GS: Memory allocation error\n");
	}
}

void GSreadFIFO(uint8* mem)
{
	try
	{
		s_gs->ReadFIFO(mem, 1);
	}
	catch (GSRecoverableError)
	{
	}
	catch (const std::bad_alloc&)
	{
		fprintf(stderr, "GS: Memory allocation error\n");
	}
}

void GSinitReadFIFO2(uint8* mem, uint32 size)
{
	GL_PERF("Init Read FIFO2");
	try
	{
		s_gs->InitReadFIFO(mem, size);
	}
	catch (GSRecoverableError)
	{
	}
	catch (const std::bad_alloc&)
	{
		fprintf(stderr, "GS: Memory allocation error\n");
	}
}

void GSreadFIFO2(uint8* mem, uint32 size)
{
	try
	{
		s_gs->ReadFIFO(mem, size);
	}
	catch (GSRecoverableError)
	{
	}
	catch (const std::bad_alloc&)
	{
		fprintf(stderr, "GS: Memory allocation error\n");
	}
}

void GSgifTransfer(const uint8* mem, uint32 size)
{
	try
	{
		s_gs->Transfer<3>(mem, size);
	}
	catch (GSRecoverableError)
	{
	}
}

void GSgifTransfer1(uint8* mem, uint32 addr)
{
	try
	{
		s_gs->Transfer<0>(const_cast<uint8*>(mem) + addr, (0x4000 - addr) / 16);
	}
	catch (GSRecoverableError)
	{
	}
}

void GSgifTransfer2(uint8* mem, uint32 size)
{
	try
	{
		s_gs->Transfer<1>(const_cast<uint8*>(mem), size);
	}
	catch (GSRecoverableError)
	{
	}
}

void GSgifTransfer3(uint8* mem, uint32 size)
{
	try
	{
		s_gs->Transfer<2>(const_cast<uint8*>(mem), size);
	}
	catch (GSRecoverableError)
	{
	}
}

void GSvsync(int field)
{
	try
	{
		s_gs->VSync(field);
	}
	catch (GSRecoverableError)
	{
	}
	catch (const std::bad_alloc&)
	{
		fprintf(stderr, "GS: Memory allocation error\n");
	}
}

uint32 GSmakeSnapshot(char* path)
{
	try
	{
		std::string s{path};

		if (!s.empty())
		{
			// Allows for providing a complete path
			std::string extension = s.substr(s.size() - 4, 4);
#ifdef _WIN32
			std::transform(extension.begin(), extension.end(), extension.begin(), (char(_cdecl*)(int))tolower);
#else
			std::transform(extension.begin(), extension.end(), extension.begin(), tolower);
#endif
			if (extension == ".png")
				return s_gs->MakeSnapshot(s);
			else if (s[s.length() - 1] != DIRECTORY_SEPARATOR)
				s = s + DIRECTORY_SEPARATOR;
		}

		return s_gs->MakeSnapshot(s + "gs");
	}
	catch (GSRecoverableError)
	{
		return false;
	}
}

void GSkeyEvent(const HostKeyEvent& e)
{
	try
	{
		if (s_gs)
			s_gs->KeyEvent(e);
	}
	catch (GSRecoverableError)
	{
	}
}

int GSfreeze(FreezeAction mode, freezeData* data)
{
	try
	{
		if (mode == FreezeAction::Save)
		{
			return s_gs->Freeze(data, false);
		}
		else if (mode == FreezeAction::Size)
		{
			return s_gs->Freeze(data, true);
		}
		else if (mode == FreezeAction::Load)
		{
			return s_gs->Defrost(data);
		}
	}
	catch (GSRecoverableError)
	{
	}

	return 0;
}

void GSconfigure()
{
#ifndef PCSX2_CORE
	try
	{
		if (!GSUtil::CheckSSE())
			return;

		theApp.SetConfigDir();
		theApp.Init();

		if (RunwxDialog())
		{
			theApp.ReloadConfig();
			// Force a reload of the gs state
			//theApp.SetCurrentRendererType(GSRendererType::Undefined);
		}
	}
	catch (GSRecoverableError)
	{
	}
#endif
}

int GStest()
{
	if (!GSUtil::CheckSSE())
		return -1;

	return 0;
}

void pt(const char* str)
{
	struct tm* current;
	time_t now;

	time(&now);
	current = localtime(&now);

	printf("%02i:%02i:%02i%s", current->tm_hour, current->tm_min, current->tm_sec, str);
}

bool GSsetupRecording(std::string& filename)
{
	if (s_gs == NULL)
	{
		printf("GS: no s_gs for recording\n");
		return false;
	}
#if defined(__unix__) || defined(__APPLE__)
	if (!theApp.GetConfigB("capture_enabled"))
	{
		printf("GS: Recording is disabled\n");
		return false;
	}
#endif
	printf("GS: Recording start command\n");
	if (s_gs->BeginCapture(filename))
	{
		pt(" - Capture started\n");
		return true;
	}
	else
	{
		pt(" - Capture cancelled\n");
		return false;
	}
}

void GSendRecording()
{
	printf("GS: Recording end command\n");
	s_gs->EndCapture();
	pt(" - Capture ended\n");
}

void GSsetGameCRC(uint32 crc, int options)
{
	s_gs->SetGameCRC(crc, options);
}

void GSsetFrameSkip(int frameskip)
{
	s_gs->SetFrameSkip(frameskip);
}

void GSgetInternalResolution(int* width, int* height)
{
	GSRenderer* gs = s_gs.get();
	if (!gs)
	{
		*width = 0;
		*height = 0;
		return;
	}

	const GSVector2i res(gs->GetInternalResolution());
	*width = res.x;
	*height = res.y;
}

void GSgetStats(std::string& info)
{
	GSPerfMon& pm = g_perfmon;

	const char* api_name = HostDisplay::RenderAPIToString(s_render_api);

	if (GSConfig.Renderer == GSRendererType::SW)
	{
		float sum = 0.0f;
		for (int i = GSPerfMon::WorkerDraw0; i < GSPerfMon::TimerLast; i++)
			sum += pm.GetTimer(static_cast<GSPerfMon::timer_t>(i));

		const double fps = GetVerticalFrequency();
		const double fillrate = pm.Get(GSPerfMon::Fillrate);
		info = format("%s SW | %d S | %d P | %d D | %.2f U | %.2f D | %.2f mpps | %d%% WCPU",
			api_name,
			(int)pm.Get(GSPerfMon::SyncPoint),
			(int)pm.Get(GSPerfMon::Prim),
			(int)pm.Get(GSPerfMon::Draw),
			pm.Get(GSPerfMon::Swizzle) / 1024,
			pm.Get(GSPerfMon::Unswizzle) / 1024,
			fps * fillrate / (1024 * 1024),
			static_cast<int>(std::lround(sum)));
	}
	else
	{
		info = format("%s HW | %d P | %d D | %d DC | %d RB | %d TC | %d TU",
			api_name,
			(int)pm.Get(GSPerfMon::Prim),
			(int)pm.Get(GSPerfMon::Draw),
			(int)std::ceil(pm.Get(GSPerfMon::DrawCalls)),
			(int)std::ceil(pm.Get(GSPerfMon::Readbacks)),
			(int)std::ceil(pm.Get(GSPerfMon::TextureCopies)),
			(int)std::ceil(pm.Get(GSPerfMon::TextureUploads)));
	}
}

#ifndef PCSX2_CORE

std::string GSGetConfigString(const char* key)
{
	return theApp.GetConfigS(key);
}

void GSLoadConfigFromApp(Pcsx2Config::GSOptions* config)
{
	// GSinit hasn't been called when we're here in wx.
	theApp.SetConfigDir();
	theApp.Init();

	config->LinearPresent = theApp.GetConfigB("linear_present");
	config->IntegerScaling = theApp.GetConfigB("integer_scaling");
	config->UseDebugDevice = theApp.GetConfigB("debug_device");
	config->UseBlitSwapChain = theApp.GetConfigB("blit_swap_chain");
	config->ThrottlePresentRate = theApp.GetConfigB("throttle_present_rate");
	config->ThreadedPresentation = theApp.GetConfigB("threaded_presentation");
	config->OsdShowMessages = theApp.GetConfigB("osd_show_messages");
	config->OsdShowSpeed = theApp.GetConfigB("osd_show_speed");
	config->OsdShowFPS = theApp.GetConfigB("osd_show_fps");
	config->OsdShowCPU = theApp.GetConfigB("osd_show_cpu");
	config->OsdShowResolution = theApp.GetConfigB("osd_show_resolution");
	config->OsdShowGSStats = theApp.GetConfigB("osd_show_gs_stats");
	config->OsdScale = static_cast<float>(theApp.GetConfigI("osd_scale"));

	config->Renderer = static_cast<GSRendererType>(theApp.GetConfigI("Renderer"));
	config->UpscaleMultiplier = std::max(0, theApp.GetConfigI("upscale_multiplier"));
	config->HWMipmap = static_cast<HWMipmapLevel>(theApp.GetConfigI("mipmap_hw"));
	config->InterlaceMode = static_cast<GSInterlaceMode>(theApp.GetConfigI("interlace"));
	config->HWDisableReadbacks = theApp.GetConfigB("disable_hw_readbacks");
	config->AccurateDATE = theApp.GetConfigB("accurate_date");
	config->GPUPaletteConversion = theApp.GetConfigB("paltex");
	config->ConservativeFramebuffer = theApp.GetConfigB("conservative_framebuffer");
	config->AutoFlushSW = theApp.GetConfigB("autoflush_sw");
	config->UserHacks = theApp.GetConfigB("UserHacks");
	config->UserHacks_WildHack = theApp.GetConfigB("UserHacks_WildHack");
	config->PreloadFrameWithGSData = theApp.GetConfigB("preload_frame_with_gs_data");
	config->UserHacks_AlignSpriteX = theApp.GetConfigB("UserHacks_align_sprite_X");
	config->UserHacks_DisableDepthSupport = theApp.GetConfigB("UserHacks_DisableDepthSupport");
	config->UserHacks_CPUFBConversion = theApp.GetConfigB("UserHacks_CPU_FB_Conversion");
	config->UserHacks_DisablePartialInvalidation = theApp.GetConfigB("UserHacks_DisablePartialInvalidation");
	config->UserHacks_AutoFlush = theApp.GetConfigB("UserHacks_AutoFlush");
	config->UserHacks_DisableSafeFeatures = theApp.GetConfigB("UserHacks_Disable_Safe_Features");
	config->WrapGSMem = theApp.GetConfigB("wrap_gs_mem");
	config->UserHacks_MergePPSprite = theApp.GetConfigB("UserHacks_merge_pp_sprite");
	config->FXAA = theApp.GetConfigB("fxaa");
	config->SWBlending = theApp.GetConfigI("accurate_blending_unit");
	config->SWExtraThreads = theApp.GetConfigI("extrathreads");
	config->SWExtraThreadsHeight = theApp.GetConfigI("extrathreads_height");
	config->TVShader = theApp.GetConfigI("TVShader");
	config->PreloadTexture = theApp.GetConfigB("preload_texture");
}

#endif

void GSUpdateConfig(const Pcsx2Config::GSOptions& new_config)
{
	Pcsx2Config::GSOptions old_config(std::move(GSConfig));
	GSConfig = new_config;
	GSConfig.Renderer = (GSConfig.Renderer == GSRendererType::Auto) ? old_config.Renderer : GSConfig.Renderer;
	if (!s_gs)
		return;

	HostDisplay* display = Host::GetHostDisplay();

	// Handle OSD scale changes by pushing a window resize through.
	if (new_config.OsdScale != old_config.OsdScale)
		Host::ResizeHostDisplay(display->GetWindowWidth(), display->GetWindowHeight(), display->GetWindowScale());

	// Options which need a full teardown/recreate.
	if (GSConfig.Renderer != old_config.Renderer ||
		GSConfig.UseDebugDevice != old_config.UseDebugDevice ||
		GSConfig.UseBlitSwapChain != old_config.UseBlitSwapChain ||
		GSConfig.ThreadedPresentation != old_config.ThreadedPresentation)
	{
		HostDisplay::RenderAPI existing_api = Host::GetHostDisplay()->GetRenderAPI();
		if (existing_api == HostDisplay::RenderAPI::OpenGLES)
			existing_api = HostDisplay::RenderAPI::OpenGL;
		DoReopenGS(existing_api != GetAPIForRenderer(GSConfig.Renderer));
	}
	else if (GSConfig.UpscaleMultiplier != old_config.UpscaleMultiplier ||
			 GSConfig.HWMipmap != old_config.HWMipmap ||
			 GSConfig.InterlaceMode != old_config.InterlaceMode ||
			 GSConfig.AccurateDATE != old_config.AccurateDATE ||
			 GSConfig.GPUPaletteConversion != old_config.GPUPaletteConversion ||
			 GSConfig.ConservativeFramebuffer != old_config.ConservativeFramebuffer ||
			 GSConfig.AutoFlushSW != old_config.AutoFlushSW ||
			 GSConfig.UserHacks != old_config.UserHacks ||
			 GSConfig.UserHacks_WildHack != old_config.UserHacks_WildHack ||
			 GSConfig.PreloadFrameWithGSData != old_config.PreloadFrameWithGSData ||
			 GSConfig.UserHacks_AlignSpriteX != old_config.UserHacks_AlignSpriteX ||
			 GSConfig.UserHacks_DisableDepthSupport != old_config.UserHacks_DisableDepthSupport ||
			 GSConfig.UserHacks_CPUFBConversion != old_config.UserHacks_CPUFBConversion ||
			 GSConfig.UserHacks_DisablePartialInvalidation != old_config.UserHacks_DisablePartialInvalidation ||
			 GSConfig.UserHacks_AutoFlush != old_config.UserHacks_AutoFlush ||
			 GSConfig.UserHacks_DisableSafeFeatures != old_config.UserHacks_DisableSafeFeatures ||
			 GSConfig.WrapGSMem != old_config.WrapGSMem ||
			 GSConfig.UserHacks_MergePPSprite != old_config.UserHacks_MergePPSprite ||
			 GSConfig.FXAA != old_config.FXAA ||
			 GSConfig.PreloadTexture != old_config.PreloadTexture ||
			 GSConfig.SWBlending != old_config.SWBlending ||
			 GSConfig.SWExtraThreads != old_config.SWExtraThreads ||
			 GSConfig.SWExtraThreadsHeight != old_config.SWExtraThreadsHeight || 
			 GSConfig.TVShader != old_config.TVShader)
	{
		DoReopenGS(false);
	}
	else
	{
		// Individual settings
	}
}

void GSSwitchRenderer(GSRendererType new_renderer)
{
	if (new_renderer == GSRendererType::Auto)
		new_renderer = GSGetBestRenderer();

	if (!s_gs || GSConfig.Renderer == new_renderer)
		return;

	HostDisplay::RenderAPI existing_api = Host::GetHostDisplay()->GetRenderAPI();
	if (existing_api == HostDisplay::RenderAPI::OpenGLES)
		existing_api = HostDisplay::RenderAPI::OpenGL;

	const bool is_software_switch = (new_renderer == GSRendererType::SW || GSConfig.Renderer == GSRendererType::SW);
	GSConfig.Renderer = new_renderer;
	DoReopenGS(!is_software_switch && existing_api != GetAPIForRenderer(new_renderer));
}

void GSResetAPIState()
{
	if (!s_gs)
		return;

	s_gs->m_dev->ResetAPIState();
}

void GSRestoreAPIState()
{
	if (!s_gs)
		return;

	s_gs->m_dev->RestoreAPIState();
}

bool GSSaveSnapshotToMemory(uint32 width, uint32 height, std::vector<uint32>* pixels)
{
	if (!s_gs)
		return false;

	return s_gs->SaveSnapshotToMemory(width, height, pixels);
}

std::string format(const char* fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	int size = vsnprintf(nullptr, 0, fmt, args) + 1;
	va_end(args);

	assert(size > 0);
	std::vector<char> buffer(std::max(1, size));

	va_start(args, fmt);
	vsnprintf(buffer.data(), size, fmt, args);
	va_end(args);

	return {buffer.data()};
}

// Helper path to dump texture
#ifdef _WIN32
const std::string root_sw("c:\\temp1\\_");
const std::string root_hw("c:\\temp2\\_");
#else
#ifdef _M_AMD64
const std::string root_sw("/tmp/GS_SW_dump64/");
const std::string root_hw("/tmp/GS_HW_dump64/");
#else
const std::string root_sw("/tmp/GS_SW_dump32/");
const std::string root_hw("/tmp/GS_HW_dump32/");
#endif
#endif

#ifdef _WIN32

void* vmalloc(size_t size, bool code)
{
	void* ptr = VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, code ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE);
	if (!ptr)
		throw std::bad_alloc();
	return ptr;
}

void vmfree(void* ptr, size_t size)
{
	VirtualFree(ptr, 0, MEM_RELEASE);
}

static HANDLE s_fh = NULL;
static uint8* s_Next[8];

void* fifo_alloc(size_t size, size_t repeat)
{
	ASSERT(s_fh == NULL);

	if (repeat >= countof(s_Next))
	{
		fprintf(stderr, "Memory mapping overflow (%zu >= %u)\n", repeat, static_cast<unsigned>(countof(s_Next)));
		return vmalloc(size * repeat, false); // Fallback to default vmalloc
	}

	s_fh = CreateFileMapping(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, size, nullptr);
	DWORD errorID = ::GetLastError();
	if (s_fh == NULL)
	{
		fprintf(stderr, "Failed to reserve memory. WIN API ERROR:%u\n", errorID);
		return vmalloc(size * repeat, false); // Fallback to default vmalloc
	}

	int mmap_segment_failed = 0;
	void* fifo = MapViewOfFile(s_fh, FILE_MAP_ALL_ACCESS, 0, 0, size);
	for (size_t i = 1; i < repeat; i++)
	{
		void* base = (uint8*)fifo + size * i;
		s_Next[i] = (uint8*)MapViewOfFileEx(s_fh, FILE_MAP_ALL_ACCESS, 0, 0, size, base);
		errorID = ::GetLastError();
		if (s_Next[i] != base)
		{
			mmap_segment_failed++;
			if (mmap_segment_failed > 4)
			{
				fprintf(stderr, "Memory mapping failed after %d attempts, aborting. WIN API ERROR:%u\n", mmap_segment_failed, errorID);
				fifo_free(fifo, size, repeat);
				return vmalloc(size * repeat, false); // Fallback to default vmalloc
			}
			do
			{
				UnmapViewOfFile(s_Next[i]);
				s_Next[i] = 0;
			} while (--i > 0);

			fifo = MapViewOfFile(s_fh, FILE_MAP_ALL_ACCESS, 0, 0, size);
		}
	}

	return fifo;
}

void fifo_free(void* ptr, size_t size, size_t repeat)
{
	ASSERT(s_fh != NULL);

	if (s_fh == NULL)
	{
		if (ptr != NULL)
			vmfree(ptr, size);
		return;
	}

	UnmapViewOfFile(ptr);

	for (size_t i = 1; i < countof(s_Next); i++)
	{
		if (s_Next[i] != 0)
		{
			UnmapViewOfFile(s_Next[i]);
			s_Next[i] = 0;
		}
	}

	CloseHandle(s_fh);
	s_fh = NULL;
}

#else

#include <sys/mman.h>
#include <unistd.h>

void* vmalloc(size_t size, bool code)
{
	size_t mask = getpagesize() - 1;

	size = (size + mask) & ~mask;

	int prot = PROT_READ | PROT_WRITE;
	int flags = MAP_PRIVATE | MAP_ANONYMOUS;

	if (code)
	{
		prot |= PROT_EXEC;
#if defined(_M_AMD64) && !defined(__APPLE__)
		// macOS doesn't allow any mappings in the first 4GB of address space
		flags |= MAP_32BIT;
#endif
	}

	void* ptr = mmap(NULL, size, prot, flags, -1, 0);
	if (ptr == MAP_FAILED)
		throw std::bad_alloc();
	return ptr;
}

void vmfree(void* ptr, size_t size)
{
	size_t mask = getpagesize() - 1;

	size = (size + mask) & ~mask;

	munmap(ptr, size);
}

#ifdef __ANDROID__
void* fifo_alloc(size_t size, size_t repeat);
void fifo_free(void* ptr, size_t size, size_t repeat);
#else

static int s_shm_fd = -1;

void* fifo_alloc(size_t size, size_t repeat)
{
	ASSERT(s_shm_fd == -1);

	const char* file_name = "/GS.mem";
	s_shm_fd = shm_open(file_name, O_RDWR | O_CREAT | O_EXCL, 0600);
	if (s_shm_fd != -1)
	{
		shm_unlink(file_name); // file is deleted but descriptor is still open
	}
	else
	{
		fprintf(stderr, "Failed to open %s due to %s\n", file_name, strerror(errno));
		return nullptr;
	}

	if (ftruncate(s_shm_fd, repeat * size) < 0)
		fprintf(stderr, "Failed to reserve memory due to %s\n", strerror(errno));

	void* fifo = mmap(nullptr, size * repeat, PROT_READ | PROT_WRITE, MAP_SHARED, s_shm_fd, 0);

	for (size_t i = 1; i < repeat; i++)
	{
		void* base = (uint8*)fifo + size * i;
		uint8* next = (uint8*)mmap(base, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, s_shm_fd, 0);
		if (next != base)
			fprintf(stderr, "Fail to mmap contiguous segment\n");
	}

	return fifo;
}

void fifo_free(void* ptr, size_t size, size_t repeat)
{
	ASSERT(s_shm_fd >= 0);

	if (s_shm_fd < 0)
		return;

	munmap(ptr, size * repeat);

	close(s_shm_fd);
	s_shm_fd = -1;
}
#endif
#endif

size_t GSApp::GetIniString(const char* lpAppName, const char* lpKeyName, const char* lpDefault, char* lpReturnedString, size_t nSize, const char* lpFileName)
{
#ifdef PCSX2_CORE
	std::string ret(Host::GetStringSettingValue("EmuCore/GS", lpKeyName, lpDefault));
	return StringUtil::Strlcpy(lpReturnedString, ret, nSize);
#else
	BuildConfigurationMap(lpFileName);

	std::string key(lpKeyName);
	std::string value = m_configuration_map[key];
	if (value.empty())
	{
		// save the value for futur call
		m_configuration_map[key] = std::string(lpDefault);
		strcpy(lpReturnedString, lpDefault);
	}
	else
		strcpy(lpReturnedString, value.c_str());

	return 0;
#endif
}

bool GSApp::WriteIniString(const char* lpAppName, const char* lpKeyName, const char* pString, const char* lpFileName)
{
#ifndef PCSX2_CORE
	BuildConfigurationMap(lpFileName);

	std::string key(lpKeyName);
	std::string value(pString);
	m_configuration_map[key] = value;

	// Save config to a file
	FILE* f = px_fopen(lpFileName, "w");

	if (f == NULL)
		return false; // FIXME print a nice message

		// Maintain compatibility with GSDumpGUI/old Windows ini.
#ifdef _WIN32
	fprintf(f, "[Settings]\n");
#endif

	for (const auto& entry : m_configuration_map)
	{
		// Do not save the inifile key which is not an option
		if (entry.first.compare("inifile") == 0)
			continue;

		// Only keep option that have a default value (allow to purge old option of the GS.ini)
		if (!entry.second.empty() && m_default_configuration.find(entry.first) != m_default_configuration.end())
			fprintf(f, "%s = %s\n", entry.first.c_str(), entry.second.c_str());
	}
	fclose(f);
#endif

	return false;
}

#ifndef PCSX2_CORE
int GSApp::GetIniInt(const char* lpAppName, const char* lpKeyName, int nDefault, const char* lpFileName)
{
	BuildConfigurationMap(lpFileName);

	std::string value = m_configuration_map[std::string(lpKeyName)];
	if (value.empty())
	{
		// save the value for futur call
		SetConfig(lpKeyName, nDefault);
		return nDefault;
	}
	else
		return atoi(value.c_str());
}
#endif

GSApp theApp;

GSApp::GSApp()
{
	// Empty constructor causes an illegal instruction exception on an SSE4.2 machine on Windows.
	// Non-empty doesn't, but raises a SIGILL signal when compiled against GCC 6.1.1.
	// So here's a compromise.
#ifdef _WIN32
	Init();
#endif
}

void GSApp::Init()
{
	static bool is_initialised = false;
	if (is_initialised)
		return;
	is_initialised = true;

	m_section = "Settings";

#ifdef _WIN32
	m_gs_renderers.push_back(GSSetting(static_cast<uint32>(GSRendererType::DX11), "Direct3D 11", ""));
#endif
	m_gs_renderers.push_back(GSSetting(static_cast<uint32>(GSRendererType::OGL), "OpenGL", ""));
	m_gs_renderers.push_back(GSSetting(static_cast<uint32>(GSRendererType::VK), "Vulkan", ""));
	m_gs_renderers.push_back(GSSetting(static_cast<uint32>(GSRendererType::SW), "Software", ""));

	// The null renderer goes third, it has use for benchmarking purposes in a release build
	m_gs_renderers.push_back(GSSetting(static_cast<uint32>(GSRendererType::Null), "Null", ""));

	m_gs_interlace.push_back(GSSetting(0, "None", ""));
	m_gs_interlace.push_back(GSSetting(1, "Weave tff", "saw-tooth"));
	m_gs_interlace.push_back(GSSetting(2, "Weave bff", "saw-tooth"));
	m_gs_interlace.push_back(GSSetting(3, "Bob tff", "use blend if shaking"));
	m_gs_interlace.push_back(GSSetting(4, "Bob bff", "use blend if shaking"));
	m_gs_interlace.push_back(GSSetting(5, "Blend tff", "slight blur, 1/2 fps"));
	m_gs_interlace.push_back(GSSetting(6, "Blend bff", "slight blur, 1/2 fps"));
	m_gs_interlace.push_back(GSSetting(7, "Automatic", "Default"));

	m_gs_upscale_multiplier.push_back(GSSetting(1, "Native", "PS2"));
	m_gs_upscale_multiplier.push_back(GSSetting(2, "2x Native", "~720p"));
	m_gs_upscale_multiplier.push_back(GSSetting(3, "3x Native", "~1080p"));
	m_gs_upscale_multiplier.push_back(GSSetting(4, "4x Native", "~1440p 2K"));
	m_gs_upscale_multiplier.push_back(GSSetting(5, "5x Native", "~1620p"));
	m_gs_upscale_multiplier.push_back(GSSetting(6, "6x Native", "~2160p 4K"));
	m_gs_upscale_multiplier.push_back(GSSetting(7, "7x Native", "~2520p"));
	m_gs_upscale_multiplier.push_back(GSSetting(8, "8x Native", "~2880p"));

	m_gs_max_anisotropy.push_back(GSSetting(0, "Off", "Default"));
	m_gs_max_anisotropy.push_back(GSSetting(2, "2x", ""));
	m_gs_max_anisotropy.push_back(GSSetting(4, "4x", ""));
	m_gs_max_anisotropy.push_back(GSSetting(8, "8x", ""));
	m_gs_max_anisotropy.push_back(GSSetting(16, "16x", ""));

	m_gs_dithering.push_back(GSSetting(0, "Off", ""));
	m_gs_dithering.push_back(GSSetting(2, "Unscaled", "Default"));
	m_gs_dithering.push_back(GSSetting(1, "Scaled", ""));

	m_gs_bifilter.push_back(GSSetting(static_cast<uint32>(BiFiltering::Nearest), "Nearest", ""));
	m_gs_bifilter.push_back(GSSetting(static_cast<uint32>(BiFiltering::Forced_But_Sprite), "Bilinear", "Forced excluding sprite"));
	m_gs_bifilter.push_back(GSSetting(static_cast<uint32>(BiFiltering::Forced), "Bilinear", "Forced"));
	m_gs_bifilter.push_back(GSSetting(static_cast<uint32>(BiFiltering::PS2), "Bilinear", "PS2"));

	m_gs_trifilter.push_back(GSSetting(static_cast<uint32>(TriFiltering::None), "None", "Default"));
	m_gs_trifilter.push_back(GSSetting(static_cast<uint32>(TriFiltering::PS2), "Trilinear", ""));
	m_gs_trifilter.push_back(GSSetting(static_cast<uint32>(TriFiltering::Forced), "Trilinear", "Ultra/Slow"));

	m_gs_generic_list.push_back(GSSetting(-1, "Automatic", "Default"));
	m_gs_generic_list.push_back(GSSetting(0, "Force-Disabled", ""));
	m_gs_generic_list.push_back(GSSetting(1, "Force-Enabled", ""));

	m_gs_hack.push_back(GSSetting(0, "Off", "Default"));
	m_gs_hack.push_back(GSSetting(1, "Half", ""));
	m_gs_hack.push_back(GSSetting(2, "Full", ""));

	m_gs_offset_hack.push_back(GSSetting(0, "Off", "Default"));
	m_gs_offset_hack.push_back(GSSetting(1, "Normal", "Vertex"));
	m_gs_offset_hack.push_back(GSSetting(2, "Special", "Texture"));
	m_gs_offset_hack.push_back(GSSetting(3, "Special", "Texture - aggressive"));

	m_gs_hw_mipmapping = {
		GSSetting(HWMipmapLevel::Automatic, "Automatic", "Default"),
		GSSetting(HWMipmapLevel::Off, "Off", ""),
		GSSetting(HWMipmapLevel::Basic, "Basic", "Fast"),
		GSSetting(HWMipmapLevel::Full, "Full", "Slow"),
	};

	m_gs_crc_level = {
		GSSetting(CRCHackLevel::Automatic, "Automatic", "Default"),
		GSSetting(CRCHackLevel::None, "None", "Debug"),
		GSSetting(CRCHackLevel::Minimum, "Minimum", "Debug"),
#ifdef _DEBUG
		GSSetting(CRCHackLevel::Partial, "Partial", "OpenGL"),
		GSSetting(CRCHackLevel::Full, "Full", "Direct3D"),
#endif
		GSSetting(CRCHackLevel::Aggressive, "Aggressive", ""),
	};

	m_gs_acc_blend_level.push_back(GSSetting(0, "None", "Fastest"));
	m_gs_acc_blend_level.push_back(GSSetting(1, "Basic", "Recommended"));
	m_gs_acc_blend_level.push_back(GSSetting(2, "Medium", ""));
	m_gs_acc_blend_level.push_back(GSSetting(3, "High", ""));
	m_gs_acc_blend_level.push_back(GSSetting(4, "Full", "Very Slow"));
	m_gs_acc_blend_level.push_back(GSSetting(5, "Ultra", "Ultra Slow"));

	m_gs_acc_blend_level_d3d11.push_back(GSSetting(0, "None", "Fastest"));
	m_gs_acc_blend_level_d3d11.push_back(GSSetting(1, "Basic", "Recommended"));
	m_gs_acc_blend_level_d3d11.push_back(GSSetting(2, "Medium", "Debug"));
	m_gs_acc_blend_level_d3d11.push_back(GSSetting(3, "High", "Debug"));

	m_gs_tv_shaders.push_back(GSSetting(0, "None", ""));
	m_gs_tv_shaders.push_back(GSSetting(1, "Scanline filter", ""));
	m_gs_tv_shaders.push_back(GSSetting(2, "Diagonal filter", ""));
	m_gs_tv_shaders.push_back(GSSetting(3, "Triangular filter", ""));
	m_gs_tv_shaders.push_back(GSSetting(4, "Wave filter", ""));

	// clang-format off
	// Avoid to clutter the ini file with useless options
#ifdef _WIN32
	// Per OS option.
	m_default_configuration["adapter"]																		= "";
	m_default_configuration["CaptureFileName"]                            = "";
	m_default_configuration["CaptureVideoCodecDisplayName"]               = "";
	m_default_configuration["dx_break_on_severity"]                       = "0";
	// D3D Blending option
	m_default_configuration["accurate_blending_unit_d3d11"]               = "1";
#else
	m_default_configuration["linux_replay"]                               = "1";
#endif
	m_default_configuration["aa1"]                                        = "1";
	m_default_configuration["accurate_date"]                              = "1";
	m_default_configuration["accurate_blending_unit"]                     = "1";
	m_default_configuration["AspectRatio"]                                = "1";
	m_default_configuration["autoflush_sw"]                               = "1";
	m_default_configuration["blit_swap_chain"]                            = "0";
	m_default_configuration["capture_enabled"]                            = "0";
	m_default_configuration["capture_out_dir"]                            = "/tmp/GS_Capture";
	m_default_configuration["capture_threads"]                            = "4";
	m_default_configuration["CaptureHeight"]                              = "480";
	m_default_configuration["CaptureWidth"]                               = "640";
	m_default_configuration["crc_hack_level"]                             = std::to_string(static_cast<int8>(CRCHackLevel::Automatic));
	m_default_configuration["CrcHacksExclusions"]                         = "";
	m_default_configuration["debug_glsl_shader"]                          = "0";
	m_default_configuration["debug_device"]                               = "0";
	m_default_configuration["disable_hw_gl_draw"]                         = "0";
	m_default_configuration["dithering_ps2"]                              = "2";
	m_default_configuration["dump"]                                       = "0";
	m_default_configuration["extrathreads"]                               = "2";
	m_default_configuration["extrathreads_height"]                        = "4";
	m_default_configuration["filter"]                                     = std::to_string(static_cast<int8>(BiFiltering::PS2));
	m_default_configuration["force_texture_clear"]                        = "0";
	m_default_configuration["fxaa"]                                       = "0";
	m_default_configuration["integer_scaling"]                            = "0";
	m_default_configuration["interlace"]                                  = "7";
	m_default_configuration["conservative_framebuffer"]                   = "1";
	m_default_configuration["linear_present"]                             = "1";
	m_default_configuration["MaxAnisotropy"]                              = "0";
	m_default_configuration["mipmap"]                                     = "1";
	m_default_configuration["mipmap_hw"]                                  = std::to_string(static_cast<int>(HWMipmapLevel::Automatic));
	m_default_configuration["ModeHeight"]                                 = "480";
	m_default_configuration["ModeWidth"]                                  = "640";
	m_default_configuration["NTSC_Saturation"]                            = "1";
	m_default_configuration["osd_show_messages"]                          = "1";
	m_default_configuration["osd_show_speed"]                             = "0";
	m_default_configuration["osd_show_fps"]                               = "0";
	m_default_configuration["osd_show_cpu"]                               = "0";
	m_default_configuration["osd_show_resolution"]                        = "0";
	m_default_configuration["osd_show_gs_stats"]                          = "0";
	m_default_configuration["osd_scale"]                                  = "100";
	m_default_configuration["override_geometry_shader"]                   = "-1";
	m_default_configuration["override_GL_ARB_copy_image"]                 = "-1";
	m_default_configuration["override_GL_ARB_clear_texture"]              = "-1";
	m_default_configuration["override_GL_ARB_clip_control"]               = "-1";
	m_default_configuration["override_GL_ARB_direct_state_access"]        = "-1";
	m_default_configuration["override_GL_ARB_draw_buffers_blend"]         = "-1";
	m_default_configuration["override_GL_ARB_gpu_shader5"]                = "-1";
	m_default_configuration["override_GL_ARB_shader_image_load_store"]    = "-1";
	m_default_configuration["override_GL_ARB_sparse_texture"]             = "-1";
	m_default_configuration["override_GL_ARB_sparse_texture2"]            = "-1";
	m_default_configuration["override_GL_ARB_texture_barrier"]            = "-1";
#ifdef GL_EXT_TEX_SUB_IMAGE
	m_default_configuration["override_GL_ARB_get_texture_sub_image"]      = "-1";
#endif
	m_default_configuration["paltex"]                                     = "0";
	m_default_configuration["png_compression_level"]                      = std::to_string(Z_BEST_SPEED);
	m_default_configuration["preload_frame_with_gs_data"]                 = "0";
	m_default_configuration["preload_texture"]                            = "0";
	m_default_configuration["Renderer"]                                   = std::to_string(static_cast<int>(GSRendererType::Auto));
	m_default_configuration["resx"]                                       = "1024";
	m_default_configuration["resy"]                                       = "1024";
	m_default_configuration["save"]                                       = "0";
	m_default_configuration["savef"]                                      = "0";
	m_default_configuration["savel"]                                      = "5000";
	m_default_configuration["saven"]                                      = "0";
	m_default_configuration["savet"]                                      = "0";
	m_default_configuration["savez"]                                      = "0";
	m_default_configuration["ShadeBoost"]                                 = "0";
	m_default_configuration["ShadeBoost_Brightness"]                      = "50";
	m_default_configuration["ShadeBoost_Contrast"]                        = "50";
	m_default_configuration["ShadeBoost_Saturation"]                      = "50";
	m_default_configuration["shaderfx"]                                   = "0";
	m_default_configuration["shaderfx_conf"]                              = "shaders/GS_FX_Settings.ini";
	m_default_configuration["shaderfx_glsl"]                              = "shaders/GS.fx";
	m_default_configuration["throttle_present_rate"]                      = "0";
	m_default_configuration["TVShader"]                                   = "0";
	m_default_configuration["upscale_multiplier"]                         = "1";
	m_default_configuration["UserHacks"]                                  = "0";
	m_default_configuration["UserHacks_align_sprite_X"]                   = "0";
	m_default_configuration["UserHacks_AutoFlush"]                        = "0";
	m_default_configuration["UserHacks_DisableDepthSupport"]              = "0";
	m_default_configuration["UserHacks_Disable_Safe_Features"]            = "0";
	m_default_configuration["UserHacks_DisablePartialInvalidation"]       = "0";
	m_default_configuration["UserHacks_CPU_FB_Conversion"]                = "0";
	m_default_configuration["UserHacks_Half_Bottom_Override"]             = "-1";
	m_default_configuration["UserHacks_HalfPixelOffset"]                  = "0";
	m_default_configuration["UserHacks_merge_pp_sprite"]                  = "0";
	m_default_configuration["UserHacks_round_sprite_offset"]              = "0";
	m_default_configuration["UserHacks_SkipDraw"]                         = "0";
	m_default_configuration["UserHacks_SkipDraw_Offset"]                  = "0";
	m_default_configuration["UserHacks_TCOffsetX"]                        = "0";
	m_default_configuration["UserHacks_TCOffsetY"]                        = "0";
	m_default_configuration["UserHacks_TextureInsideRt"]                  = "0";
	m_default_configuration["UserHacks_TriFilter"]                        = std::to_string(static_cast<int8>(TriFiltering::None));
	m_default_configuration["UserHacks_WildHack"]                         = "0";
	m_default_configuration["wrap_gs_mem"]                                = "0";
	m_default_configuration["vsync"]                                      = "0";
	// clang-format on
}

#ifndef PCSX2_CORE
void GSApp::ReloadConfig()
{
	if (m_configuration_map.empty())
		return;

	auto file = m_configuration_map.find("inifile");
	if (file == m_configuration_map.end())
		return;

	// A map was built so reload it
	std::string filename = file->second;
	m_configuration_map.clear();
	BuildConfigurationMap(filename.c_str());
}

void GSApp::BuildConfigurationMap(const char* lpFileName)
{
	// Check if the map was already built
	std::string inifile_value(lpFileName);
	if (inifile_value.compare(m_configuration_map["inifile"]) == 0)
		return;
	m_configuration_map["inifile"] = inifile_value;

	// Load config from file
#ifdef _WIN32
	std::ifstream file(convert_utf8_to_utf16(lpFileName));
#else
	std::ifstream file(lpFileName);
#endif
	if (!file.is_open())
		return;

	std::string line;
	while (std::getline(file, line))
	{
		const auto separator = line.find('=');
		if (separator == std::string::npos)
			continue;

		std::string key = line.substr(0, separator);
		// Trim trailing whitespace
		key.erase(key.find_last_not_of(" \r\t") + 1);

		if (key.empty())
			continue;

		// Only keep options that have a default value so older, no longer used
		// ini options can be purged.
		if (m_default_configuration.find(key) == m_default_configuration.end())
			continue;

		std::string value = line.substr(separator + 1);
		// Trim leading whitespace
		value.erase(0, value.find_first_not_of(" \r\t"));

		m_configuration_map[key] = value;
	}
}
#endif

void GSApp::SetConfigDir()
{
	// we need to initialize the ini folder later at runtime than at theApp init, as
	// core settings aren't populated yet, thus we do populate it if needed either when
	// opening GS settings or init -- govanify
	wxString iniName(L"GS.ini");
	m_ini = EmuFolders::Settings.Combine(iniName).GetFullPath();
}

std::string GSApp::GetConfigS(const char* entry)
{
	char buff[4096] = {0};
	auto def = m_default_configuration.find(entry);

	if (def != m_default_configuration.end())
	{
		GetIniString(m_section.c_str(), entry, def->second.c_str(), buff, countof(buff), m_ini.c_str());
	}
	else
	{
		fprintf(stderr, "Option %s doesn't have a default value\n", entry);
		GetIniString(m_section.c_str(), entry, "", buff, countof(buff), m_ini.c_str());
	}

	return {buff};
}

void GSApp::SetConfig(const char* entry, const char* value)
{
	WriteIniString(m_section.c_str(), entry, value, m_ini.c_str());
}

int GSApp::GetConfigI(const char* entry)
{
	auto def = m_default_configuration.find(entry);

	if (def != m_default_configuration.end())
	{
#ifndef PCSX2_CORE
		return GetIniInt(m_section.c_str(), entry, std::stoi(def->second), m_ini.c_str());
#else
		return Host::GetIntSettingValue("EmuCore/GS", entry, std::stoi(def->second));
#endif
	}
	else
	{
		fprintf(stderr, "Option %s doesn't have a default value\n", entry);
#ifndef PCSX2_CORE
		return GetIniInt(m_section.c_str(), entry, 0, m_ini.c_str());
#else
		return Host::GetIntSettingValue("EmuCore/GS", entry, 0);
#endif
	}
}

bool GSApp::GetConfigB(const char* entry)
{
#ifndef PCSX2_CORE
	return !!GetConfigI(entry);
#else
	auto def = m_default_configuration.find(entry);

	if (def != m_default_configuration.end())
	{
		return Host::GetBoolSettingValue("EmuCore/GS", entry, StringUtil::FromChars<bool>(def->second).value_or(false));
	}
	else
	{
		fprintf(stderr, "Option %s doesn't have a default value\n", entry);
		return Host::GetBoolSettingValue("EmuCore/GS", entry, false);
	}
#endif
}

void GSApp::SetConfig(const char* entry, int value)
{
	char buff[32] = {0};

	sprintf(buff, "%d", value);

	SetConfig(entry, buff);
}
