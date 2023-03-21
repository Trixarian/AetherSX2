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

#include <time.h>
#include <cmath>

#include "Common.h"
#include "R3000A.h"
#include "Counters.h"
#include "IopCounters.h"

#include "GS.h"
#include "VUmicro.h"
#include "PerformanceMetrics.h"
#include "VMManager.h"
#include "Patch.h"

#include "ps2/HwInternal.h"
#include "Sio.h"

#ifndef PCSX2_CORE
#include "gui/App.h"
#endif

#ifndef DISABLE_RECORDING
#	include "Recording/InputRecordingControls.h"
#endif

using namespace Threading;

extern u8 psxhblankgate;
static const uint EECNT_FUTURE_TARGET = 0x10000000;
static int gates = 0;

uint g_FrameCount = 0;

// Counter 4 takes care of scanlines - hSync/hBlanks
// Counter 5 takes care of vSync/vBlanks
Counter counters[4];
SyncCounter hsyncCounter;
SyncCounter vsyncCounter;

u32 nextsCounter;	// records the cpuRegs.cycle value of the last call to rcntUpdate()
s32 nextCounter;	// delta from nextsCounter, in cycles, until the next rcntUpdate()

// Forward declarations needed because C/C++ both are wimpy single-pass compilers.

static void rcntStartGate(bool mode, u32 sCycle);
static void rcntEndGate(bool mode, u32 sCycle);
static void rcntWcount(int index, u32 value);
static void rcntWmode(int index, u32 value);
static void rcntWtarget(int index, u32 value);
static void rcntWhold(int index, u32 value);

// For Analog/Double Strike and Interlace modes
static bool IsInterlacedVideoMode()
{
	return (gsVideoMode == GS_VideoMode::PAL || gsVideoMode == GS_VideoMode::NTSC || gsVideoMode == GS_VideoMode::DVD_NTSC || gsVideoMode == GS_VideoMode::DVD_PAL || gsVideoMode == GS_VideoMode::HDTV_1080I);
}

void rcntReset(int index) {
	counters[index].count = 0;
	counters[index].sCycleT = cpuRegs.cycle;
}

// Updates the state of the nextCounter value (if needed) to serve
// any pending events for the given counter.
// Call this method after any modifications to the state of a counter.
static __fi void _rcntSet( int cntidx )
{
	s32 c;
	pxAssume( cntidx <= 4 );		// rcntSet isn't valid for h/vsync counters.

	const Counter& counter = counters[cntidx];

	// Stopped or special hsync gate?
	if (!counter.mode.IsCounting || (counter.mode.ClockSource == 0x3) ) return;

	// check for special cases where the overflow or target has just passed
	// (we probably missed it because we're doing/checking other things)
	if( counter.count > 0x10000 || counter.count > counter.target )
	{
		nextCounter = 4;
		return;
	}

	// nextCounter is relative to the cpuRegs.cycle when rcntUpdate() was last called.
	// However, the current _rcntSet could be called at any cycle count, so we need to take
	// that into account.  Adding the difference from that cycle count to the current one
	// will do the trick!

	c = ((0x10000 - counter.count) * counter.rate) - (cpuRegs.cycle - counter.sCycleT);
	c += cpuRegs.cycle - nextsCounter;		// adjust for time passed since last rcntUpdate();
	if (c < nextCounter)
	{
		nextCounter = c;
		cpuSetNextEvent( nextsCounter, nextCounter ); // Need to update on counter resets/target changes
	}

	// Ignore target diff if target is currently disabled.
	// (the overflow is all we care about since it goes first, and then the
	// target will be turned on afterward, and handled in the next event test).

	if( counter.target & EECNT_FUTURE_TARGET )
	{
		return;
	}
	else
	{
		c = ((counter.target - counter.count) * counter.rate) - (cpuRegs.cycle - counter.sCycleT);
		c += cpuRegs.cycle - nextsCounter;		// adjust for time passed since last rcntUpdate();
		if (c < nextCounter)
		{
			nextCounter = c;
			cpuSetNextEvent(nextsCounter, nextCounter); // Need to update on counter resets/target changes
		}
	}
}


static __fi void cpuRcntSet()
{
	int i;

	// Default to next VBlank
	nextsCounter = cpuRegs.cycle;
	nextCounter = vsyncCounter.CycleT - (cpuRegs.cycle - vsyncCounter.sCycle);

	// Also check next HSync
	s32 nextHsync = hsyncCounter.CycleT - (cpuRegs.cycle - hsyncCounter.sCycle);
	if (nextHsync < nextCounter)
		nextCounter = nextHsync;

	for (i = 0; i < 4; i++)
		_rcntSet( i );

	// sanity check!
	if( nextCounter < 0 ) nextCounter = 0;
	cpuSetNextEvent(nextsCounter, nextCounter); // Need to update on counter resets/target changes
}

