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

#pragma once

#include "common/emitter/tools.h"
#include "common/General.h"
#include "common/Path.h"
#include <string>

class SettingsInterface;
class SettingsWrapper;

enum class CDVD_SourceType : uint8_t;

enum GamefixId
{
	GamefixId_FIRST = 0,

	Fix_FpuMultiply = GamefixId_FIRST,
	Fix_FpuNegDiv,
	Fix_GoemonTlbMiss,
	Fix_SkipMpeg,
	Fix_OPHFlag,
	Fix_EETiming,
	Fix_DMABusy,
	Fix_GIFFIFO,
	Fix_VIFFIFO,
	Fix_VIF1Stall,
	Fix_VuAddSub,
	Fix_Ibit,
	Fix_VUKickstart,
	Fix_VUOverflow,
	Fix_XGKick,

	GamefixId_COUNT
};

// TODO - config - not a fan of the excessive use of enums and macros to make them work
// a proper object would likely make more sense (if possible).

enum SpeedhackId
{
	SpeedhackId_FIRST = 0,

	Speedhack_mvuFlag = SpeedhackId_FIRST,
	Speedhack_InstantVU1,

	SpeedhackId_COUNT
};

enum class VsyncMode
{
	Off,
	On,
	Adaptive,
};

enum class AspectRatioType : u8
{
	Stretch,
	R4_3,
	R16_9,
	MaxCount
};

enum class FMVAspectRatioSwitchType : u8
{
	Off,
	R4_3,
	R16_9,
	MaxCount
};

enum class MemoryCardType
{
	Empty,
	File,
	Folder,
	MaxCount
};

enum class LimiterModeType : u8
{
	Nominal,
	Turbo,
	Slomo,
	Unlimited,
};

enum class GSRendererType : s8
{
	Auto = -1,
	DX11 = 3,
	Null = 11,
	OGL = 12,
	SW = 13,
	VK = 14,
};

enum class GSInterlaceMode : u8
{
	None,
	WeaveTFF,
	WeaveBFF,
	BobTFF,
	BobBFF,
	BlendTFF,
	BlendBFF,
	Automatic
};

// Ordering was done to keep compatibility with older ini file.
enum class BiFiltering : u8
{
	Nearest,
	Forced,
	PS2,
	Forced_But_Sprite,
};

enum class TriFiltering : u8
{
	None,
	PS2,
	Forced,
};

enum class HWMipmapLevel : s8
{
	Automatic = -1,
	Off,
	Basic,
	Full
};

enum class CRCHackLevel : s8
{
	Automatic = -1,
	None,
	Minimum,
	Partial,
	Full,
	Aggressive
};

// Template function for casting enumerations to their underlying type
template <typename Enumeration>
typename std::underlying_type<Enumeration>::type enum_cast(Enumeration E)
{
	return static_cast<typename std::underlying_type<Enumeration>::type>(E);
}

ImplementEnumOperators(GamefixId);
ImplementEnumOperators(SpeedhackId);

//------------ DEFAULT sseMXCSR VALUES ---------------
#define DEFAULT_sseMXCSR 0xffc0 //FPU rounding > DaZ, FtZ, "chop"
#define DEFAULT_sseVUMXCSR 0xffc0 //VU  rounding > DaZ, FtZ, "chop"


// --------------------------------------------------------------------------------------
//  TraceFiltersEE
// --------------------------------------------------------------------------------------
struct TraceFiltersEE
{
	BITFIELD32()
	bool
		m_EnableAll : 1, // Master Enable switch (if false, no logs at all)
		m_EnableDisasm : 1,
		m_EnableRegisters : 1,
		m_EnableEvents : 1; // Enables logging of event-driven activity -- counters, DMAs, etc.
	BITFIELD_END

	TraceFiltersEE()
	{
		bitset = 0;
	}

	bool operator==(const TraceFiltersEE& right) const
	{
		return OpEqu(bitset);
	}

