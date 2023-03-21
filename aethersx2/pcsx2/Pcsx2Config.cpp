/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2021  PCSX2 Dev Team
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

#include <wx/fileconf.h>

#include "common/SettingsInterface.h"
#include "common/SettingsWrapper.h"
#include "common/StringUtil.h"
#include "Config.h"
#include "HostDisplay.h"
#include "GS.h"
#include "HostDisplay.h"
#include "CDVD/CDVDaccess.h"
#include "MemoryCardFile.h"
#include "CDVD/CDVDaccess.h"

#ifndef PCSX2_CORE
#include "gui/AppConfig.h"
#include "GS/GS.h"
#endif

namespace EmuFolders
{
	wxDirName AppRoot;
	wxDirName DataRoot;
	wxDirName Settings;
	wxDirName Bios;
	wxDirName Snapshots;
	wxDirName Savestates;
	wxDirName MemoryCards;
	wxDirName Langs;
	wxDirName Logs;
	wxDirName Cheats;
	wxDirName CheatsWS;
	wxDirName Resources;
	wxDirName Cache;
	wxDirName Covers;
	wxDirName GameSettings;
} // namespace EmuFolders

void TraceLogFilters::LoadSave(SettingsWrapper& wrap)
{
	SettingsWrapSection("EmuCore/TraceLog");

	SettingsWrapEntry(Enabled);

	// Retaining backwards compat of the trace log enablers isn't really important, and
	// doing each one by hand would be murder.  So let's cheat and just save it as an int:

	SettingsWrapEntry(EE.bitset);
	SettingsWrapEntry(IOP.bitset);
}

const wxChar* const tbl_SpeedhackNames[] =
	{
		L"mvuFlag",
		L"InstantVU1"};

const __fi wxChar* EnumToString(SpeedhackId id)
{
	return tbl_SpeedhackNames[id];
}

void Pcsx2Config::SpeedhackOptions::Set(SpeedhackId id, bool enabled)
{
	EnumAssert(id);
	switch (id)
	{
		case Speedhack_mvuFlag:
			vuFlagHack = enabled;
			break;
		case Speedhack_InstantVU1:
			vu1Instant = enabled;
			break;
			jNO_DEFAULT;
	}
}

Pcsx2Config::SpeedhackOptions::SpeedhackOptions()
{
	DisableAll();

	// Set recommended speedhacks to enabled by default. They'll still be off globally on resets.
	WaitLoop = true;
	IntcStat = true;
	vuFlagHack = true;
	vu1Instant = true;
}

Pcsx2Config::SpeedhackOptions& Pcsx2Config::SpeedhackOptions::DisableAll()
{
	bitset = 0;
	EECycleRate = 0;
	EECycleSkip = 0;

	return *this;
}

void Pcsx2Config::SpeedhackOptions::LoadSave(SettingsWrapper& wrap)
{
	SettingsWrapSection("EmuCore/Speedhacks");

	SettingsWrapBitfield(EECycleRate);
	SettingsWrapBitfield(EECycleSkip);
	SettingsWrapBitBool(fastCDVD);
	SettingsWrapBitBool(IntcStat);
	SettingsWrapBitBool(WaitLoop);
	SettingsWrapBitBool(vuFlagHack);
	SettingsWrapBitBool(vuThread);
	SettingsWrapBitBool(vu1Instant);
}

void Pcsx2Config::ProfilerOptions::LoadSave(SettingsWrapper& wrap)
{
	SettingsWrapSection("EmuCore/Profiler");

	SettingsWrapBitBool(Enabled);
	SettingsWrapBitBool(RecBlocks_EE);
	SettingsWrapBitBool(RecBlocks_IOP);
	SettingsWrapBitBool(RecBlocks_VU0);
	SettingsWrapBitBool(RecBlocks_VU1);
}

Pcsx2Config::RecompilerOptions::RecompilerOptions()
{
	bitset = 0;

	//StackFrameChecks	= false;
	//PreBlockCheckEE	= false;

	// All recs are enabled by default.

	EnableEE = true;
	EnableEECache = false;
	EnableIOP = true;
	EnableVU0 = true;
	EnableVU1 = true;
#ifdef __ANDROID__
	EnableFastmem = true;
#else
	EnableFastmem = false;
#endif

	// vu and fpu clamping default to standard overflow.
	vuOverflow = true;
	//vuExtraOverflow = false;
	//vuSignOverflow = false;
	//vuUnderflow = false;

	fpuOverflow = true;
	//fpuExtraOverflow = false;
	//fpuFullMode = false;
}