void rcntInit()
{
	int i;

	g_FrameCount = 0;

	memzero(counters);

	for (i=0; i<4; i++) {
		counters[i].rate = 2;
		counters[i].target = 0xffff;
	}
	counters[0].interrupt =  9;
	counters[1].interrupt = 10;
	counters[2].interrupt = 11;
	counters[3].interrupt = 12;

	hsyncCounter.Mode = MODE_HRENDER;
	hsyncCounter.sCycle = cpuRegs.cycle;
	vsyncCounter.Mode = MODE_VRENDER;
	vsyncCounter.sCycle = cpuRegs.cycle;

	for (i=0; i<4; i++) rcntReset(i);
	cpuRcntSet();
}


#ifndef _WIN32
#include <sys/time.h>
#endif

static s64 m_iTicks=0;
static u64 m_iStart=0;

struct vSyncTimingInfo
{
	double Framerate;       // frames per second (8 bit fixed)
	GS_VideoMode VideoMode; // used to detect change (interlaced/progressive)
	u32 Render;             // time from vblank end to vblank start (cycles)
	u32 Blank;              // time from vblank start to vblank end (cycles)

	u32 GSBlank;            // GS CSR is swapped roughly 3.5 hblank's after vblank start

	u32 hSyncError;         // rounding error after the duration of a rendered frame (cycles)
	u32 hRender;            // time from hblank end to hblank start (cycles)
	u32 hBlank;             // time from hblank start to hblank end (cycles)
	u32 hScanlinesPerFrame; // number of scanlines per frame (525/625 for NTSC/PAL)
};


static vSyncTimingInfo vSyncInfo;


static void vSyncInfoCalc(vSyncTimingInfo* info, double framesPerSecond, u32 scansPerFrame)
{
	constexpr double clock = static_cast<double>(PS2CLK);

	const u64 Frame = clock * 10000ULL / framesPerSecond;
	const u64 Scanline = Frame / scansPerFrame;

	// There are two renders and blanks per frame. This matches the PS2 test results.
	// The PAL and NTSC VBlank periods respectively lasts for approximately 22 and 26 scanlines.
	// An older test suggests that these periods are actually the periods that VBlank is off, but
	// Legendz Gekitou! Saga Battle runs very slowly if the VBlank period is inverted.
	// Some of the more timing sensitive games and their symptoms when things aren't right:
	// Dynasty Warriors 3 Xtreme Legends - fake save corruption when loading save
	// Jak II - random speedups
	// Shadow of Rome - FMV audio issues
	const u64 HalfFrame = Frame / 2;
	const u64 Blank = Scanline * (gsVideoMode == GS_VideoMode::NTSC ? 22 : 26);
	const u64 Render = HalfFrame - Blank;
	const u64 GSBlank = Scanline * 3.5; // GS VBlank/CSR Swap happens roughly 3.5 Scanlines after VBlank Start

	// Important!  The hRender/hBlank timers should be 50/50 for best results.
	//  (this appears to be what the real EE's timing crystal does anyway)

	u64 hBlank = Scanline / 2;
	u64 hRender = Scanline - hBlank;

	if (!IsInterlacedVideoMode())
	{
		hBlank /= 2;
		hRender /= 2;
	}

	//TODO: Carry fixed-point math all the way through the entire vsync and hsync counting processes, and continually apply rounding
	//as needed for each scheduled v/hsync related event. Much better to handle than this messed state.
	info->Framerate = framesPerSecond;
	info->GSBlank = (u32)(GSBlank / 10000);
	info->Render = (u32)(Render / 10000);
	info->Blank = (u32)(Blank / 10000);

	info->hRender = (u32)(hRender / 10000);
	info->hBlank = (u32)(hBlank / 10000);
	info->hScanlinesPerFrame = scansPerFrame;

	if ((Render % 10000) >= 5000) info->Render++;
	if ((Blank % 10000) >= 5000) info->Blank++;

	if ((hRender % 10000) >= 5000) info->hRender++;
	if ((hBlank % 10000) >= 5000) info->hBlank++;

	// Calculate accumulative hSync rounding error per half-frame:
	if (IsInterlacedVideoMode()) // gets off the chart in that mode
	{
		u32 hSyncCycles = ((info->hRender + info->hBlank) * scansPerFrame) / 2;
		u32 vSyncCycles = (info->Render + info->Blank);
		info->hSyncError = vSyncCycles - hSyncCycles;
		//Console.Warning("%d",info->hSyncError);
	}
	else info->hSyncError = 0;
	// Note: In NTSC modes there is some small rounding error in the vsync too,
	// however it would take thousands of frames for it to amount to anything and
	// is thus not worth the effort at this time.
}

const char* ReportVideoMode()
{
	switch (gsVideoMode)
	{
	case GS_VideoMode::PAL:          return "PAL";
	case GS_VideoMode::NTSC:         return "NTSC";
	case GS_VideoMode::DVD_NTSC:     return "DVD NTSC";
	case GS_VideoMode::DVD_PAL:      return "DVD PAL";
	case GS_VideoMode::VESA:         return "VESA";
	case GS_VideoMode::SDTV_480P:    return "SDTV 480p";
	case GS_VideoMode::SDTV_576P:    return "SDTV 576p";
	case GS_VideoMode::HDTV_720P:    return "HDTV 720p";
	case GS_VideoMode::HDTV_1080I:   return "HDTV 1080i";
	case GS_VideoMode::HDTV_1080P:   return "HDTV 1080p";
	default:                         return "Unknown";
	}
}