	bool operator!=(const TraceFiltersEE& right) const
	{
		return !this->operator==(right);
	}
};

// --------------------------------------------------------------------------------------
//  TraceFiltersIOP
// --------------------------------------------------------------------------------------
struct TraceFiltersIOP
{
	BITFIELD32()
	bool
		m_EnableAll : 1, // Master Enable switch (if false, no logs at all)
		m_EnableDisasm : 1,
		m_EnableRegisters : 1,
		m_EnableEvents : 1; // Enables logging of event-driven activity -- counters, DMAs, etc.
	BITFIELD_END

	TraceFiltersIOP()
	{
		bitset = 0;
	}

	bool operator==(const TraceFiltersIOP& right) const
	{
		return OpEqu(bitset);
	}

	bool operator!=(const TraceFiltersIOP& right) const
	{
		return !this->operator==(right);
	}
};

// --------------------------------------------------------------------------------------
//  TraceLogFilters
// --------------------------------------------------------------------------------------
struct TraceLogFilters
{
	// Enabled - global toggle for high volume logging.  This is effectively the equivalent to
	// (EE.Enabled() || IOP.Enabled() || SIF) -- it's cached so that we can use the macros
	// below to inline the conditional check.  This is desirable because these logs are
	// *very* high volume, and debug builds get noticably slower if they have to invoke
	// methods/accessors to test the log enable bits.  Debug builds are slow enough already,
	// so I prefer this to help keep them usable.
	bool Enabled;

	TraceFiltersEE EE;
	TraceFiltersIOP IOP;

	TraceLogFilters()
	{
		Enabled = false;
	}

	void LoadSave(SettingsWrapper& ini);

	bool operator==(const TraceLogFilters& right) const
	{
		return OpEqu(Enabled) && OpEqu(EE) && OpEqu(IOP);
	}

	bool operator!=(const TraceLogFilters& right) const
	{
		return !this->operator==(right);
	}
};

// --------------------------------------------------------------------------------------
//  Pcsx2Config class
// --------------------------------------------------------------------------------------
// This is intended to be a public class library between the core emulator and GUI only.
//
// When GUI code performs modifications of this class, it must be done with strict thread
// safety, since the emu runs on a separate thread.  Additionally many components of the
// class require special emu-side resets or state save/recovery to be applied.  Please
// use the provided functions to lock the emulation into a safe state and then apply
// chances on the necessary scope (see Core_Pause, Core_ApplySettings, and Core_Resume).
//
struct Pcsx2Config
{
	struct ProfilerOptions
	{
		BITFIELD32()
		bool
			Enabled : 1, // universal toggle for the profiler.
			RecBlocks_EE : 1, // Enables per-block profiling for the EE recompiler [unimplemented]
			RecBlocks_IOP : 1, // Enables per-block profiling for the IOP recompiler [unimplemented]
			RecBlocks_VU0 : 1, // Enables per-block profiling for the VU0 recompiler [unimplemented]
			RecBlocks_VU1 : 1; // Enables per-block profiling for the VU1 recompiler [unimplemented]
		BITFIELD_END

		// Default is Disabled, with all recs enabled underneath.
		ProfilerOptions()
			: bitset(0xfffffffe)
		{
		}
		void LoadSave(SettingsWrapper& wrap);

		bool operator==(const ProfilerOptions& right) const
		{
			return OpEqu(bitset);
		}

		bool operator!=(const ProfilerOptions& right) const
		{
			return !OpEqu(bitset);
		}
	};

	// ------------------------------------------------------------------------
	struct RecompilerOptions
	{
		BITFIELD32()
		bool
			EnableEE : 1,
			EnableIOP : 1,
			EnableVU0 : 1,
			EnableVU1 : 1;

		bool
			vuOverflow : 1,
			vuExtraOverflow : 1,
			vuSignOverflow : 1,
			vuUnderflow : 1;