void Pcsx2Config::RecompilerOptions::ApplySanityCheck()
{
	bool fpuIsRight = true;

	if (fpuExtraOverflow)
		fpuIsRight = fpuOverflow;

	if (fpuFullMode)
		fpuIsRight = fpuOverflow && fpuExtraOverflow;

	if (!fpuIsRight)
	{
		// Values are wonky; assume the defaults.
		fpuOverflow = RecompilerOptions().fpuOverflow;
		fpuExtraOverflow = RecompilerOptions().fpuExtraOverflow;
		fpuFullMode = RecompilerOptions().fpuFullMode;
	}

	bool vuIsOk = true;

	if (vuExtraOverflow)
		vuIsOk = vuIsOk && vuOverflow;
	if (vuSignOverflow)
		vuIsOk = vuIsOk && vuExtraOverflow;

	if (!vuIsOk)
	{
		// Values are wonky; assume the defaults.
		vuOverflow = RecompilerOptions().vuOverflow;
		vuExtraOverflow = RecompilerOptions().vuExtraOverflow;
		vuSignOverflow = RecompilerOptions().vuSignOverflow;
		vuUnderflow = RecompilerOptions().vuUnderflow;
	}
}

void Pcsx2Config::RecompilerOptions::LoadSave(SettingsWrapper& wrap)
{
	SettingsWrapSection("EmuCore/CPU/Recompiler");

	SettingsWrapBitBool(EnableEE);
	SettingsWrapBitBool(EnableIOP);
	SettingsWrapBitBool(EnableEECache);
	SettingsWrapBitBool(EnableVU0);
	SettingsWrapBitBool(EnableVU1);
	SettingsWrapBitBool(EnableFastmem);

#ifndef __ANDROID__
	SettingsWrapBitBool(vuOverflow);
	SettingsWrapBitBool(vuExtraOverflow);
	SettingsWrapBitBool(vuSignOverflow);
	SettingsWrapBitBool(vuUnderflow);

	SettingsWrapBitBool(fpuOverflow);
	SettingsWrapBitBool(fpuExtraOverflow);
	SettingsWrapBitBool(fpuFullMode);
#else
	int FPUClampMode = (fpuFullMode ? 3 : (fpuExtraOverflow ? 2 : (fpuOverflow ? 1 : 0)));
	int VUClampMode = (vuSignOverflow ? 3 : (vuExtraOverflow ? 2 : (vuOverflow ? 1 : 0)));
	SettingsWrapBitfield(FPUClampMode);
	SettingsWrapBitfield(VUClampMode);
	fpuOverflow = (FPUClampMode > 0);
	fpuExtraOverflow = (FPUClampMode > 1);
	fpuFullMode = (FPUClampMode > 2);
	vuOverflow = (VUClampMode > 0);
	vuExtraOverflow = (VUClampMode > 1);
	vuSignOverflow = (VUClampMode > 2);
#endif

	SettingsWrapBitBool(StackFrameChecks);
	SettingsWrapBitBool(PreBlockCheckEE);
	SettingsWrapBitBool(PreBlockCheckIOP);
}

Pcsx2Config::CpuOptions::CpuOptions()
{
	sseMXCSR.bitmask = DEFAULT_sseMXCSR;
	sseVUMXCSR.bitmask = DEFAULT_sseVUMXCSR;
}

void Pcsx2Config::CpuOptions::ApplySanityCheck()
{
	sseMXCSR.ClearExceptionFlags().DisableExceptions();
	sseVUMXCSR.ClearExceptionFlags().DisableExceptions();

	Recompiler.ApplySanityCheck();
}

void Pcsx2Config::CpuOptions::LoadSave(SettingsWrapper& wrap)
{
	SettingsWrapSection("EmuCore/CPU");

	SettingsWrapBitBoolEx(sseMXCSR.DenormalsAreZero, "FPU.DenormalsAreZero");
	SettingsWrapBitBoolEx(sseMXCSR.FlushToZero, "FPU.FlushToZero");
	SettingsWrapBitfieldEx(sseMXCSR.RoundingControl, "FPU.Roundmode");

	SettingsWrapBitBoolEx(sseVUMXCSR.DenormalsAreZero, "VU.DenormalsAreZero");
	SettingsWrapBitBoolEx(sseVUMXCSR.FlushToZero, "VU.FlushToZero");
	SettingsWrapBitfieldEx(sseVUMXCSR.RoundingControl, "VU.Roundmode");

	Recompiler.LoadSave(wrap);
}

const char* Pcsx2Config::GSOptions::AspectRatioNames[] = {
	"Stretch",
	"4:3",
	"16:9",
	nullptr};

const char* Pcsx2Config::GSOptions::FMVAspectRatioSwitchNames[] = {
	"Off",
	"4:3",
	"16:9",
	nullptr};

const char* Pcsx2Config::GSOptions::GetRendererName(GSRendererType type)
{
	switch (type)
	{
	case GSRendererType::Auto: return "Auto";
	case GSRendererType::DX11: return "Direct3D 11";
	case GSRendererType::OGL: return "OpenGL";
	case GSRendererType::VK: return "Vulkan";
	case GSRendererType::SW: return "Software";
	case GSRendererType::Null: return "Null";
	default: return "";
	}
}

