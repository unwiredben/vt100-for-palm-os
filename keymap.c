/* $Id: keymap.c,v 1.3 1998/01/16 17:18:11 vassilii Exp $ */

#include <Common.h>
#include <System/SysAll.h>
#include <UI/UIAll.h>

#include "rsrc.h"
#include "keymap.h"
#include "vt100.h"
#include "chcodes.h"

typedef struct { 
	Byte byteCharsActuallyUsed;
	Byte a_byteMapTo[CODE_CHARS_MAX];
} KeyMap;

enum { 
	eKeyPageUp, eKeyPageDn, 
	eKeyHard1, eKeyHard2, eKeyHard3, eKeyHard4,
	eKeyCradle,

	KEYMAP_MAX
};

/* The default map is selected for people using the PDA
* for a more-or-less standard navigation environment, like Lynx
* or an editor
*/
static KeyMap keymap[KEYMAP_MAX] = {
	{3, "\eOA"}, /* ANSI up */
	{3, "\eOB"}, /* ANSI down */

	{1, {CTL('C')}},
	{3, "\eOD"}, /* ANSI left */
	{3, "\eOC"}, /* ANSI right */
	{1, {'\n'}},

	{1, {CTL('L')}} /* redraw screen in most terminal apps -- kinda HotSync...*/
};

Boolean
keymap_RemapEvent (

	EventPtr event

) {
	if (
		event->eType != keyDownEvent
		|| event->data.keyDown.modifiers & poweredOnKeyMask
			/* don't remap hardware button that switched the PDA on
			 * For some strange reason poweredOnKeyMask is never set
			 * for the cradle (HotSync) button press!
			 */
	)
		return false;

	{
		KeyMap *p_keymap = keymap;
		switch (event->data.keyDown.chr) { /* see <UI/Chars.h> 
											* for poss. values */
			default: 
				return false;

			/* Hopefully the clause with '+= 0' will be optimized away, */
			/* so we leave it here for the sake of readability. */
			/* */
			/* Possible size (costing speed) optimization: */
			/* case hardCradleChr: p_keymap++; */
			/* case hard4Chr: p_keymap++; */
			/* ... */
			/* case hardDownChr: p_keymap++; */
			case pageUpChr: p_keymap += eKeyPageUp; break;
			case pageDownChr: p_keymap += eKeyPageDn; break;
			case hard1Chr: p_keymap += eKeyHard1; break;
			case hard2Chr: p_keymap += eKeyHard2; break;
			case hard3Chr: p_keymap += eKeyHard3; break;
			case hard4Chr: p_keymap += eKeyHard4; break;

			case hardCradleChr:
			case hardCradle2Chr: p_keymap += eKeyCradle;
		}

		{
			Byte i = p_keymap->byteCharsActuallyUsed;
			Byte *p = p_keymap->a_byteMapTo;

			while (i--)
				VirtualKeyPress(* p++);
		}

		return true;
	}
}