const char* ReportInterlaceMode()
{
	const u64& smode2 = *(u64*)PS2GS_BASE(GS_SMODE2);
	return (smode2 & 1) ? ((smode2 & 2) ? "Interlaced (Frame)" : "Interlaced (Field)") : "Progressive";
}

double GetVerticalFrequency()
{
	// Note about NTSC/PAL "double strike" modes:
	// NTSC and PAL can be configured in such a way to produce a non-interlaced signal.
	// This involves modifying the signal slightly by either adding or subtracting a line (526/524 instead of 525)
	// which has the function of causing the odd and even fields to strike the same lines.
	// Doing this modifies the vertical refresh rate slightly. Beatmania is sensitive to this and
	// not accounting for it will cause the audio and video to become desynced.
	//
	// In the case of the GS, I believe it adds a halfline to the vertical back porch but more research is needed.
	// For now I'm just going to subtract off the config setting.
	//
	// According to the GS:
	// NTSC (interlaced): 59.94
	// NTSC (non-interlaced): 59.82
	// PAL (interlaced): 50.00
	// PAL (non-interlaced): 49.76
	//
	// More Information:
	// https://web.archive.org/web/20201031235528/https://wiki.nesdev.com/w/index.php/NTSC_video
	// https://web.archive.org/web/20201102100937/http://forums.nesdev.com/viewtopic.php?t=7909
	// https://web.archive.org/web/20120629231826fw_/http://ntsc-tv.com/index.html
	// https://web.archive.org/web/20200831051302/https://www.hdretrovision.com/240p/
	switch (gsVideoMode)
	{
	case GS_VideoMode::Uninitialized: // SetGsCrt hasn't executed yet, give some temporary values.
		return 60.00;
	case GS_VideoMode::PAL:
	case GS_VideoMode::DVD_PAL:
		return gsIsInterlaced ? EmuConfig.GS.FrameratePAL : EmuConfig.GS.FrameratePAL - 0.24f;
	case GS_VideoMode::NTSC:
	case GS_VideoMode::DVD_NTSC:
		return gsIsInterlaced ? EmuConfig.GS.FramerateNTSC : EmuConfig.GS.FramerateNTSC - 0.11f;
	case GS_VideoMode::SDTV_480P:
		return 59.94;
	case GS_VideoMode::HDTV_1080P:
	case GS_VideoMode::HDTV_1080I:
	case GS_VideoMode::HDTV_720P:
	case GS_VideoMode::SDTV_576P:
	case GS_VideoMode::VESA:
		return 60.00;
	default:
		// Pass NTSC vertical frequency value when unknown video mode is detected.
		return FRAMERATE_NTSC * 2;
	}
}

u32 UpdateVSyncRate()
{
	// Notice:  (and I probably repeat this elsewhere, but it's worth repeating)
	//  The PS2's vsync timer is an *independent* crystal that is fixed to either 59.94 (NTSC)
	//  or 50.0 (PAL) Hz.  It has *nothing* to do with real TV timings or the real vsync of
	//  the GS's output circuit.  It is the same regardless if the GS is outputting interlace
	//  or progressive scan content. 

	const double vertical_frequency = GetVerticalFrequency();

	const double frames_per_second = vertical_frequency / 2.0;
	const double frame_limit = frames_per_second * EmuConfig.GS.LimitScalar;

	const double tick_rate = GetTickFrequency() / 2.0;
	const s64 ticks = static_cast<s64>(tick_rate / std::max(frame_limit, 1.0));

	u32 total_scanlines = 0;
	bool custom  = false;

	switch (gsVideoMode)
	{
	case GS_VideoMode::Uninitialized: // SYSCALL instruction hasn't executed yet, give some temporary values.
		total_scanlines = SCANLINES_TOTAL_NTSC;
		break;
	case GS_VideoMode::PAL:
	case GS_VideoMode::DVD_PAL:
		custom = (EmuConfig.GS.FrameratePAL != 50.0);
		total_scanlines = SCANLINES_TOTAL_PAL;
		break;
	case GS_VideoMode::NTSC:
	case GS_VideoMode::DVD_NTSC:
		custom = (EmuConfig.GS.FramerateNTSC != 59.94);
		total_scanlines = SCANLINES_TOTAL_NTSC;
		break;
	case GS_VideoMode::SDTV_480P:
	case GS_VideoMode::SDTV_576P:
	case GS_VideoMode::HDTV_720P:
	case GS_VideoMode::VESA:
		total_scanlines = SCANLINES_TOTAL_NTSC;
		break;
	case GS_VideoMode::HDTV_1080P:
	case GS_VideoMode::HDTV_1080I:
		total_scanlines = SCANLINES_TOTAL_1080;
		break;
	case GS_VideoMode::Unknown:
	default:
		total_scanlines = SCANLINES_TOTAL_NTSC;
		Console.Error("PCSX2-Counters: Unknown video mode detected");
		pxAssertDev(false , "Unknown video mode detected via SetGsCrt");
	}

	const bool video_mode_initialized = gsVideoMode != GS_VideoMode::Uninitialized;

	if (vSyncInfo.Framerate != frames_per_second || vSyncInfo.VideoMode != gsVideoMode)
	{
		vSyncInfo.VideoMode = gsVideoMode;

		vSyncInfoCalc(&vSyncInfo, frames_per_second, total_scanlines);

		if (video_mode_initialized)
			Console.WriteLn(Color_Green, "(UpdateVSyncRate) Mode Changed to %s.", ReportVideoMode());
		
		if (custom && video_mode_initialized)
			Console.Indent().WriteLn(Color_StrongGreen, "... with user configured refresh rate: %.02f Hz", vertical_frequency);

		hsyncCounter.CycleT = vSyncInfo.hRender; // Amount of cycles before the counter will be updated
		vsyncCounter.CycleT = vSyncInfo.Render;  // Amount of cycles before the counter will be updated

		cpuRcntSet();
	}

	PerformanceMetrics::SetVerticalFrequency(vertical_frequency);

	if (m_iTicks != ticks)
		m_iTicks = ticks;

	m_iStart = GetCPUTicks();

	return static_cast<u32>(m_iTicks);
}