Pcsx2Config::GSOptions::GSOptions()
{
	bitset = 0;

	IntegerScaling = false;
	LinearPresent = true;
	UseDebugDevice = false;
	UseBlitSwapChain = false;
	ThrottlePresentRate = false;
	OsdShowMessages = true;
	OsdShowSpeed = false;
	OsdShowFPS = false;
	OsdShowCPU = false;
	OsdShowResolution = false;
	OsdShowGSStats = false;

	HWDisableReadbacks = false;
	AccurateDATE = true;
	GPUPaletteConversion = false;
	ConservativeFramebuffer = true;
	AutoFlushSW = true;
	PreloadFrameWithGSData = false;
	WrapGSMem = false;
	UserHacks = false;
	UserHacks_AlignSpriteX = false;
	UserHacks_AutoFlush = false;
	UserHacks_CPUFBConversion = false;
	UserHacks_DisableDepthSupport = false;
	UserHacks_DisablePartialInvalidation = false;
	UserHacks_DisableSafeFeatures = false;
	UserHacks_MergePPSprite = false;
	UserHacks_WildHack = false;
}

void Pcsx2Config::GSOptions::LoadSave(SettingsWrapper& wrap)
{
	SettingsWrapSection("EmuCore/GS");

#ifdef PCSX2_DEVBUILD
	SettingsWrapEntry(SynchronousMTGS);
#endif
	SettingsWrapEntry(VsyncQueueSize);

	SettingsWrapEntry(FrameLimitEnable);
	SettingsWrapEntry(FrameSkipEnable);
	wrap.EnumEntry(CURRENT_SETTINGS_SECTION, "VsyncEnable", VsyncEnable, NULL, VsyncEnable);

	// LimitScalar is set at runtime.
	SettingsWrapEntry(FramerateNTSC);
	SettingsWrapEntry(FrameratePAL);

	SettingsWrapEntry(FramesToDraw);
	SettingsWrapEntry(FramesToSkip);

#ifdef PCSX2_CORE
	SettingsWrapBitBool(IntegerScaling);
	SettingsWrapBitBool(LinearPresent);
	SettingsWrapBitBool(UseDebugDevice);
	SettingsWrapBitBool(UseBlitSwapChain);
	SettingsWrapBitBool(ThrottlePresentRate);
	SettingsWrapBitBool(ThreadedPresentation);

	SettingsWrapBitBool(OsdShowMessages);
	SettingsWrapBitBool(OsdShowSpeed);
	SettingsWrapBitBool(OsdShowFPS);
	SettingsWrapBitBool(OsdShowCPU);
	SettingsWrapBitBool(OsdShowResolution);
	SettingsWrapBitBool(OsdShowGSStats);

	wrap.EnumEntry(CURRENT_SETTINGS_SECTION, "AspectRatio", AspectRatio, AspectRatioNames, AspectRatio);
	wrap.EnumEntry(CURRENT_SETTINGS_SECTION, "FMVAspectRatioSwitch", FMVAspectRatioSwitch, FMVAspectRatioSwitchNames, FMVAspectRatioSwitch);

	SettingsWrapEntry(Zoom);
	SettingsWrapEntry(StretchY);
	SettingsWrapEntry(OffsetX);
	SettingsWrapEntry(OffsetY);

	SettingsWrapEntry(OsdScale);

	// Options load from main INI.
	SettingsWrapBitfieldEx(UpscaleMultiplier, "upscale_multiplier");
	SettingsWrapBitfieldEx(SWBlending, "accurate_blending_unit");
	SettingsWrapBitfieldEx(SWExtraThreads, "extrathreads");
	SettingsWrapBitfieldEx(SWExtraThreadsHeight, "extrathreads_height");
	SettingsWrapBitBoolEx(HWDisableReadbacks, "disable_hw_readbacks");
	SettingsWrapBitBoolEx(AccurateDATE, "accurate_date");
	SettingsWrapBitBoolEx(GPUPaletteConversion, "paltex");
	SettingsWrapBitBoolEx(ConservativeFramebuffer, "conservative_framebuffer");
	SettingsWrapBitBoolEx(AutoFlushSW, "autoflush_sw");
	SettingsWrapBitBoolEx(UserHacks, "UserHacks");
	SettingsWrapBitBoolEx(UserHacks_WildHack, "UserHacks_WildHack");
	SettingsWrapBitBoolEx(PreloadFrameWithGSData, "preload_frame_with_gs_data");
	SettingsWrapBitBoolEx(UserHacks_AlignSpriteX, "UserHacks_align_sprite_X");
	SettingsWrapBitBoolEx(UserHacks_DisableDepthSupport, "UserHacks_DisableDepthSupport");
	SettingsWrapBitBoolEx(UserHacks_CPUFBConversion, "UserHacks_CPU_FB_Conversion");
	SettingsWrapBitBoolEx(UserHacks_DisablePartialInvalidation, "UserHacks_DisablePartialInvalidation");
	SettingsWrapBitBoolEx(UserHacks_AutoFlush, "UserHacks_AutoFlush");
	SettingsWrapBitBoolEx(UserHacks_DisableSafeFeatures, "UserHacks_Disable_Safe_Features");
	SettingsWrapBitBoolEx(WrapGSMem, "wrap_gs_mem");
	SettingsWrapBitBoolEx(UserHacks_MergePPSprite, "UserHacks_merge_pp_sprite");
	SettingsWrapBitBoolEx(FXAA, "fxaa");
	SettingsWrapBitBoolEx(PreloadTexture, "preload_texture");
	Renderer = static_cast<GSRendererType>(wrap.EntryBitfield(CURRENT_SETTINGS_SECTION, "Renderer", static_cast<int>(Renderer), static_cast<int>(Renderer)));
	HWMipmap = static_cast<HWMipmapLevel>(wrap.EntryBitfield(CURRENT_SETTINGS_SECTION, "mipmap_hw", static_cast<int>(HWMipmap), static_cast<int>(HWMipmap)));
	InterlaceMode = static_cast<GSInterlaceMode>(wrap.EntryBitfield(CURRENT_SETTINGS_SECTION, "interlace", static_cast<int>(InterlaceMode), static_cast<int>(InterlaceMode)));
	TVShader = wrap.EntryBitfield(CURRENT_SETTINGS_SECTION, "TVShader", TVShader, TVShader);
#else
	if (wrap.IsLoading())
		GSLoadConfigFromApp(this);
#endif
}