		bool
			fpuOverflow : 1,
			fpuExtraOverflow : 1,
			fpuFullMode : 1;

		bool
			StackFrameChecks : 1,
			PreBlockCheckEE : 1,
			PreBlockCheckIOP : 1;
		bool
			EnableEECache : 1;
		bool
			EnableFastmem : 1;
		BITFIELD_END

		RecompilerOptions();
		void ApplySanityCheck();

		void LoadSave(SettingsWrapper& wrap);

		bool operator==(const RecompilerOptions& right) const
		{
			return OpEqu(bitset);
		}

		bool operator!=(const RecompilerOptions& right) const
		{
			return !OpEqu(bitset);
		}
	};

	// ------------------------------------------------------------------------
	struct CpuOptions
	{
		RecompilerOptions Recompiler;

		SSE_MXCSR sseMXCSR;
		SSE_MXCSR sseVUMXCSR;

		CpuOptions();
		void LoadSave(SettingsWrapper& wrap);
		void ApplySanityCheck();

		bool operator==(const CpuOptions& right) const
		{
			return OpEqu(sseMXCSR) && OpEqu(sseVUMXCSR) && OpEqu(Recompiler);
		}

		bool operator!=(const CpuOptions& right) const
		{
			return !this->operator==(right);
		}
	};

	// ------------------------------------------------------------------------
	struct GSOptions
	{
		static const char* AspectRatioNames[];
		static const char* FMVAspectRatioSwitchNames[];
		
		static const char* GetRendererName(GSRendererType type);

		BITFIELD32()
		bool
			IntegerScaling : 1,
			LinearPresent : 1,
			UseDebugDevice : 1,
			UseBlitSwapChain : 1,
			ThrottlePresentRate : 1,
			ThreadedPresentation : 1,
			OsdShowMessages : 1,
			OsdShowSpeed : 1,
			OsdShowFPS : 1,
			OsdShowCPU : 1,
			OsdShowResolution : 1,
			OsdShowGSStats : 1;

		bool
			HWDisableReadbacks : 1,
			AccurateDATE : 1,
			GPUPaletteConversion : 1,
			ConservativeFramebuffer : 1,
			AutoFlushSW : 1,
			PreloadFrameWithGSData : 1,
			WrapGSMem : 1,
			UserHacks : 1,
			UserHacks_AlignSpriteX : 1,
			UserHacks_AutoFlush : 1,
			UserHacks_CPUFBConversion : 1,
			UserHacks_DisableDepthSupport : 1,
			UserHacks_DisablePartialInvalidation : 1,
			UserHacks_DisableSafeFeatures : 1,
			UserHacks_MergePPSprite : 1,
			UserHacks_WildHack : 1,
			FXAA : 1,
			PreloadTexture : 1;
		BITFIELD_END

		int VsyncQueueSize{2};

		// forces the MTGS to execute tags/tasks in fully blocking/synchronous
		// style. Useful for debugging potential bugs in the MTGS pipeline.
		bool SynchronousMTGS{false};
		bool FrameLimitEnable{true};
		bool FrameSkipEnable{false};

		VsyncMode VsyncEnable{VsyncMode::Off};

		int FramesToDraw{2}; // number of consecutive frames (fields) to render
		int FramesToSkip{2}; // number of consecutive frames (fields) to skip

		double LimitScalar{1.0};
		double FramerateNTSC{59.94};
		double FrameratePAL{50.00};

		AspectRatioType AspectRatio{AspectRatioType::R4_3};
		FMVAspectRatioSwitchType FMVAspectRatioSwitch{FMVAspectRatioSwitchType::Off};
		GSInterlaceMode InterlaceMode{GSInterlaceMode::Automatic};

		double Zoom{100.0};
		double StretchY{100.0};
		double OffsetX{0.0};
		double OffsetY{0.0};

		double OsdScale{100.0};