void frameLimitReset()
{
	m_iStart = GetCPUTicks();
}

// Convenience function to update UI thread and set patches. 
static __fi void frameLimitUpdateCore()
{
#ifndef PCSX2_CORE
	GetCoreThread().VsyncInThread();
#else
	VMManager::Internal::VSyncOnCPUThread();
#endif
	Cpu->CheckExecutionState();
}

// Framelimiter - Measures the delta time between calls and stalls until a
// certain amount of time passes if such time hasn't passed yet.
// See the GS FrameSkip function for details on why this is here and not in the GS.
static __fi void frameLimit()
{
	// Framelimiter off in settings? Framelimiter go brrr.
	if (EmuConfig.GS.LimitScalar == 0.0)
	{
		frameLimitUpdateCore();
		return;
	}

	const u64 uExpectedEnd = m_iStart + m_iTicks;  // Compute when we would expect this frame to end, assuming everything goes perfectly perfect.
	const u64 iEnd = GetCPUTicks();                // The current tick we actually stopped on.
	const s64 sDeltaTime = iEnd - uExpectedEnd;    // The diff between when we stopped and when we expected to.

	// If frame ran too long...
	if (sDeltaTime >= m_iTicks)
	{
		// ... Fudge the next frame start over a bit. Prevents fast forward zoomies.
		m_iStart += (sDeltaTime / m_iTicks) * m_iTicks;
		frameLimitUpdateCore();
		return;
	}

	// Conversion of delta from CPU ticks (microseconds) to milliseconds
	s32 msec = (int) ((sDeltaTime * -1000) / (s64) GetTickFrequency());
	
	// If any integer value of milliseconds exists, sleep it off.
	// Prior comments suggested that 1-2 ms sleeps were inaccurate on some OSes;
	// further testing suggests instead that this was utter bullshit. 
	if (msec > 1)
	{
		Threading::Sleep(msec - 1);
	}
	
#ifndef __ANDROID__
	// Conversion to milliseconds loses some precision; after sleeping off whole milliseconds,
	// spin the thread without sleeping until we finally reach our expected end time.
	while (GetCPUTicks() < uExpectedEnd)
	{
		// SKREEEEEEEE
	}
#endif

	// Finally, set our next frame start to when this one ends
	m_iStart = uExpectedEnd;
	frameLimitUpdateCore();
}

static __fi void VSyncStart(u32 sCycle)
{
#ifndef DISABLE_RECORDING
	if (g_Conf->EmuOptions.EnableRecordingTools)
	{
		// It is imperative that any frame locking that must happen occurs before Vsync is started
		// Not doing so would sacrifice a frame of a savestate-based recording when loading any savestate
		g_InputRecordingControls.HandlePausingAndLocking();
	}
#endif

	frameLimit(); // limit FPS
	gsPostVsyncStart(); // MUST be after framelimit; doing so before causes funk with frame times!

	if(EmuConfig.Trace.Enabled && EmuConfig.Trace.EE.m_EnableAll)
		SysTrace.EE.Counters.Write( "    ================  EE COUNTER VSYNC START (frame: %d)  ================", g_FrameCount );

	// EE Profiling and Debug code.
	// FIXME: should probably be moved to VsyncInThread, and handled
	// by UI implementations.  (ie, AppCoreThread in PCSX2-wx interface).
	vSyncDebugStuff( g_FrameCount );

	CpuVU0->Vsync();
	CpuVU1->Vsync();

	hwIntcIrq(INTC_VBLANK_S);
	psxVBlankStart();
	
	if (gates) rcntStartGate(true, sCycle); // Counters Start Gate code

	// INTC - VB Blank Start Hack --
	// Hack fix!  This corrects a freezeup in Granda 2 where it decides to spin
	// on the INTC_STAT register after the exception handler has already cleared
	// it.  But be warned!  Set the value to larger than 4 and it breaks Dark
	// Cloud and other games. -_-

	// How it works: Normally the INTC raises exceptions immediately at the end of the
	// current branch test.  But in the case of Grandia 2, the game's code is spinning
	// on the INTC status, and the exception handler (for some reason?) clears the INTC
	// before returning *and* returns to a location other than EPC.  So the game never
	// gets to the point where it sees the INTC Irq set true.

	// (I haven't investigated why Dark Cloud freezes on larger values)
	// (all testing done using the recompiler -- dunno how the ints respond yet)

	//cpuRegs.eCycle[30] = 2;

	// Update 08/2021: The only game I know to require this kind of thing as of 1.7.0 is Penny Racers/Gadget Racers (which has a patch to avoid the problem and others)
	// These games have a tight loop checking INTC_STAT waiting for the VBLANK Start, however the game also has a VBLANK Hander which clears it.
	// Therefore, there needs to be some delay in order for it to see the interrupt flag before the interrupt is acknowledged, likely helped on real hardware by the pipelines.
	// Without the patch and fixing this, the games have other issues, so I'm not going to rush to fix it.
	// Refraction
}