bool Pcsx2Config::GSOptions::UseHardwareRenderer() const
{
	return (Renderer == GSRendererType::DX11 || Renderer == GSRendererType::OGL || Renderer == GSRendererType::VK);
}

float Pcsx2Config::GSOptions::GetAspectRatioFloat() const
{
	switch (AspectRatio)
	{
		case AspectRatioType::Stretch:
			return 1.0f;

		case AspectRatioType::R16_9:
			return 16.0f / 9.0f;

		case AspectRatioType::R4_3:
		default:
			return 4.0f / 3.0f;
	}
}

VsyncMode Pcsx2Config::GetEffectiveVsyncMode() const
{
	if (GS.LimitScalar != 1.0)
	{
		Console.WriteLn("Vsync is OFF");
		return VsyncMode::Off;
	}

	Console.WriteLn("Vsync is %s", GS.VsyncEnable == VsyncMode::Off ? "OFF" : (GS.VsyncEnable == VsyncMode::Adaptive ? "ADAPTIVE" : "ON"));
	return GS.VsyncEnable;
}

float Pcsx2Config::GetPresentFPSLimit() const
{
	if (GS.LimitScalar > 0.0 && GS.LimitScalar <= 1.0 || !GS.ThrottlePresentRate)
		return 0.0f;

	// TODO: Choose something better.
	HostDisplay* display = Host::GetHostDisplay();
	const float rr = display ? display->GetWindowInfo().surface_refresh_rate : 0.0f;
	return (rr > 0.0f) ? rr : 60.0f;
}

Pcsx2Config::SPU2Options::SPU2Options()
{
	OutputModule = "cubeb";
}

void Pcsx2Config::SPU2Options::LoadSave(SettingsWrapper& wrap)
{
	{
		SettingsWrapSection("SPU2/Mixing");

		Interpolation = static_cast<InterpolationMode>(wrap.EntryBitfield(CURRENT_SETTINGS_SECTION, "Interpolation", static_cast<int>(Interpolation), static_cast<int>(Interpolation)));
		SettingsWrapEntry(FinalVolume);

		SettingsWrapEntry(VolumeAdjustC);
		SettingsWrapEntry(VolumeAdjustFL);
		SettingsWrapEntry(VolumeAdjustFR);
		SettingsWrapEntry(VolumeAdjustBL);
		SettingsWrapEntry(VolumeAdjustBR);
		SettingsWrapEntry(VolumeAdjustSL);
		SettingsWrapEntry(VolumeAdjustSR);
		SettingsWrapEntry(VolumeAdjustLFE);
	}

	{
		SettingsWrapSection("SPU2/Output");

		SettingsWrapEntry(OutputModule);
		SettingsWrapEntry(Latency);
		SynchMode = static_cast<SynchronizationMode>(wrap.EntryBitfield(CURRENT_SETTINGS_SECTION, "SynchMode", static_cast<int>(SynchMode), static_cast<int>(SynchMode)));
		SettingsWrapEntry(SpeakerConfiguration);
	}
}

const wxChar* const tbl_GamefixNames[] =
	{
		L"FpuMul",
		L"FpuNegDiv",
		L"GoemonTlb",
		L"SkipMPEG",
		L"OPHFlag",
		L"EETiming",
		L"DMABusy",
		L"GIFFIFO",
		L"VIFFIFO",
		L"VIF1Stall",
		L"VuAddSub",
		L"Ibit",
		L"VUKickstart",
		L"VUOverflow",
		L"XGKick"};

const __fi wxChar* EnumToString(GamefixId id)
{
	return tbl_GamefixNames[id];
}

// all gamefixes are disabled by default.
Pcsx2Config::GamefixOptions::GamefixOptions()
{
	DisableAll();
}

Pcsx2Config::GamefixOptions& Pcsx2Config::GamefixOptions::DisableAll()
{
	bitset = 0;
	return *this;
}