		GSRendererType Renderer{GSRendererType::Auto};
		uint UpscaleMultiplier{1};

		HWMipmapLevel HWMipmap{HWMipmapLevel::Automatic};
		int SWBlending{1};
		int SWExtraThreads{2};
		int SWExtraThreadsHeight{4};
		int TVShader{ 0 };

		GSOptions();

		void LoadSave(SettingsWrapper& wrap);

		bool UseHardwareRenderer() const;
		float GetAspectRatioFloat() const;

		bool operator==(const GSOptions& right) const
		{
			return OpEqu(bitset) &&

				   OpEqu(SynchronousMTGS) &&
				   OpEqu(VsyncQueueSize) &&

				   OpEqu(FrameSkipEnable) &&
				   OpEqu(FrameLimitEnable) &&
				   OpEqu(VsyncEnable) &&

				   OpEqu(LimitScalar) &&
				   OpEqu(FramerateNTSC) &&
				   OpEqu(FrameratePAL) &&

				   OpEqu(FramesToDraw) &&
				   OpEqu(FramesToSkip) &&

				   OpEqu(AspectRatio) &&
				   OpEqu(FMVAspectRatioSwitch) &&

				   OpEqu(Zoom) &&
				   OpEqu(StretchY) &&
				   OpEqu(OffsetX) &&
				   OpEqu(OffsetY) &&
				   OpEqu(OsdScale) &&

				   OpEqu(Renderer) &&
				   OpEqu(UpscaleMultiplier) &&
				   OpEqu(HWMipmap) &&
				   OpEqu(SWBlending) &&
				   OpEqu(SWExtraThreads) &&
				   OpEqu(TVShader);
		}

		bool operator!=(const GSOptions& right) const
		{
			return !this->operator==(right);
		}
	};

	struct SPU2Options
	{
		enum class InterpolationMode
		{
			Nearest,
			Linear,
			Cubic,
			Hermite,
			CatmullRom,
			Gaussian
		};

		enum class SynchronizationMode
		{
			TimeStretch,
			ASync,
			None,
		};


		BITFIELD32()
		bool
			AdvancedVolumeControl : 1;
		BITFIELD_END

		InterpolationMode Interpolation = InterpolationMode::Gaussian;
		SynchronizationMode SynchMode = SynchronizationMode::TimeStretch;

		s32 FinalVolume = 100;
		s32 Latency{100};
		s32 SpeakerConfiguration{0};

		double VolumeAdjustC{ 0.0f };
		double VolumeAdjustFL{ 0.0f };
		double VolumeAdjustFR{ 0.0f };
		double VolumeAdjustBL{ 0.0f };
		double VolumeAdjustBR{ 0.0f };
		double VolumeAdjustSL{ 0.0f };
		double VolumeAdjustSR{ 0.0f };
		double VolumeAdjustLFE{ 0.0f };

		std::string OutputModule;

		SPU2Options();

		void LoadSave(SettingsWrapper& wrap);

		bool operator==(const SPU2Options& right) const
		{
			return OpEqu(bitset) &&

				OpEqu(Interpolation) &&
				OpEqu(SynchMode) &&

				OpEqu(FinalVolume) &&
				OpEqu(Latency) &&
				OpEqu(SpeakerConfiguration) &&

				OpEqu(VolumeAdjustC) &&
				OpEqu(VolumeAdjustFL) &&
				OpEqu(VolumeAdjustFR) &&
				OpEqu(VolumeAdjustBL) &&
				OpEqu(VolumeAdjustBR) &&
				OpEqu(VolumeAdjustSL) &&
				OpEqu(VolumeAdjustSR) &&
				OpEqu(VolumeAdjustLFE) &&

				OpEqu(OutputModule);
		}

		bool operator!=(const SPU2Options& right) const
		{
			return !this->operator==(right);
		}
	};

