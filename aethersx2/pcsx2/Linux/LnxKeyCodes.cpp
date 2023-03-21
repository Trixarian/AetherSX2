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
#include "gui/ConsoleLogger.h"

#include <gdk/gdkkeysyms.h>

void NTFS_CompressFile( const wxString& file, bool compressStatus ) {}

// Returns a WXK_* keycode, given some kinda GDK input mess!
int TranslateGDKtoWXK( u32 keysym )
{
    int key_code;

    switch ( keysym )
    {
        // Shift, Control and Alt don't generate the CHAR events at all
        case GDK_KEY_Shift_L:
        case GDK_KEY_Shift_R:
            key_code = WXK_SHIFT;
            break;
        case GDK_KEY_Control_L:
        case GDK_KEY_Control_R:
            key_code = WXK_CONTROL;
            break;
        case GDK_KEY_Meta_L:
        case GDK_KEY_Meta_R:
        case GDK_KEY_Alt_L:
        case GDK_KEY_Alt_R:
        case GDK_KEY_Super_L:
        case GDK_KEY_Super_R:
            key_code = WXK_ALT;
            break;

        // neither do the toggle modifies
        case GDK_KEY_Scroll_Lock:
            key_code = WXK_SCROLL;
            break;

        case GDK_KEY_Caps_Lock:
            key_code = WXK_CAPITAL;
            break;

        case GDK_KEY_Num_Lock:
            key_code = WXK_NUMLOCK;
            break;


        // various other special keys
        case GDK_KEY_Menu:
            key_code = WXK_MENU;
            break;

        case GDK_KEY_Help:
            key_code = WXK_HELP;
            break;

        case GDK_KEY_BackSpace:
            key_code = WXK_BACK;
            break;

        case GDK_KEY_ISO_Left_Tab:
        case GDK_KEY_Tab:
            key_code = WXK_TAB;
            break;

        case GDK_KEY_Linefeed:
        case GDK_KEY_Return:
            key_code = WXK_RETURN;
            break;

        case GDK_KEY_Clear:
            key_code = WXK_CLEAR;
            break;

        case GDK_KEY_Pause:
            key_code = WXK_PAUSE;
            break;

        case GDK_KEY_Select:
            key_code = WXK_SELECT;
            break;

        case GDK_KEY_Print:
            key_code = WXK_PRINT;
            break;

        case GDK_KEY_Execute:
            key_code = WXK_EXECUTE;
            break;

        case GDK_KEY_Escape:
            key_code = WXK_ESCAPE;
            break;

        // cursor and other extended keyboard keys
        case GDK_KEY_Delete:
            key_code = WXK_DELETE;
            break;

        case GDK_KEY_Home:
            key_code = WXK_HOME;
            break;

        case GDK_KEY_Left:
            key_code = WXK_LEFT;
            break;

        case GDK_KEY_Up:
            key_code = WXK_UP;
            break;

        case GDK_KEY_Right:
            key_code = WXK_RIGHT;
            break;

        case GDK_KEY_Down:
            key_code = WXK_DOWN;
            break;

        case GDK_KEY_Prior:     // == GDK_KEY_Page_Up
            key_code = WXK_PAGEUP;
            break;

        case GDK_KEY_Next:      // == GDK_KEY_Page_Down
            key_code = WXK_PAGEDOWN;
            break;

        case GDK_KEY_End:
            key_code = WXK_END;
            break;

        case GDK_KEY_Begin:
            key_code = WXK_HOME;
            break;

        case GDK_KEY_Insert:
            key_code = WXK_INSERT;
            break;


        // numpad keys
        case GDK_KEY_KP_0:
        case GDK_KEY_KP_1:
        case GDK_KEY_KP_2:
        case GDK_KEY_KP_3:
        case GDK_KEY_KP_4:
        case GDK_KEY_KP_5:
        case GDK_KEY_KP_6:
        case GDK_KEY_KP_7:
        case GDK_KEY_KP_8:
        case GDK_KEY_KP_9:
            key_code = WXK_NUMPAD0 + keysym - GDK_KEY_KP_0;
            break;

        case GDK_KEY_KP_Space:
            key_code = WXK_NUMPAD_SPACE;
            break;

        case GDK_KEY_KP_Tab:
            key_code = WXK_NUMPAD_TAB;
            break;

        case GDK_KEY_KP_Enter:
            key_code = WXK_NUMPAD_ENTER;
            break;

        case GDK_KEY_KP_F1:
            key_code = WXK_NUMPAD_F1;
            break;

        case GDK_KEY_KP_F2:
            key_code = WXK_NUMPAD_F2;
            break;

        case GDK_KEY_KP_F3:
            key_code = WXK_NUMPAD_F3;
            break;

        case GDK_KEY_KP_F4:
            key_code = WXK_NUMPAD_F4;
            break;

        case GDK_KEY_KP_Home:
            key_code = WXK_NUMPAD_HOME;
            break;

        case GDK_KEY_KP_Left:
            key_code = WXK_NUMPAD_LEFT;
            break;

        case GDK_KEY_KP_Up:
            key_code = WXK_NUMPAD_UP;
            break;

        case GDK_KEY_KP_Right:
            key_code = WXK_NUMPAD_RIGHT;
            break;

        case GDK_KEY_KP_Down:
            key_code = WXK_NUMPAD_DOWN;
            break;

        case GDK_KEY_KP_Prior: // == GDK_KEY_KP_Page_Up
            key_code = WXK_NUMPAD_PAGEUP;
            break;

        case GDK_KEY_KP_Next: // == GDK_KEY_KP_Page_Down
            key_code = WXK_NUMPAD_PAGEDOWN;
            break;

        case GDK_KEY_KP_End:
            key_code = WXK_NUMPAD_END;
            break;

        case GDK_KEY_KP_Begin:
            key_code = WXK_NUMPAD_BEGIN;
            break;

        case GDK_KEY_KP_Insert:
            key_code = WXK_NUMPAD_INSERT;
            break;

        case GDK_KEY_KP_Delete:
            key_code = WXK_NUMPAD_DELETE;
            break;

        case GDK_KEY_KP_Equal:
            key_code = WXK_NUMPAD_EQUAL;
            break;

        case GDK_KEY_KP_Multiply:
            key_code = WXK_NUMPAD_MULTIPLY;
            break;

        case GDK_KEY_KP_Add:
            key_code = WXK_NUMPAD_ADD;
            break;

        case GDK_KEY_KP_Separator:
            // FIXME: what is this?
            //
            // Some numeric keyboards have a comma on them. I believe this is the symbol
            // for the comma, to distinguish it from the period on the numeric keypad.
            // --arcum42
            key_code = WXK_NUMPAD_SEPARATOR;
            break;

        case GDK_KEY_KP_Subtract:
            key_code = WXK_NUMPAD_SUBTRACT;
            break;

        case GDK_KEY_KP_Decimal:
            key_code = WXK_NUMPAD_DECIMAL;
            break;

        case GDK_KEY_KP_Divide:
            key_code = WXK_NUMPAD_DIVIDE;
            break;


        // function keys
        case GDK_KEY_F1:
        case GDK_KEY_F2:
        case GDK_KEY_F3:
        case GDK_KEY_F4:
        case GDK_KEY_F5:
        case GDK_KEY_F6:
        case GDK_KEY_F7:
        case GDK_KEY_F8:
        case GDK_KEY_F9:
        case GDK_KEY_F10:
        case GDK_KEY_F11:
        case GDK_KEY_F12:
            key_code = WXK_F1 + keysym - GDK_KEY_F1;
            break;

        default:
            key_code = 0;
    }

    return key_code;
}