// Enables a full list of gamefixes.  The list can be either comma or pipe-delimited.
//   Example:  "XGKick,IpuWait"  or  "EEtiming,FpuCompare"
// If an unrecognized tag is encountered, a warning is printed to the console, but no error
// is generated.  This allows the system to function in the event that future versions of
// PCSX2 remove old hacks once they become obsolete.
void Pcsx2Config::GamefixOptions::Set(const wxString& list, bool enabled)
{
	wxStringTokenizer izer(list, L",|", wxTOKEN_STRTOK);

	while (izer.HasMoreTokens())
	{
		wxString token(izer.GetNextToken());

		GamefixId i;
		for (i = GamefixId_FIRST; i < pxEnumEnd; ++i)
		{
			if (token.CmpNoCase(EnumToString(i)) == 0)
				break;
		}
		if (i < pxEnumEnd)
			Set(i);
	}
}

void Pcsx2Config::GamefixOptions::Set(GamefixId id, bool enabled)
{
	EnumAssert(id);
	switch (id)
	{
		case Fix_VuAddSub:
			VuAddSubHack = enabled;
			break;
		case Fix_FpuMultiply:
			FpuMulHack = enabled;
			break;
		case Fix_FpuNegDiv:
			FpuNegDivHack = enabled;
			break;
		case Fix_XGKick:
			XgKickHack = enabled;
			break;
		case Fix_EETiming:
			EETimingHack = enabled;
			break;
		case Fix_SkipMpeg:
			SkipMPEGHack = enabled;
			break;
		case Fix_OPHFlag:
			OPHFlagHack = enabled;
			break;
		case Fix_DMABusy:
			DMABusyHack = enabled;
			break;
		case Fix_VIFFIFO:
			VIFFIFOHack = enabled;
			break;
		case Fix_VIF1Stall:
			VIF1StallHack = enabled;
			break;
		case Fix_GIFFIFO:
			GIFFIFOHack = enabled;
			break;
		case Fix_GoemonTlbMiss:
			GoemonTlbHack = enabled;
			break;
		case Fix_Ibit:
			IbitHack = enabled;
			break;
		case Fix_VUKickstart:
			VUKickstartHack = enabled;
			break;
		case Fix_VUOverflow:
			VUOverflowHack = enabled;
			break;
			jNO_DEFAULT;
	}
}

bool Pcsx2Config::GamefixOptions::Get(GamefixId id) const
{
	EnumAssert(id);
	switch (id)
	{
		case Fix_VuAddSub:
			return VuAddSubHack;
		case Fix_FpuMultiply:
			return FpuMulHack;
		case Fix_FpuNegDiv:
			return FpuNegDivHack;
		case Fix_XGKick:
			return XgKickHack;
		case Fix_EETiming:
			return EETimingHack;
		case Fix_SkipMpeg:
			return SkipMPEGHack;
		case Fix_OPHFlag:
			return OPHFlagHack;
		case Fix_DMABusy:
			return DMABusyHack;
		case Fix_VIFFIFO:
			return VIFFIFOHack;
		case Fix_VIF1Stall:
			return VIF1StallHack;
		case Fix_GIFFIFO:
			return GIFFIFOHack;
		case Fix_GoemonTlbMiss:
			return GoemonTlbHack;
		case Fix_Ibit:
			return IbitHack;
		case Fix_VUKickstart:
			return VUKickstartHack;
		case Fix_VUOverflow:
			return VUOverflowHack;
			jNO_DEFAULT;
	}
	return false; // unreachable, but we still need to suppress warnings >_<
}

void Pcsx2Config::GamefixOptions::LoadSave(SettingsWrapper& wrap)
{
	SettingsWrapSection("EmuCore/Gamefixes");

	SettingsWrapBitBool(VuAddSubHack);
	SettingsWrapBitBool(FpuMulHack);
	SettingsWrapBitBool(FpuNegDivHack);
	SettingsWrapBitBool(XgKickHack);
	SettingsWrapBitBool(EETimingHack);
	SettingsWrapBitBool(SkipMPEGHack);
	SettingsWrapBitBool(OPHFlagHack);
	SettingsWrapBitBool(DMABusyHack);
	SettingsWrapBitBool(VIFFIFOHack);
	SettingsWrapBitBool(VIF1StallHack);
	SettingsWrapBitBool(GIFFIFOHack);
	SettingsWrapBitBool(GoemonTlbHack);
	SettingsWrapBitBool(IbitHack);
	SettingsWrapBitBool(VUKickstartHack);
	SettingsWrapBitBool(VUOverflowHack);
}


Pcsx2Config::DebugOptions::DebugOptions()
{
	ShowDebuggerOnStart = false;
	AlignMemoryWindowStart = true;
	FontWidth = 8;
	FontHeight = 12;
	WindowWidth = 0;
	WindowHeight = 0;
	MemoryViewBytesPerRow = 16;
}