	// ------------------------------------------------------------------------
	// NOTE: The GUI's GameFixes panel is dependent on the order of bits in this structure.
	struct GamefixOptions
	{
		BITFIELD32()
		bool
			FpuMulHack : 1, // Tales of Destiny hangs.
			FpuNegDivHack : 1, // Gundam games messed up camera-view.
			GoemonTlbHack : 1, // Gomeon tlb miss hack. The game need to access unmapped virtual address. Instead to handle it as exception, tlb are preloaded at startup
			SkipMPEGHack : 1, // Skips MPEG videos (Katamari and other games need this)
			OPHFlagHack : 1, // Bleach Blade Battlers
			EETimingHack : 1, // General purpose timing hack.
			DMABusyHack : 1, // Denies writes to the DMAC when it's busy. This is correct behaviour but bad timing can cause problems.
			GIFFIFOHack : 1, // Enabled the GIF FIFO (more correct but slower)
			VIFFIFOHack : 1, // Pretends to fill the non-existant VIF FIFO Buffer.
			VIF1StallHack : 1, // Like above, processes FIFO data before the stall is allowed (to make sure data goes over).
			VuAddSubHack : 1, // Tri-ace games, they use an encryption algorithm that requires VU ADDI opcode to be bit-accurate.
			IbitHack : 1, // I bit hack. Needed to stop constant VU recompilation in some games
			VUKickstartHack : 1, // Gives new VU programs a slight head start and runs VU's ahead of EE to avoid VU register reading/writing issues
			VUOverflowHack : 1, // Tries to simulate overflow flag checks (not really possible on x86 without soft floats)
			XgKickHack : 1; // Erementar Gerad, adds more delay to VU XGkick instructions. Corrects the color of some graphics, but breaks Tri-ace games and others.
		BITFIELD_END

		GamefixOptions();
		void LoadSave(SettingsWrapper& wrap);
		GamefixOptions& DisableAll();

		void Set(const wxString& list, bool enabled = true);
		void Clear(const wxString& list) { Set(list, false); }

		bool Get(GamefixId id) const;
		void Set(GamefixId id, bool enabled = true);
		void Clear(GamefixId id) { Set(id, false); }

		bool operator==(const GamefixOptions& right) const
		{
			return OpEqu(bitset);
		}

		bool operator!=(const GamefixOptions& right) const
		{
			return !OpEqu(bitset);
		}
	};

	// ------------------------------------------------------------------------
	struct SpeedhackOptions
	{
		BITFIELD32()
		bool
			fastCDVD : 1, // enables fast CDVD access
			IntcStat : 1, // tells Pcsx2 to fast-forward through intc_stat waits.
			WaitLoop : 1, // enables constant loop detection and fast-forwarding
			vuFlagHack : 1, // microVU specific flag hack
			vuThread : 1, // Enable Threaded VU1
			vu1Instant : 1; // Enable Instant VU1 (Without MTVU only)
		BITFIELD_END

		s8 EECycleRate; // EE cycle rate selector (1.0, 1.5, 2.0)
		u8 EECycleSkip; // EE Cycle skip factor (0, 1, 2, or 3)

		SpeedhackOptions();
		void LoadSave(SettingsWrapper& conf);
		SpeedhackOptions& DisableAll();

		void Set(SpeedhackId id, bool enabled = true);

		bool operator==(const SpeedhackOptions& right) const
		{
			return OpEqu(bitset) && OpEqu(EECycleRate) && OpEqu(EECycleSkip);
		}

		bool operator!=(const SpeedhackOptions& right) const
		{
			return !this->operator==(right);
		}
	};

	struct DebugOptions
	{
		BITFIELD32()
		bool
			ShowDebuggerOnStart : 1;
		bool
			AlignMemoryWindowStart : 1;
		BITFIELD_END

		u8 FontWidth;
		u8 FontHeight;
		u32 WindowWidth;
		u32 WindowHeight;
		u32 MemoryViewBytesPerRow;

