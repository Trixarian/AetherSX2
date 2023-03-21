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

#pragma once

#include "common/Threading.h"
#include "common/ScopedPtrMT.h"
#include "common/EventSource.h"

namespace Threading
{

	// --------------------------------------------------------------------------------------
	//  ThreadDeleteEvent
	// --------------------------------------------------------------------------------------
	class EventListener_Thread : public IEventDispatcher<int>
	{
	public:
		typedef int EvtParams;

	protected:
		pxThread* m_thread;

	public:
		EventListener_Thread()
		{
			m_thread = NULL;
		}

		virtual ~EventListener_Thread() = default;

		void SetThread(pxThread& thr) { m_thread = &thr; }
		void SetThread(pxThread* thr) { m_thread = thr; }

		void DispatchEvent(const int& params)
		{
			OnThreadCleanup();
		}

	protected:
		// Invoked by the pxThread when the thread execution is ending.  This is
		// typically more useful than a delete listener since the extended thread information
		// provided by virtualized functions/methods will be available.
		// Important!  This event is executed *by the thread*, so care must be taken to ensure
		// thread sync when necessary (posting messages to the main thread, etc).
		virtual void OnThreadCleanup() = 0;
	};

	/// Set the name of the current thread
	void SetNameOfCurrentThread(const char* name);

	void SetAffinityForCurrentThread(u64 processor_mask);

	// --------------------------------------------------------------------------------------
	// pxThread - Helper class for the basics of starting/managing persistent threads.
	// --------------------------------------------------------------------------------------
	// This class is meant to be a helper for the typical threading model of "start once and
	// reuse many times."  This class incorporates a lot of extra overhead in stopping and
	// starting threads, but in turn provides most of the basic thread-safety and event-handling
	// functionality needed for a threaded operation.  In practice this model is usually an
	// ideal one for efficiency since Operating Systems themselves typically subscribe to a
	// design where sleeping, suspending, and resuming threads is very efficient, but starting
	// new threads has quite a bit of overhead.
	//
	// To use this as a base class for your threaded procedure, overload the following virtual
	// methods:
	//  void OnStart();
	//  void ExecuteTaskInThread();
	//  void OnCleanupInThread();
	//
	// Use the public methods Start() and Cancel() to start and shutdown the thread, and use
	// m_sem_event internally to post/receive events for the thread (make a public accessor for
	// it in your derived class if your thread utilizes the post).
	//
	// Notes:
	//  * Constructing threads as static global vars isn't recommended since it can potentially
	//    confuse w32pthreads, if the static initializers are executed out-of-order (C++ offers
	//    no dependency options for ensuring correct static var initializations).  Use heap
	//    allocation to create thread objects instead.
	//
	class pxThread
	{
		DeclareNoncopyableObject(pxThread);

		friend void pxYield(int ms);

	protected:
		wxString m_name; // diagnostic name for our thread.
		pthread_t m_thread;
		uptr m_native_id; // typically an id, but implementing platforms can do whatever.
		uptr m_native_handle; // typically a pointer/handle, but implementing platforms can do whatever.

		Semaphore m_sem_event; // general wait event that's needed by most threads
		Semaphore m_sem_startup; // startup sync tool
		Mutex m_mtx_InThread; // used for canceling and closing threads in a deadlock-safe manner
		MutexRecursive m_mtx_start; // used to lock the Start() code from starting simultaneous threads accidentally.
		Mutex m_mtx_ThreadName;

		std::atomic<bool> m_detached; // a boolean value which indicates if the m_thread handle is valid
		std::atomic<bool> m_running; // set true by Start(), and set false by Cancel(), Block(), etc.

		// exception handle, set non-NULL if the thread terminated with an exception
		// Use RethrowException() to re-throw the exception using its original exception type.
		ScopedPtrMT<BaseException> m_except;

		EventSource<EventListener_Thread> m_evtsrc_OnDelete;

		u32 m_stack_size = 0;


	public:
		virtual ~pxThread();
		pxThread(const wxString& name = L"pxThread");

		pthread_t GetId() const { return m_thread; }
		u64 GetCpuTime() const;

		virtual void Start();
		virtual void Cancel(bool isBlocking = true);
		virtual bool Cancel(const wxTimeSpan& timeout);
		virtual bool Detach();
		virtual void Block();
		virtual bool Block(const wxTimeSpan& timeout);
		virtual void RethrowException() const;

		void AddListener(EventListener_Thread& evt);
		void AddListener(EventListener_Thread* evt)
		{
			if (evt == NULL)
				return;
			AddListener(*evt);
		}

		void WaitOnSelf(Semaphore& mutex) const;
		void WaitOnSelf(Mutex& mutex) const;
		bool WaitOnSelf(Semaphore& mutex, const wxTimeSpan& timeout) const;
		bool WaitOnSelf(Mutex& mutex, const wxTimeSpan& timeout) const;

		bool IsRunning() const;
		bool IsSelf() const;
		bool HasPendingException() const { return !!m_except; }

		wxString GetName() const;
		void SetName(const wxString& newname);

		void SetAffinity(u64 processor_mask);

	protected:
		// Extending classes should always implement your own OnStart(), which is called by
		// Start() once necessary locks have been obtained.  Do not override Start() directly
		// unless you're really sure that's what you need to do. ;)
		virtual void OnStart();

		virtual void OnStartInThread();

		// This is called when the thread has been canceled or exits normally.  The pxThread
		// automatically binds it to the pthread cleanup routines as soon as the thread starts.
		virtual void OnCleanupInThread();

		// Implemented by derived class to perform actual threaded task!
		virtual void ExecuteTaskInThread() = 0;

		void TestCancel() const;

		// Yields this thread to other threads and checks for cancellation.  A sleeping thread should
		// always test for cancellation, however if you really don't want to, you can use Threading::Sleep()
		// or better yet, disable cancellation of the thread completely with DisableCancellation().
		//
		// Parameters:
		//   ms - 'minimum' yield time in milliseconds (rough -- typically yields are longer by 1-5ms
		//         depending on operating system/platform).  If ms is 0 or unspecified, then a single
		//         timeslice is yielded to other contending threads.  If no threads are contending for
		//         time when ms==0, then no yield is done, but cancellation is still tested.
		void Yield(int ms = 0)
		{
			pxAssert(IsSelf());
			Threading::Sleep(ms);
			TestCancel();
		}

		void FrankenMutex(Mutex& mutex);

		bool AffinityAssert_AllowFromSelf(const DiagnosticOrigin& origin) const;
		bool AffinityAssert_DisallowFromSelf(const DiagnosticOrigin& origin) const;

		// ----------------------------------------------------------------------------
		// Section of methods for internal use only.

		void _platform_specific_OnStartInThread();
		void _platform_specific_OnCleanupInThread();
		bool _basecancel();
		void _selfRunningTest(const wxChar* name) const;
		void _DoSetThreadName(const wxString& name);
		void _DoSetThreadName(const char* name) { SetNameOfCurrentThread(name); }
		void _internal_execute();
		void _try_virtual_invoke(void (pxThread::*method)());
		void _ThreadCleanup();

		static void* _internal_callback(void* func);
		static void internal_callback_helper(void* func);
		static void _pt_callback_cleanup(void* handle);
	};
} // namespace Threading