void Pcsx2Config::DebugOptions::LoadSave(SettingsWrapper& wrap)
{
	SettingsWrapSection("EmuCore/Debugger");

	SettingsWrapBitBool(ShowDebuggerOnStart);
	SettingsWrapBitBool(AlignMemoryWindowStart);
	SettingsWrapBitfield(FontWidth);
	SettingsWrapBitfield(FontHeight);
	SettingsWrapBitfield(WindowWidth);
	SettingsWrapBitfield(WindowHeight);
	SettingsWrapBitfield(MemoryViewBytesPerRow);
}

Pcsx2Config::FilenameOptions::FilenameOptions()
{
}

void Pcsx2Config::FilenameOptions::LoadSave(SettingsWrapper& wrap)
{
	SettingsWrapSection("Filenames");

	wrap.Entry(CURRENT_SETTINGS_SECTION, "BIOS", Bios, Bios);
}

void Pcsx2Config::FramerateOptions::SanityCheck()
{
	// Ensure Conformation of various options...

	NominalScalar = std::clamp(NominalScalar, 0.05, 10.0);
	TurboScalar = std::clamp(TurboScalar, 0.05, 10.0);
	SlomoScalar = std::clamp(SlomoScalar, 0.05, 10.0);
}

void Pcsx2Config::FramerateOptions::LoadSave(SettingsWrapper& wrap)
{
	SettingsWrapSection("Framerate");

//#ifndef __ANDROID__
	SettingsWrapEntry(NominalScalar);
	SettingsWrapEntry(TurboScalar);
	SettingsWrapEntry(SlomoScalar);
#if 0
	// On Android, we use strings for these..
	std::string speedValue = StringUtil::StdStringFromFormat("%f", NominalScalar);
	wrap.Entry(CURRENT_SETTINGS_SECTION, "NominalScalar", speedValue, speedValue);
	NominalScalar = StringUtil::FromChars<double>(speedValue).value_or(NominalScalar);
	speedValue = StringUtil::StdStringFromFormat("%f", TurboScalar);
	wrap.Entry(CURRENT_SETTINGS_SECTION, "TurboScalar", speedValue, speedValue);
	TurboScalar = StringUtil::FromChars<double>(speedValue).value_or(TurboScalar);
	speedValue = StringUtil::StdStringFromFormat("%f", SlomoScalar);
	wrap.Entry(CURRENT_SETTINGS_SECTION, "SlomoScalar", speedValue, speedValue);
	SlomoScalar = StringUtil::FromChars<double>(speedValue).value_or(SlomoScalar);
#endif

	SettingsWrapEntry(SkipOnLimit);
	SettingsWrapEntry(SkipOnTurbo);
}

Pcsx2Config::Pcsx2Config()
{
	bitset = 0;
	// Set defaults for fresh installs / reset settings
	McdEnableEjection = true;
	McdFolderAutoManage = true;
	EnablePatches = true;
	BackupSavestate = true;

#ifdef __WXMSW__
	McdCompressNTFS = true;
#endif

	// To be moved to FileMemoryCard pluign (someday)
	for (uint slot = 0; slot < 8; ++slot)
	{
		Mcd[slot].Enabled = !FileMcd_IsMultitapSlot(slot); // enables main 2 slots
		Mcd[slot].Filename = FileMcd_GetDefaultName(slot);

		// Folder memory card is autodetected later.
		Mcd[slot].Type = MemoryCardType::File;
	}

	GzipIsoIndexTemplate = "$(f).pindex.tmp";
}

void Pcsx2Config::LoadSave(SettingsWrapper& wrap)
{
	SettingsWrapSection("EmuCore");

	SettingsWrapBitBool(CdvdVerboseReads);
	SettingsWrapBitBool(CdvdDumpBlocks);
	SettingsWrapBitBool(CdvdShareWrite);
	SettingsWrapBitBool(EnablePatches);
	SettingsWrapBitBool(EnableCheats);
	SettingsWrapBitBool(EnableIPC);
	SettingsWrapBitBool(EnableWideScreenPatches);
#ifndef DISABLE_RECORDING
	SettingsWrapBitBool(EnableRecordingTools);
#endif
	SettingsWrapBitBool(ConsoleToStdio);
	SettingsWrapBitBool(HostFs);

	SettingsWrapBitBool(BackupSavestate);
	SettingsWrapBitBool(McdEnableEjection);
	SettingsWrapBitBool(McdFolderAutoManage);
	SettingsWrapBitBool(MultitapPort0_Enabled);
	SettingsWrapBitBool(MultitapPort1_Enabled);

	// Process various sub-components:

	Speedhacks.LoadSave(wrap);
	Cpu.LoadSave(wrap);
	GS.LoadSave(wrap);
	SPU2.LoadSave(wrap);
	Gamefixes.LoadSave(wrap);
	Profiler.LoadSave(wrap);

	Debugger.LoadSave(wrap);
	Trace.LoadSave(wrap);

	SettingsWrapEntry(GzipIsoIndexTemplate);

	// For now, this in the derived config for backwards ini compatibility.
#ifdef PCSX2_CORE
	BaseFilenames.LoadSave(wrap);
	Framerate.LoadSave(wrap);
	LoadSaveMemcards(wrap);

	SettingsWrapEntry(GzipIsoIndexTemplate);

#ifdef __WXMSW__
	SettingsWrapEntry(McdCompressNTFS);
#endif
#endif

	if (wrap.IsLoading())
	{
		CurrentAspectRatio = GS.AspectRatio;
	}
}

