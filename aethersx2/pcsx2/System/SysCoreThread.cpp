/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
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
#include "Common.h"
#include "gui/App.h"
#include "IopBios.h"
#include "R5900.h"

#include "common/Timer.h"
#include "common/WindowInfo.h"
extern WindowInfo g_gs_window_info;

#include "Counters.h"
#include "GS.h"
#include "Elfheader.h"
#include "Patch.h"
#include "SysThreads.h"
#include "MTVU.h"
#include "IPC.h"
#include "FW.h"
#include "SPU2/spu2.h"
#include "DEV9/DEV9.h"
#include "USB/USB.h"
#include "MemoryCardFile.h"
#include "PerformanceMetrics.h"
#ifdef _WIN32
#include "PAD/Windows/PAD.h"
#else
#include "PAD/Linux/PAD.h"
#endif

#include "DebugTools/MIPSAnalyst.h"
#include "DebugTools/SymbolMap.h"

#include "common/PageFaultSource.h"
#include "common/Threading.h"
#include "IopBios.h"

#ifdef __WXMSW__
#include <wx/msw/wrapwin.h>
#endif

#if defined(_M_X86_32) || defined(_M_X86_64)
#include "common/emitter/x86_intrin.h"
#endif

bool g_CDVDReset = false;

namespace IPCSettings
{
	unsigned int slot = IPC_DEFAULT_SLOT;
};

// --------------------------------------------------------------------------------------
//  SysCoreThread *External Thread* Implementations
//    (Called from outside the context of this thread)
// --------------------------------------------------------------------------------------

SysCoreThread::SysCoreThread()
{
	m_name = L"EE Core";
	m_resetRecompilers = true;
	m_resetProfilers = true;
	m_resetVsyncTimers = true;
	m_resetVirtualMachine = true;

	m_hasActiveMachine = false;
}

SysCoreThread::~SysCoreThread()
{
	try
	{
		SysCoreThread::Cancel();
	}
	DESTRUCTOR_CATCHALL
}

void SysCoreThread::Cancel(bool isBlocking)
{
	m_hasActiveMachine = false;
	R3000A::ioman::reset();
	_parent::Cancel();
}

bool SysCoreThread::Cancel(const wxTimeSpan& span)
{
	m_hasActiveMachine = false;
	R3000A::ioman::reset();
	return _parent::Cancel(span);
}

void SysCoreThread::OnStart()
{
	_parent::OnStart();
}

void SysCoreThread::OnSuspendInThread()
{
	TearDownSystems(static_cast<SystemsMask>(-1)); // All systems
	GetMTGS().Suspend();
}

void SysCoreThread::Start()
{
	GSinit();
	SPU2init();
	PADinit();
	DEV9init();
	USBinit();
	_parent::Start();
}

// Resumes the core execution state, or does nothing is the core is already running.  If
// settings were changed, resets will be performed as needed and emulation state resumed from
// memory savestates.
//
// Exceptions (can occur on first call only):
//   ThreadCreationError - Insufficient system resources to create thread.
//
void SysCoreThread::OnResumeReady()
{
	if (m_resetVirtualMachine)
		m_hasActiveMachine = false;

	if (!m_hasActiveMachine)
		m_resetRecompilers = true;
}

// This function *will* reset the emulator in order to allow the specified elf file to
// take effect.  This is because it really doesn't make sense to change the elf file outside
// the context of a reset/restart.
void SysCoreThread::SetElfOverride(const wxString& elf)
{
	//pxAssertDev( !m_hasValidMachine, "Thread synchronization error while assigning ELF override." );
	m_elf_override = elf;


	Hle_SetElfPath(elf.ToUTF8());
}

// Performs a quicker reset that does not deallocate memory associated with PS2 virtual machines
// or cpu providers (recompilers).
void SysCoreThread::ResetQuick()
{
	Suspend();

	m_resetVirtualMachine = true;
	m_hasActiveMachine = false;
	R3000A::ioman::reset();
}