static __fi void GSVSync()
{
	// CSR is swapped and GS vBlank IRQ is triggered roughly 3.5 hblanks after VSync Start
	CSRreg.SwapField();

	if (!CSRreg.VSINT)
	{
		CSRreg.VSINT = true;
		if (!GSIMR.VSMSK)
			gsIrq();
	}
}

static __fi void VSyncEnd(u32 sCycle)
{
#ifndef DISABLE_RECORDING
	if (g_Conf->EmuOptions.EnableRecordingTools)
	{
		g_InputRecordingControls.CheckPauseStatus();
	}
#endif

	if(EmuConfig.Trace.Enabled && EmuConfig.Trace.EE.m_EnableAll)
		SysTrace.EE.Counters.Write( "    ================  EE COUNTER VSYNC END (frame: %d)  ================", g_FrameCount );

	g_FrameCount++;

	hwIntcIrq(INTC_VBLANK_E);  // HW Irq
	psxVBlankEnd(); // psxCounters vBlank End
	if (gates) rcntEndGate(true, sCycle); // Counters End Gate Code

	// FolderMemoryCard needs information on how much time has passed since the last write
	// Call it every 60 frames
	if (!(g_FrameCount % 60))
		sioNextFrame();

	// This doesn't seem to be needed here.  Games only seem to break with regard to the
	// vsyncstart irq.
	//cpuRegs.eCycle[30] = 2;
}

//#define VSYNC_DEBUG		// Uncomment this to enable some vSync Timer debugging features.
#ifdef VSYNC_DEBUG
static u32 hsc=0;
static int vblankinc = 0;
#endif

__fi void rcntUpdate_hScanline()
{
	if( !cpuTestCycle( hsyncCounter.sCycle, hsyncCounter.CycleT ) ) return;

	//iopEventAction = 1;
	if (hsyncCounter.Mode & MODE_HBLANK) { //HBLANK Start
		rcntStartGate(false, hsyncCounter.sCycle);
		psxCheckStartGate16(0);

		// Setup the hRender's start and end cycle information:
		hsyncCounter.sCycle += vSyncInfo.hBlank;		// start  (absolute cycle value)
		hsyncCounter.CycleT = vSyncInfo.hRender;		// endpoint (delta from start value)
		hsyncCounter.Mode = MODE_HRENDER;
	}
	else { //HBLANK END / HRENDER Begin
		if (!CSRreg.HSINT)
		{
			CSRreg.HSINT = true;
			if (!GSIMR.HSMSK)
				gsIrq();
		}
		if (gates) rcntEndGate(false, hsyncCounter.sCycle);
		if (psxhblankgate) psxCheckEndGate16(0);

		// set up the hblank's start and end cycle information:
		hsyncCounter.sCycle += vSyncInfo.hRender;	// start (absolute cycle value)
		hsyncCounter.CycleT = vSyncInfo.hBlank;		// endpoint (delta from start value)
		hsyncCounter.Mode = MODE_HBLANK;

#		ifdef VSYNC_DEBUG
		hsc++;
#		endif
	}
}

__fi void rcntUpdate_vSync()
{
	s32 diff = (cpuRegs.cycle - vsyncCounter.sCycle);
	if( diff < vsyncCounter.CycleT ) return;

	if (vsyncCounter.Mode == MODE_VSYNC)
	{
		VSyncEnd(vsyncCounter.sCycle);
		
		vsyncCounter.sCycle += vSyncInfo.Blank;
		vsyncCounter.CycleT = vSyncInfo.Render;
		vsyncCounter.Mode = MODE_VRENDER;
	}
	else if (vsyncCounter.Mode == MODE_GSBLANK) // GS CSR Swap and interrupt
	{
		GSVSync();

		vsyncCounter.Mode = MODE_VSYNC;
		// Don't set the start cycle, makes it easier to calculate the correct Vsync End time
		vsyncCounter.CycleT = vSyncInfo.Blank;
	}
	else	// VSYNC end / VRENDER begin
	{
		VSyncStart(vsyncCounter.sCycle);

		vsyncCounter.sCycle += vSyncInfo.Render;
		vsyncCounter.CycleT = vSyncInfo.GSBlank;
		vsyncCounter.Mode = MODE_GSBLANK;

		// Accumulate hsync rounding errors:
		hsyncCounter.sCycle += vSyncInfo.hSyncError;

#		ifdef VSYNC_DEBUG
		vblankinc++;
		if( vblankinc > 1 )
		{
			if( hsc != vSyncInfo.hScanlinesPerFrame )
				Console.WriteLn( " ** vSync > Abnormal Scanline Count: %d", hsc );
			hsc = 0;
			vblankinc = 0;
		}
#		endif
	}
}