void Pcsx2Config::LoadSaveMemcards(SettingsWrapper& wrap)
{
	for (uint slot = 0; slot < 2; ++slot)
	{
		wrap.Entry("MemoryCards", StringUtil::StdStringFromFormat("Slot%u_Enable", slot + 1).c_str(),
			Mcd[slot].Enabled, Mcd[slot].Enabled);
		wrap.Entry("MemoryCards", StringUtil::StdStringFromFormat("Slot%u_Filename", slot + 1).c_str(),
			Mcd[slot].Filename, Mcd[slot].Filename);
	}

	for (uint slot = 2; slot < 8; ++slot)
	{
		int mtport = FileMcd_GetMtapPort(slot) + 1;
		int mtslot = FileMcd_GetMtapSlot(slot) + 1;

		wrap.Entry("MemoryCards", StringUtil::StdStringFromFormat("Multitap%u_Slot%u_Enable", mtport, mtslot).c_str(),
			Mcd[slot].Enabled, Mcd[slot].Enabled);
		wrap.Entry("MemoryCards", StringUtil::StdStringFromFormat("Multitap%u_Slot%u_Filename", mtport, mtslot).c_str(),
			Mcd[slot].Filename, Mcd[slot].Filename);
	}
}

bool Pcsx2Config::MultitapEnabled(uint port) const
{
	pxAssert(port < 2);
	return (port == 0) ? MultitapPort0_Enabled : MultitapPort1_Enabled;
}

std::string Pcsx2Config::FullpathToBios() const
{
	std::string ret;
	if (!BaseFilenames.Bios.empty())
		ret = Path::CombineStdString(EmuFolders::Bios, BaseFilenames.Bios);
	return ret;
}

wxString Pcsx2Config::FullpathToMcd(uint slot) const
{
	return Path::Combine(EmuFolders::MemoryCards, StringUtil::UTF8StringToWxString(Mcd[slot].Filename));
}

bool Pcsx2Config::operator==(const Pcsx2Config& right) const
{
	bool equal =
		OpEqu(bitset) &&
		OpEqu(Cpu) &&
		OpEqu(GS) &&
		OpEqu(Speedhacks) &&
		OpEqu(Gamefixes) &&
		OpEqu(Profiler) &&
		OpEqu(Debugger) &&
		OpEqu(Framerate) &&
		OpEqu(Trace) &&
		OpEqu(BaseFilenames) &&
		OpEqu(GzipIsoIndexTemplate);
	for (u32 i = 0; i < sizeof(Mcd) / sizeof(Mcd[0]); i++)
	{
		equal &= OpEqu(Mcd[i].Enabled);
		equal &= OpEqu(Mcd[i].Filename);
	}

	return equal;
}

void Pcsx2Config::CopyConfig(const Pcsx2Config& cfg)
{
	Cpu = cfg.Cpu;
	GS = cfg.GS;
	Speedhacks = cfg.Speedhacks;
	Gamefixes = cfg.Gamefixes;
	Profiler = cfg.Profiler;
	Debugger = cfg.Debugger;
	Trace = cfg.Trace;
	BaseFilenames = cfg.BaseFilenames;
	Framerate = cfg.Framerate;
	for (u32 i = 0; i < sizeof(Mcd) / sizeof(Mcd[0]); i++)
	{
		// Type will be File here, even if it's a folder, so we preserve the old value.
		// When the memory card is re-opened, it should redetect anyway.
		Mcd[i].Enabled = cfg.Mcd[i].Enabled;
		Mcd[i].Filename = cfg.Mcd[i].Filename;
	}

	GzipIsoIndexTemplate = cfg.GzipIsoIndexTemplate;

	CdvdVerboseReads = cfg.CdvdVerboseReads;
	CdvdDumpBlocks = cfg.CdvdDumpBlocks;
	CdvdShareWrite = cfg.CdvdShareWrite;
	EnablePatches = cfg.EnablePatches;
	EnableCheats = cfg.EnableCheats;
	EnableIPC = cfg.EnableIPC;
	EnableWideScreenPatches = cfg.EnableWideScreenPatches;
#ifndef DISABLE_RECORDING
	EnableRecordingTools = cfg.EnableRecordingTools;
#endif
	UseBOOT2Injection = cfg.UseBOOT2Injection;
	BackupSavestate = cfg.BackupSavestate;
	McdEnableEjection = cfg.McdEnableEjection;
	McdFolderAutoManage = cfg.McdFolderAutoManage;
	MultitapPort0_Enabled = cfg.MultitapPort0_Enabled;
	MultitapPort1_Enabled = cfg.MultitapPort1_Enabled;
	ConsoleToStdio = cfg.ConsoleToStdio;
	HostFs = cfg.HostFs;
#ifdef __WXMSW__
	McdCompressNTFS = cfg.McdCompressNTFS;
#endif
}