		DebugOptions();
		void LoadSave(SettingsWrapper& wrap);

		bool operator==(const DebugOptions& right) const
		{
			return OpEqu(bitset) && OpEqu(FontWidth) && OpEqu(FontHeight) && OpEqu(WindowWidth) && OpEqu(WindowHeight) && OpEqu(MemoryViewBytesPerRow);
		}

		bool operator!=(const DebugOptions& right) const
		{
			return !this->operator==(right);
		}
	};

	// ------------------------------------------------------------------------
	struct FramerateOptions
	{
		bool SkipOnLimit{false};
		bool SkipOnTurbo{false};

		double NominalScalar{1.0};
		double TurboScalar{2.0};
		double SlomoScalar{0.5};

		void LoadSave(SettingsWrapper& wrap);
		void SanityCheck();

		bool operator==(const FramerateOptions& right) const
		{
			return OpEqu(SkipOnLimit) && OpEqu(SkipOnTurbo) && OpEqu(NominalScalar) && OpEqu(TurboScalar) && OpEqu(SlomoScalar);
		}

		bool operator!=(const FramerateOptions& right) const
		{
			return !this->operator==(right);
		}
	};

	// ------------------------------------------------------------------------
	struct FilenameOptions
	{
		std::string Bios;

		FilenameOptions();
		void LoadSave(SettingsWrapper& wrap);

		bool operator==(const FilenameOptions& right) const
		{
			return OpEqu(Bios);
		}

		bool operator!=(const FilenameOptions& right) const
		{
			return !this->operator==(right);
		}
	};

	// ------------------------------------------------------------------------
	// Options struct for each memory card.
	//
	struct McdOptions
	{
		std::string Filename; // user-configured location of this memory card
		bool Enabled; // memory card enabled (if false, memcard will not show up in-game)
		MemoryCardType Type; // the memory card implementation that should be used
	};

	BITFIELD32()
	bool
		CdvdVerboseReads : 1, // enables cdvd read activity verbosely dumped to the console
		CdvdDumpBlocks : 1, // enables cdvd block dumping
		CdvdShareWrite : 1, // allows the iso to be modified while it's loaded
		EnablePatches : 1, // enables patch detection and application
		EnableCheats : 1, // enables cheat detection and application
		EnableIPC : 1, // enables inter-process communication
		EnableWideScreenPatches : 1,
#ifndef DISABLE_RECORDING
		EnableRecordingTools : 1,
#endif
		// when enabled uses BOOT2 injection, skipping sony bios splashes
		UseBOOT2Injection : 1,
		BackupSavestate : 1,
		// enables simulated ejection of memory cards when loading savestates
		McdEnableEjection : 1,
		McdFolderAutoManage : 1,

		MultitapPort0_Enabled : 1,
		MultitapPort1_Enabled : 1,

		ConsoleToStdio : 1,
		HostFs : 1;

	// uses automatic ntfs compression when creating new memory cards (Win32 only)
#ifdef __WXMSW__
	bool McdCompressNTFS;
#endif
	BITFIELD_END

	CpuOptions Cpu;
	GSOptions GS;
	SpeedhackOptions Speedhacks;
	GamefixOptions Gamefixes;
	ProfilerOptions Profiler;
	DebugOptions Debugger;
	FramerateOptions Framerate;
	SPU2Options SPU2;

	TraceLogFilters Trace;

	FilenameOptions BaseFilenames;

	// Memorycard options - first 2 are default slots, last 6 are multitap 1 and 2
	// slots (3 each)
	McdOptions Mcd[8];
	std::string GzipIsoIndexTemplate; // for quick-access index with gzipped ISO

	// Set at runtime, not loaded from config.
	std::string CurrentBlockdump;
	std::string CurrentIRX;
	std::string CurrentGameArgs;
	AspectRatioType CurrentAspectRatio = AspectRatioType::R4_3;
	LimiterModeType LimiterMode = LimiterModeType::Nominal;