void SysCoreThread::Reset()
{
	ResetQuick();
	GetVmMemory().DecommitAll();
	SysClearExecutionCache();
	sApp.PostAppMethod(&Pcsx2App::leaveDebugMode);
	g_FrameCount = 0;
}


// Applies a full suite of new settings, which will automatically facilitate the necessary
// resets of the core and components.  The scope of resetting
// is determined by comparing the current settings against the new settings, so that only
// real differences are applied.
void SysCoreThread::ApplySettings(const Pcsx2Config& src)
{
	if (src == EmuConfig)
		return;

	if (!pxAssertDev(IsPaused() | IsSelf(), "CoreThread is not paused; settings cannot be applied."))
		return;

	m_resetRecompilers = (src.Cpu != EmuConfig.Cpu) || (src.Gamefixes != EmuConfig.Gamefixes) || (src.Speedhacks != EmuConfig.Speedhacks);
	m_resetProfilers = (src.Profiler != EmuConfig.Profiler);
	m_resetVsyncTimers = (src.GS != EmuConfig.GS);

	const bool gs_settings_changed = (EmuConfig.GS != src.GS);

	EmuConfig.CopyConfig(src);
	if (gs_settings_changed && GetMTGS().IsOpen())
		GetMTGS().ApplySettings();
}

// --------------------------------------------------------------------------------------
//  SysCoreThread *Worker* Implementations
//    (Called from the context of this thread only)
// --------------------------------------------------------------------------------------
bool SysCoreThread::HasPendingStateChangeRequest() const
{
	return !m_hasActiveMachine || GetMTGS().HasPendingException() || _parent::HasPendingStateChangeRequest();
}

void SysCoreThread::_reset_stuff_as_needed()
{
	// Note that resetting recompilers along with the virtual machine is only really needed
	// because of changes to the TLB.  We don't actually support the TLB, however, so rec
	// resets aren't in fact *needed* ... yet.  But might as well, no harm.  --air

	GetVmMemory().CommitAll();

	if (m_resetVirtualMachine || m_resetRecompilers || m_resetProfilers)
	{
		SysClearExecutionCache();
		memBindConditionalHandlers();
		SetCPUState(EmuConfig.Cpu.sseMXCSR, EmuConfig.Cpu.sseVUMXCSR);

		m_resetRecompilers = false;
		m_resetProfilers = false;
	}

	if (m_resetVirtualMachine)
	{
		DoCpuReset();

		m_resetVirtualMachine = false;
		m_resetVsyncTimers = false;

		ForgetLoadedPatches();
	}

	if (m_resetVsyncTimers)
	{
		UpdateVSyncRate();
		frameLimitReset();

		m_resetVsyncTimers = false;
	}
}

void SysCoreThread::DoCpuReset()
{
	AffinityAssert_AllowFromSelf(pxDiagSpot);
	cpuReset();
}

// This is called from the PS2 VM at the start of every vsync (either 59.94 or 50 hz by PS2
// clock scale, which does not correlate to the actual host machine vsync).
//
// Default tasks: Updates PADs and applies vsync patches.  Derived classes can override this
// to change either PAD and/or Patching behaviors.
//
// [TODO]: Should probably also handle profiling and debugging updates, once those are
// re-implemented.
//
void SysCoreThread::VsyncInThread()
{
	ApplyLoadedPatches(PPT_CONTINUOUSLY);
	ApplyLoadedPatches(PPT_COMBINED_0_1);
}

void SysCoreThread::GameStartingInThread()
{
	GetMTGS().SendGameCRC(ElfCRC);

	MIPSAnalyst::ScanForFunctions(R5900SymbolMap, ElfTextRange.first, ElfTextRange.first + ElfTextRange.second, true);
	R5900SymbolMap.UpdateActiveSymbols();
	R3000SymbolMap.UpdateActiveSymbols();
	sApp.PostAppMethod(&Pcsx2App::resetDebugger);

	ApplyLoadedPatches(PPT_ONCE_ON_LOAD);
	ApplyLoadedPatches(PPT_COMBINED_0_1);
#ifdef USE_SAVESLOT_UI_UPDATES
	UI_UpdateSysControls();
#endif
	if (EmuConfig.EnableIPC && m_IpcState == OFF)
	{
		m_IpcState = ON;
		m_socketIpc = std::make_unique<SocketIPC>(this, IPCSettings::slot);
	}
	if (m_IpcState == ON && m_socketIpc->m_end)
		m_socketIpc->Start();
}