void EmuFolders::SetDefaults()
{
	Bios = DataRoot.Combine(wxDirName("bios"));
	Snapshots = DataRoot.Combine(wxDirName("snaps"));
	Savestates = DataRoot.Combine(wxDirName("sstates"));
	MemoryCards = DataRoot.Combine(wxDirName("memcards"));
	Logs = DataRoot.Combine(wxDirName("logs"));
	Cheats = DataRoot.Combine(wxDirName("cheats"));
	CheatsWS = DataRoot.Combine(wxDirName("cheats_ws"));
	Covers = DataRoot.Combine(wxDirName("covers"));
	GameSettings = DataRoot.Combine(wxDirName("gamesettings"));
	Cache = DataRoot.Combine(wxDirName("cache"));
	Resources = AppRoot.Combine(wxDirName("resources"));
}

static wxDirName LoadPathFromSettings(SettingsInterface& si, const wxDirName& root, const char* name, const char* def)
{
	std::string value = si.GetStringValue("Folders", name, def);
	wxDirName ret(value);
	if (!ret.IsAbsolute())
		ret = root.Combine(ret);
	return ret;
}

void EmuFolders::LoadConfig(SettingsInterface& si)
{
	Bios = LoadPathFromSettings(si, DataRoot, "Bios", "bios");
	Snapshots = LoadPathFromSettings(si, DataRoot, "Snapshots", "snaps");
	Savestates = LoadPathFromSettings(si, DataRoot, "Savestates", "sstates");
	MemoryCards = LoadPathFromSettings(si, DataRoot, "MemoryCards", "memcards");
	Logs = LoadPathFromSettings(si, DataRoot, "Logs", "logs");
	Cheats = LoadPathFromSettings(si, DataRoot, "Cheats", "cheats");
	CheatsWS = LoadPathFromSettings(si, DataRoot, "CheatsWS", "cheats_ws");
	Covers = LoadPathFromSettings(si, DataRoot, "Covers", "covers");
	GameSettings = LoadPathFromSettings(si, DataRoot, "GameSettings", "gamesettings");
	Cache = LoadPathFromSettings(si, DataRoot, "Cache", "cache");

	Console.WriteLn("BIOS Directory: %s", Bios.ToString().c_str().AsChar());
	Console.WriteLn("Snapshots Directory: %s", Snapshots.ToString().c_str().AsChar());
	Console.WriteLn("Savestates Directory: %s", Savestates.ToString().c_str().AsChar());
	Console.WriteLn("MemoryCards Directory: %s", MemoryCards.ToString().c_str().AsChar());
	Console.WriteLn("Logs Directory: %s", Logs.ToString().c_str().AsChar());
	Console.WriteLn("Cheats Directory: %s", Cheats.ToString().c_str().AsChar());
	Console.WriteLn("CheatsWS Directory: %s", CheatsWS.ToString().c_str().AsChar());
	Console.WriteLn("Covers Directory: %s", Covers.ToString().c_str().AsChar());
	Console.WriteLn("Game Settings Directory: %s", GameSettings.ToString().c_str().AsChar());
	Console.WriteLn("Cache Directory: %s", Cache.ToString().c_str().AsChar());
}

void EmuFolders::Save(SettingsInterface& si)
{
	// convert back to relative
	const wxString datarel(DataRoot.ToString());
	si.SetStringValue("Folders", "Bios", wxDirName::MakeAutoRelativeTo(Bios, datarel).c_str());
	si.SetStringValue("Folders", "Snapshots", wxDirName::MakeAutoRelativeTo(Snapshots, datarel).c_str());
	si.SetStringValue("Folders", "Savestates", wxDirName::MakeAutoRelativeTo(Savestates, datarel).c_str());
	si.SetStringValue("Folders", "MemoryCards", wxDirName::MakeAutoRelativeTo(MemoryCards, datarel).c_str());
	si.SetStringValue("Folders", "Logs", wxDirName::MakeAutoRelativeTo(Logs, datarel).c_str());
	si.SetStringValue("Folders", "Cheats", wxDirName::MakeAutoRelativeTo(Cheats, datarel).c_str());
	si.SetStringValue("Folders", "CheatsWS", wxDirName::MakeAutoRelativeTo(CheatsWS, datarel).c_str());
	si.SetStringValue("Folders", "Cache", wxDirName::MakeAutoRelativeTo(Cache, datarel).c_str());
}

bool EmuFolders::EnsureFoldersExist()
{
	bool result = Bios.Mkdir();
	result = Settings.Mkdir() && result;
	result = Snapshots.Mkdir() && result;
	result = Savestates.Mkdir() && result;
	result = MemoryCards.Mkdir() && result;
	result = Logs.Mkdir() && result;
	result = Cheats.Mkdir() && result;
	result = CheatsWS.Mkdir() && result;
	result = Covers.Mkdir() && result;
	result = GameSettings.Mkdir() && result;
	result = Cache.Mkdir() && result;
	return result;
}