	Pcsx2Config();
	void LoadSave(SettingsWrapper& wrap);
	void LoadSaveMemcards(SettingsWrapper& wrap);

	// TODO: Make these std::string when we remove wxFile...
	std::string FullpathToBios() const;
	wxString FullpathToMcd(uint slot) const;

	bool MultitapEnabled(uint port) const;

	VsyncMode GetEffectiveVsyncMode() const;
	float GetPresentFPSLimit() const;

	bool operator==(const Pcsx2Config& right) const;
	bool operator!=(const Pcsx2Config& right) const
	{
		return !this->operator==(right);
	}

	// You shouldn't assign to this class, because it'll mess with the runtime variables (Current...).
	// But you can still use this to copy config. Only needed until we drop wx.
	void CopyConfig(const Pcsx2Config& cfg);
};

extern Pcsx2Config EmuConfig;

namespace EmuFolders
{
	extern wxDirName AppRoot;
	extern wxDirName DataRoot;
	extern wxDirName Settings;
	extern wxDirName Bios;
	extern wxDirName Snapshots;
	extern wxDirName Savestates;
	extern wxDirName MemoryCards;
	extern wxDirName Langs;
	extern wxDirName Logs;
	extern wxDirName Cheats;
	extern wxDirName CheatsWS;
	extern wxDirName Resources;
	extern wxDirName Cache;
	extern wxDirName Covers;
	extern wxDirName GameSettings;

	// Assumes that AppRoot and DataRoot have been initialized.
	void SetDefaults();
	bool EnsureFoldersExist();
	void LoadConfig(SettingsInterface& si);
	void Save(SettingsInterface& si);
} // namespace EmuFolders

/////////////////////////////////////////////////////////////////////////////////////////
// Helper Macros for Reading Emu Configurations.
//

// ------------ CPU / Recompiler Options ---------------

#define THREAD_VU1 (EmuConfig.Cpu.Recompiler.EnableVU1 && EmuConfig.Speedhacks.vuThread)
#define INSTANT_VU1 (EmuConfig.Speedhacks.vu1Instant)
#define CHECK_EEREC (EmuConfig.Cpu.Recompiler.EnableEE)
#define CHECK_CACHE (EmuConfig.Cpu.Recompiler.EnableEECache)
#define CHECK_IOPREC (EmuConfig.Cpu.Recompiler.EnableIOP)

#ifdef _M_ARM64
#define CHECK_FASTMEM (EmuConfig.Cpu.Recompiler.EnableEE && EmuConfig.Cpu.Recompiler.EnableFastmem)
#else
#define CHECK_FASTMEM (EmuConfig.Cpu.Recompiler.EnableEE && EmuConfig.Cpu.Recompiler.EnableFastmem && false)
#endif