static __fi void _cpuTestTarget( int i )
{
	if (counters[i].count < counters[i].target)
		return;

	if(counters[i].mode.TargetInterrupt) {
		EECNT_LOG("EE Counter[%d] TARGET reached - mode=%x, count=%x, target=%x", i, counters[i].mode, counters[i].count, counters[i].target);
		if (!counters[i].mode.TargetReached)
		{
			counters[i].mode.TargetReached = 1;
			hwIntcIrq(counters[i].interrupt);
		}
	}

	if (counters[i].mode.ZeroReturn)
		counters[i].count -= counters[i].target; // Reset on target
	else
		counters[i].target |= EECNT_FUTURE_TARGET; // OR with future target to prevent a retrigger
}

static __fi void _cpuTestOverflow( int i )
{
	if (counters[i].count <= 0xffff) return;

	if (counters[i].mode.OverflowInterrupt) {
		EECNT_LOG("EE Counter[%d] OVERFLOW - mode=%x, count=%x", i, counters[i].mode, counters[i].count);
		if (!counters[i].mode.OverflowReached)
		{
			counters[i].mode.OverflowReached = 1;
			hwIntcIrq(counters[i].interrupt);
		}
	}

	// wrap counter back around zero, and enable the future target:
	counters[i].count -= 0x10000;
	counters[i].target &= 0xffff;
}


// forceinline note: this method is called from two locations, but one
// of them is the interpreter, which doesn't count. ;)  So might as
// well forceinline it!
__fi void rcntUpdate()
{
	rcntUpdate_vSync();

	// Update counters so that we can perform overflow and target tests.

	for (int i=0; i<=3; i++)
	{
		// We want to count gated counters (except the hblank which exclude below, and are
		// counted by the hblank timer instead)

		//if ( gates & (1<<i) ) continue;

		if (!counters[i].mode.IsCounting ) continue;

		if(counters[i].mode.ClockSource != 0x3)	// don't count hblank sources
		{
			s32 change = cpuRegs.cycle - counters[i].sCycleT;
			if( change < 0 ) change = 0;	// sanity check!

			counters[i].count += change / counters[i].rate;
			change -= (change / counters[i].rate) * counters[i].rate;
			counters[i].sCycleT = cpuRegs.cycle - change;

			// Check Counter Targets and Overflows:
			_cpuTestTarget( i );
			_cpuTestOverflow( i );
		}
		else counters[i].sCycleT = cpuRegs.cycle;
	}

	cpuRcntSet();
}

static __fi void _rcntSetGate( int index )
{
	if (counters[index].mode.EnableGate)
	{
		// If the Gate Source is hblank and the clock selection is also hblank
		// then the gate is disabled and the counter acts as a normal hblank source.

		if( !(counters[index].mode.GateSource == 0 && counters[index].mode.ClockSource == 3) )
		{
			EECNT_LOG( "EE Counter[%d] Using Gate!  Source=%s, Mode=%d.",
				index, counters[index].mode.GateSource ? "vblank" : "hblank", counters[index].mode.GateMode );

			gates |= (1<<index);
			counters[index].mode.IsCounting = 0;
			rcntReset(index);
			return;
		}
		else
			EECNT_LOG( "EE Counter[%d] GATE DISABLED because of hblank source.", index );
	}

	gates &= ~(1<<index);
}

// mode - 0 means hblank source, 8 means vblank source.
static __fi void rcntStartGate(bool isVblank, u32 sCycle)
{
	int i;

	for (i=0; i <=3; i++)
	{
		//if ((mode == 0) && ((counters[i].mode & 0x83) == 0x83))
		if (!isVblank && counters[i].mode.IsCounting && (counters[i].mode.ClockSource == 3) )
		{
			// Update counters using the hblank as the clock.  This keeps the hblank source
			// nicely in sync with the counters and serves as an optimization also, since these
			// counter won't recieve special rcntUpdate scheduling.

			// Note: Target and overflow tests must be done here since they won't be done
			// currectly by rcntUpdate (since it's not being scheduled for these counters)

			counters[i].count += HBLANK_COUNTER_SPEED;
			_cpuTestTarget( i );
			_cpuTestOverflow( i );
		}

		if (!(gates & (1<<i))) continue;
		if ((!!counters[i].mode.GateSource) != isVblank) continue;

		switch (counters[i].mode.GateMode) {
			case 0x0: //Count When Signal is low (off)

				// Just set the start cycle (sCycleT) -- counting will be done as needed
				// for events (overflows, targets, mode changes, and the gate off below)

				counters[i].count = rcntRcount(i);
				counters[i].mode.IsCounting = 0;
				counters[i].sCycleT = sCycle;
				EECNT_LOG("EE Counter[%d] %s StartGate Type0, count = %x", i,
					isVblank ? "vblank" : "hblank", counters[i].count );
				break;

			case 0x2:	// reset and start counting on vsync end
				// this is the vsync start so do nothing.
				break;

			case 0x1: //Reset and start counting on Vsync start
			case 0x3: //Reset and start counting on Vsync start and end
				counters[i].mode.IsCounting = 1;
				counters[i].count = 0;
				counters[i].target &= 0xffff;
				counters[i].sCycleT = sCycle;
				EECNT_LOG("EE Counter[%d] %s StartGate Type%d, count = %x", i,
					isVblank ? "vblank" : "hblank", counters[i].mode.GateMode, counters[i].count );
				break;
		}
	}

	// No need to update actual counts here.  Counts are calculated as needed by reads to
	// rcntRcount().  And so long as sCycleT is set properly, any targets or overflows
	// will be scheduled and handled.

	// Note: No need to set counters here.  They'll get set when control returns to
	// rcntUpdate, since we're being called from there anyway.
}