bool SysCoreThread::StateCheckInThread()
{
	GetMTGS().RethrowException();
	return _parent::StateCheckInThread() && (_reset_stuff_as_needed(), true);
}

// Runs CPU cycles indefinitely, until the user or another thread requests execution to break.
// Rationale: This very short function allows an override point and solves an SEH
// "exception-type boundary" problem (can't mix SEH and C++ exceptions in the same function).
void SysCoreThread::DoCpuExecute()
{
	m_hasActiveMachine = true;
	UI_EnableSysActions();
	Cpu->Execute();
}

void SysCoreThread::ExecuteTaskInThread()
{
	Threading::EnableHiresScheduler(); // Note that *something* in SPU2 and GS also set the timer resolution to 1ms.
	m_sem_event.WaitWithoutYield();

#if defined(_M_X86_32) || defined(_M_X86_64)
	m_mxcsr_saved.bitmask = _mm_getcsr();
#elif defined(_M_ARM64)
	m_fpcr_saved.bitmask = a64_getfpcr();
#endif

	PCSX2_PAGEFAULT_PROTECT
	{
		while (true)
		{
			StateCheckInThread();
			DoCpuExecute();
		}
	}
	PCSX2_PAGEFAULT_EXCEPT;
}

void SysCoreThread::TearDownSystems(SystemsMask systemsToTearDown)
{
	if (systemsToTearDown & System_DEV9) DEV9close();
	if (systemsToTearDown & System_USB) USBclose();
	if (systemsToTearDown & System_CDVD) DoCDVDclose();
	if (systemsToTearDown & System_FW) FWclose();
	if (systemsToTearDown & System_PAD) PADclose();
	if (systemsToTearDown & System_SPU2) SPU2close();
	if (systemsToTearDown & System_MCD) FileMcd_EmuClose();

	PerformanceMetrics::SetCPUThreadTimer(Common::ThreadCPUTimer());
}

void SysCoreThread::OnResumeInThread(SystemsMask systemsToReinstate)
{
	PerformanceMetrics::SetCPUThreadTimer(Common::ThreadCPUTimer::GetForCallingThread());

	GetMTGS().WaitForOpen();
	if (systemsToReinstate & System_DEV9) DEV9open();
	if (systemsToReinstate & System_USB) USBopen(g_gs_window_info);
	if (systemsToReinstate & System_FW) FWopen();
	if (systemsToReinstate & System_SPU2) SPU2open();
	if (systemsToReinstate & System_PAD) PADopen(g_gs_window_info);
	if (systemsToReinstate & System_MCD) FileMcd_EmuOpen();
}


// Invoked by the pthread_exit or pthread_cancel.
void SysCoreThread::OnCleanupInThread()
{
	m_ExecMode = ExecMode_Closing;

	m_hasActiveMachine = false;
	m_resetVirtualMachine = true;

	R3000A::ioman::reset();
	// FIXME: temporary workaround for deadlock on exit, which actually should be a crash
	vu1Thread.WaitVU();
	USBclose();
	SPU2close();
	PADclose();
	DEV9close();
	DoCDVDclose();
	FWclose();
	FileMcd_EmuClose();
	GetMTGS().Suspend();
	USBshutdown();
	SPU2shutdown();
	PADshutdown();
	DEV9shutdown();
	GetMTGS().Cancel();

	// TODO: ARM
#if defined(_M_X86_32) || defined(_M_X86_64)
	_mm_setcsr(m_mxcsr_saved.bitmask);
#elif defined(_M_ARM64)
  a64_setfpcr(m_fpcr_saved.bitmask);
#endif
	Threading::DisableHiresScheduler();
	_parent::OnCleanupInThread();

	m_ExecMode = ExecMode_NoThreadYet;
}