//------------ SPECIAL GAME FIXES!!! ---------------
#define CHECK_VUADDSUBHACK (EmuConfig.Gamefixes.VuAddSubHack) // Special Fix for Tri-ace games, they use an encryption algorithm that requires VU addi opcode to be bit-accurate.
#define CHECK_FPUMULHACK (EmuConfig.Gamefixes.FpuMulHack) // Special Fix for Tales of Destiny hangs.
#define CHECK_FPUNEGDIVHACK (EmuConfig.Gamefixes.FpuNegDivHack) // Special Fix for Gundam games messed up camera-view.
#define CHECK_XGKICKHACK (EmuConfig.Gamefixes.XgKickHack) // Special Fix for Erementar Gerad, adds more delay to VU XGkick instructions. Corrects the color of some graphics.
#define CHECK_EETIMINGHACK (EmuConfig.Gamefixes.EETimingHack) // Fix all scheduled events to happen in 1 cycle.
#define CHECK_SKIPMPEGHACK (EmuConfig.Gamefixes.SkipMPEGHack) // Finds sceMpegIsEnd pattern to tell the game the mpeg is finished (Katamari and a lot of games need this)
#define CHECK_OPHFLAGHACK (EmuConfig.Gamefixes.OPHFlagHack) // Bleach Blade Battlers
#define CHECK_DMABUSYHACK (EmuConfig.Gamefixes.DMABusyHack) // Denies writes to the DMAC when it's busy. This is correct behaviour but bad timing can cause problems.
#define CHECK_VIFFIFOHACK (EmuConfig.Gamefixes.VIFFIFOHack) // Pretends to fill the non-existant VIF FIFO Buffer.
#define CHECK_VIF1STALLHACK (EmuConfig.Gamefixes.VIF1StallHack) // Like above, processes FIFO data before the stall is allowed (to make sure data goes over).
#define CHECK_GIFFIFOHACK (EmuConfig.Gamefixes.GIFFIFOHack) // Enabled the GIF FIFO (more correct but slower)
#define CHECK_VUOVERFLOWHACK (EmuConfig.Gamefixes.VUOverflowHack) // Special Fix for Superman Returns, they check for overflows on PS2 floats which we can't do without soft floats.

//------------ Advanced Options!!! ---------------
#define CHECK_VU_OVERFLOW (EmuConfig.Cpu.Recompiler.vuOverflow)
#define CHECK_VU_EXTRA_OVERFLOW (EmuConfig.Cpu.Recompiler.vuExtraOverflow) // If enabled, Operands are clamped before being used in the VU recs
#define CHECK_VU_SIGN_OVERFLOW (EmuConfig.Cpu.Recompiler.vuSignOverflow)
#define CHECK_VU_UNDERFLOW (EmuConfig.Cpu.Recompiler.vuUnderflow)
#define CHECK_VU_EXTRA_FLAGS 0 // Always disabled now // Sets correct flags in the sVU recs

#define CHECK_FPU_OVERFLOW (EmuConfig.Cpu.Recompiler.fpuOverflow)
#define CHECK_FPU_EXTRA_OVERFLOW (EmuConfig.Cpu.Recompiler.fpuExtraOverflow) // If enabled, Operands are checked for infinities before being used in the FPU recs
#define CHECK_FPU_EXTRA_FLAGS 1 // Always enabled now // Sets D/I flags on FPU instructions
#define CHECK_FPU_FULL (EmuConfig.Cpu.Recompiler.fpuFullMode)

//------------ EE Recompiler defines - Comment to disable a recompiler ---------------

#define SHIFT_RECOMPILE // Speed majorly reduced if disabled
#define BRANCH_RECOMPILE // Speed extremely reduced if disabled - more then shift

// Disabling all the recompilers in this block is interesting, as it still runs at a reasonable rate.
// It also adds a few glitches. Really reminds me of the old Linux 64-bit version. --arcum42
#define ARITHMETICIMM_RECOMPILE
#define ARITHMETIC_RECOMPILE
#define MULTDIV_RECOMPILE
#define JUMP_RECOMPILE
#define LOADSTORE_RECOMPILE
#define MOVE_RECOMPILE
/*#define MMI_RECOMPILE
#define MMI0_RECOMPILE
#define MMI1_RECOMPILE
#define MMI2_RECOMPILE
#define MMI3_RECOMPILE*/
#define FPU_RECOMPILE
#define CP0_RECOMPILE
#define CP2_RECOMPILE

// You can't recompile ARITHMETICIMM without ARITHMETIC.
#ifndef ARITHMETIC_RECOMPILE
#undef ARITHMETICIMM_RECOMPILE
#endif

#define EE_CONST_PROP 1 // rec2 - enables constant propagation (faster)

// Change to 1 for console logs of SIF, GPU (PS1 mode) and MDEC (PS1 mode).
// These do spam a lot though!
#define PSX_EXTRALOGS 0