// mode - 0 means hblank signal, 8 means vblank signal.
static __fi void rcntEndGate(bool isVblank , u32 sCycle)
{
	int i;

	for(i=0; i <=3; i++) { //Gates for counters
		if (!(gates & (1<<i))) continue;
		if ((!!counters[i].mode.GateSource) != isVblank) continue;

		switch (counters[i].mode.GateMode) {
			case 0x0: //Count When Signal is low (off)

				// Set the count here.  Since the timer is being turned off it's
				// important to record its count at this point (it won't be counted by
				// calls to rcntUpdate).
				counters[i].mode.IsCounting = 1;
				counters[i].sCycleT = cpuRegs.cycle;
				
				EECNT_LOG("EE Counter[%d] %s EndGate Type0, count = %x", i,
					isVblank ? "vblank" : "hblank", counters[i].count );
			break;

			case 0x1:	// Reset and start counting on Vsync start
				// this is the vsync end so do nothing
			break;

			case 0x2: //Reset and start counting on Vsync end
			case 0x3: //Reset and start counting on Vsync start and end
				counters[i].mode.IsCounting = 1;
				counters[i].count = 0;
				counters[i].target &= 0xffff;
				counters[i].sCycleT = sCycle;
				EECNT_LOG("EE Counter[%d] %s EndGate Type%d, count = %x", i,
					isVblank ? "vblank" : "hblank", counters[i].mode.GateMode, counters[i].count );
			break;
		}
	}
	// Note: No need to set counters here.  They'll get set when control returns to
	// rcntUpdate, since we're being called from there anyway.
}

static __fi u32 rcntCycle(int index)
{
	if (counters[index].mode.IsCounting && (counters[index].mode.ClockSource != 0x3))
		return counters[index].count + ((cpuRegs.cycle - counters[index].sCycleT) / counters[index].rate);
	else
		return counters[index].count;
}

static __fi void rcntWmode(int index, u32 value)
{
	if(counters[index].mode.IsCounting) {
		if(counters[index].mode.ClockSource != 0x3) {

			u32 change = cpuRegs.cycle - counters[index].sCycleT;
			if( change > 0 )
			{
				counters[index].count += change / counters[index].rate;
				change -= (change / counters[index].rate) * counters[index].rate;
				counters[index].sCycleT = cpuRegs.cycle - change;
			}
		}
	}
	else counters[index].sCycleT = cpuRegs.cycle;

	// Clear OverflowReached and TargetReached flags (0xc00 mask), but *only* if they are set to 1 in the
	// given value.  (yes, the bits are cleared when written with '1's).

	counters[index].modeval &= ~(value & 0xc00);
	counters[index].modeval = (counters[index].modeval & 0xc00) | (value & 0x3ff);
	EECNT_LOG("EE Counter[%d] writeMode = %x   passed value=%x", index, counters[index].modeval, value );

	switch (counters[index].mode.ClockSource) { //Clock rate divisers *2, they use BUSCLK speed not PS2CLK
		case 0: counters[index].rate = 2; break;
		case 1: counters[index].rate = 32; break;
		case 2: counters[index].rate = 512; break;
		case 3: counters[index].rate = vSyncInfo.hBlank+vSyncInfo.hRender; break;
	}

	_rcntSetGate( index );
	_rcntSet( index );
}

static __fi void rcntWcount(int index, u32 value)
{
	EECNT_LOG("EE Counter[%d] writeCount = %x,   oldcount=%x, target=%x", index, value, counters[index].count, counters[index].target );

	counters[index].count = value & 0xffff;

	// reset the target, and make sure we don't get a premature target.
	counters[index].target &= 0xffff;
	if( counters[index].count > counters[index].target )
		counters[index].target |= EECNT_FUTURE_TARGET;

	// re-calculate the start cycle of the counter based on elapsed time since the last counter update:
	if(counters[index].mode.IsCounting) {
		if(counters[index].mode.ClockSource != 0x3) {
			s32 change = cpuRegs.cycle - counters[index].sCycleT;
			if( change > 0 ) {
				change -= (change / counters[index].rate) * counters[index].rate;
				counters[index].sCycleT = cpuRegs.cycle - change;
			}
		}
	}
	else counters[index].sCycleT = cpuRegs.cycle;

	_rcntSet( index );
}

