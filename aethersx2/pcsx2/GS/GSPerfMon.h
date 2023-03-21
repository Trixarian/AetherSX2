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

#include "GS_types.h"

class GSPerfMon
{
public:
	enum timer_t
	{
		Main,
		Sync,
		WorkerDraw0, WorkerDraw1, WorkerDraw2, WorkerDraw3, WorkerDraw4, WorkerDraw5, WorkerDraw6, WorkerDraw7,
		WorkerDraw8, WorkerDraw9, WorkerDraw10, WorkerDraw11, WorkerDraw12, WorkerDraw13, WorkerDraw14, WorkerDraw15,
		TimerLast,
	};

	enum counter_t
	{
		Prim,
		Draw,
		DrawCalls,
		Readbacks,
		Swizzle,
		Unswizzle,
		Fillrate,
		Quad,
		SyncPoint,
		CounterLast,

		// Reused counters for HW.
		TextureCopies = Fillrate,
		TextureUploads = SyncPoint,
	};

protected:
	double m_counters[CounterLast];
	double m_stats[CounterLast];
	float m_timer_stats[TimerLast];
	uint64 m_begin[TimerLast], m_total[TimerLast], m_start[TimerLast];
	uint64 m_frame;
	clock_t m_lastframe;
	int m_count;

	friend class GSPerfMonAutoTimer;

public:
	GSPerfMon();

	void SetFrame(uint64 frame) { m_frame = frame; }
	uint64 GetFrame() { return m_frame; }
	void EndFrame();

	void Put(counter_t c, double val = 0) { m_counters[c] += val; }
	double Get(counter_t c) { return m_stats[c]; }
	float GetTimer(timer_t t) { return m_timer_stats[t]; }
	void Update();

	void Start(int timer = Main);
	void Stop(int timer = Main);
};

class GSPerfMonAutoTimer
{
	GSPerfMon* m_pm;
	int m_timer;

public:
	GSPerfMonAutoTimer(GSPerfMon* pm, int timer = GSPerfMon::Main)
	{
		m_timer = timer;
		(m_pm = pm)->Start(m_timer);
	}
	~GSPerfMonAutoTimer() { m_pm->Stop(m_timer); }
};

extern GSPerfMon g_perfmon;