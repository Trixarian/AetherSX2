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

/*
 * ix86 public header v0.9.1
 *
 * Original Authors (v0.6.2 and prior):
 *		linuzappz <linuzappz@pcsx.net>
 *		alexey silinov
 *		goldfinger
 *		zerofrog(@gmail.com)
 *
 * Authors of v0.9.1:
 *		Jake.Stine(@gmail.com)
 *		cottonvibes(@gmail.com)
 *		sudonim(1@gmail.com)
 */

//  PCSX2's New C++ Emitter
// --------------------------------------------------------------------------------------
// To use it just include the x86Emitter namespace into your file/class/function off choice.
//
// This header file is intended for use by public code.  It includes the appropriate
// inlines and class definitions for efficient codegen.  (code internal to the emitter
// should usually use ix86_internal.h instead, and manually include the
// ix86_inlines.inl file when it is known that inlining of ModSib functions are
// wanted).
//

#pragma once

#include "common/emitter/x86types.h"
#include "common/emitter/tools.h"
#include "common/emitter/instructions.h"

// Including legacy items for now, but these should be removed eventually,
// once most code is no longer dependent on them.
#include "common/emitter/legacy_types.h"
#include "common/emitter/legacy_instructions.h"