static __fi void rcntWtarget(int index, u32 value)
{
	EECNT_LOG("EE Counter[%d] writeTarget = %x", index, value);

	counters[index].target = value & 0xffff;

	// guard against premature (instant) targeting.
	// If the target is behind the current count, set it up so that the counter must
	// overflow first before the target fires:

	if(counters[index].mode.IsCounting) {
		if(counters[index].mode.ClockSource != 0x3) {

			u32 change = cpuRegs.cycle - counters[index].sCycleT;
			if( change > 0 )
			{
				counters[index].count += change / counters[index].rate;
				change -= (change / counters[index].rate) * counters[index].rate;
				counters[index].sCycleT = cpuRegs.cycle - change;
			}
		}
	}

	if( counters[index].target <= rcntCycle(index) )
		counters[index].target |= EECNT_FUTURE_TARGET;

	_rcntSet( index );
}

static __fi void rcntWhold(int index, u32 value)
{
	EECNT_LOG("EE Counter[%d] Hold Write = %x", index, value);
	counters[index].hold = value;
}

__fi u32 rcntRcount(int index)
{
	u32 ret;

	// only count if the counter is turned on (0x80) and is not an hsync gate (!0x03)
	if (counters[index].mode.IsCounting && (counters[index].mode.ClockSource != 0x3))
		ret = counters[index].count + ((cpuRegs.cycle - counters[index].sCycleT) / counters[index].rate);
	else
		ret = counters[index].count;

	// Spams the Console.
	EECNT_LOG("EE Counter[%d] readCount32 = %x", index, ret);
	return ret;
}

template< uint page >
__fi u16 rcntRead32( u32 mem )
{
	// Important DevNote:
	// Yes this uses a u16 return value on purpose!  The upper bits 16 of the counter registers
	// are all fixed to 0, so we always truncate everything in these two pages using a u16
	// return value! --air

	iswitch( mem ) {
	icase(RCNT0_COUNT)	return (u16)rcntRcount(0);
	icase(RCNT0_MODE)	return (u16)counters[0].modeval;
	icase(RCNT0_TARGET)	return (u16)counters[0].target;
	icase(RCNT0_HOLD)	return (u16)counters[0].hold;

	icase(RCNT1_COUNT)	return (u16)rcntRcount(1);
	icase(RCNT1_MODE)	return (u16)counters[1].modeval;
	icase(RCNT1_TARGET)	return (u16)counters[1].target;
	icase(RCNT1_HOLD)	return (u16)counters[1].hold;

	icase(RCNT2_COUNT)	return (u16)rcntRcount(2);
	icase(RCNT2_MODE)	return (u16)counters[2].modeval;
	icase(RCNT2_TARGET)	return (u16)counters[2].target;

	icase(RCNT3_COUNT)	return (u16)rcntRcount(3);
	icase(RCNT3_MODE)	return (u16)counters[3].modeval;
	icase(RCNT3_TARGET)	return (u16)counters[3].target;
	}
	
	return psHu16(mem);
}

template< uint page >
__fi bool rcntWrite32( u32 mem, mem32_t& value )
{
	pxAssume( mem >= RCNT0_COUNT && mem < 0x10002000 );

	// [TODO] : counters should actually just use the EE's hw register space for storing
	// count, mode, target, and hold. This will allow for a simplified handler for register
	// reads.

	iswitch( mem ) {
	icase(RCNT0_COUNT)	return rcntWcount(0, value),	false;
	icase(RCNT0_MODE)	return rcntWmode(0, value),		false;
	icase(RCNT0_TARGET)	return rcntWtarget(0, value),	false;
	icase(RCNT0_HOLD)	return rcntWhold(0, value),		false;

	icase(RCNT1_COUNT)	return rcntWcount(1, value),	false;
	icase(RCNT1_MODE)	return rcntWmode(1, value),		false;
	icase(RCNT1_TARGET)	return rcntWtarget(1, value),	false;
	icase(RCNT1_HOLD)	return rcntWhold(1, value),		false;

	icase(RCNT2_COUNT)	return rcntWcount(2, value),	false;
	icase(RCNT2_MODE)	return rcntWmode(2, value),		false;
	icase(RCNT2_TARGET)	return rcntWtarget(2, value),	false;

	icase(RCNT3_COUNT)	return rcntWcount(3, value),	false;
	icase(RCNT3_MODE)	return rcntWmode(3, value),		false;
	icase(RCNT3_TARGET)	return rcntWtarget(3, value),	false;
	}

	// unhandled .. do memory writeback.
	return true;
}

template u16 rcntRead32<0x00>( u32 mem );
template u16 rcntRead32<0x01>( u32 mem );

template bool rcntWrite32<0x00>( u32 mem, mem32_t& value );
template bool rcntWrite32<0x01>( u32 mem, mem32_t& value );

void SaveStateBase::rcntFreeze()
{
	Freeze( counters );
	Freeze( hsyncCounter );
	Freeze( vsyncCounter );
	Freeze( nextCounter );
	Freeze( nextsCounter );
	Freeze( vSyncInfo );
	Freeze( gsVideoMode );
	Freeze( gsIsInterlaced );
	Freeze( gates );

	if( IsLoading() )
		cpuRcntSet();
}
