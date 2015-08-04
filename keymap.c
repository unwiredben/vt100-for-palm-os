/* $Id: keymap.c,v 1.3 1998/01/16 17:18:11 vassilii Exp $ */

#if 0
#include <Common.h>
#include <System/SysAll.h>
#include <UI/UIAll.h>
#endif /* 0 */
#include <PalmOS.h>
#include <PalmCompatibility.h>

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


int
keymap_defaults (int mapentry)
{
  int i;

#define SETKEY(n, sz, str) \
  if ((mapentry < 0) || (mapentry == n)) { \
    keymap[n].byteCharsActuallyUsed = sz; \
    for (i = 0; i < sz; i++) { keymap[n].a_byteMapTo[i] = str[i]; } \
  }

  SETKEY(eKeyPageUp, 3, "\eOA");
  SETKEY(eKeyPageDn, 3, "\eOB");
  SETKEY(eKeyHard1, 1, "\003");
  SETKEY(eKeyHard2, 3, "\eOD");
  SETKEY(eKeyHard3, 3, "\eOC");
  SETKEY(eKeyHard4, 1, "\n");
  SETKEY(eKeyCradle, 1, "\014");
  return 0;
}


Boolean
keymap_getprefs (UInt32 appid, UInt16 prefid)
{
  UInt16 prefsize;
  Int16 prefver;

  prefsize = sizeof(keymap);
  prefver = PrefGetAppPreferences(appid, prefid, &keymap, &prefsize, true);
  if ((prefver == noPreferenceFound) || (prefsize != sizeof(keymap))) {
    keymap_defaults(-1);
    return false;
  }
  return true;
}


Boolean
keymap_setprefs (UInt32 appid, UInt16 prefid)
{
  UInt16 prefsize;
  Int16 prefver;

  prefsize = sizeof(keymap);
  prefver = VERSION;
  PrefSetAppPreferences(appid, prefid, prefver, &keymap, prefsize, true);
  return true;
}



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



int
keymap_snprint (unsigned char *buf, int maxlen, int len, unsigned char *data)
{
  int buflen;
  int i;
  int ch;

  *buf = 0;
  buflen = 0;

  for (i = 0; (i < len) && (buflen < maxlen - 4); i++) {
/* Worse-case required space is 4 chars, for \NNN form. */
    ch = data[i];
#if 0
printf("CH[%d] = %d\n", len, ch);
#endif /* 0 */
    if (ch < 32) {
      switch (ch) {
        /* check for \x forms before using ^x form. */
        case '\e':
          buf[buflen++] = PREFIX_ESCAPE;
          buf[buflen++] = 'e';
          break;
        case '\r':
          buf[buflen++] = PREFIX_ESCAPE;
          buf[buflen++] = 'r';
          break;
        case '\n':
          buf[buflen++] = PREFIX_ESCAPE;
          buf[buflen++] = 'n';
          break;
        case '\t':
          buf[buflen++] = PREFIX_ESCAPE;
          buf[buflen++] = 't';
          break;
        default:
          buf[buflen++] = PREFIX_CONTROL;
          buf[buflen++] = (ch + 'A' - 1);
          break;
      }
    } else {
      switch (ch) {
        /* Check for special \x forms. */
        case PREFIX_ESCAPE:
          buf[buflen++] = PREFIX_ESCAPE;
          buf[buflen++] = PREFIX_ESCAPE;
          break;
        case PREFIX_CONTROL:
          buf[buflen++] = PREFIX_ESCAPE;
          buf[buflen++] = PREFIX_CONTROL;
          break;
#if 0
        case PREFIX_META:
          buf[buflen++] = PREFIX_ESCAPE;
          buf[buflen++] = PREFIX_META;
#endif
        default:
          buf[buflen++] = ch;
          break;
      } //switch
    } //if ch range
  } //for i in len

  buf[buflen] = 0;


#if 0
  for (i = 0; i < buflen; i++) {
    printf("CHAR %d = %d\n", i, buf[i]);
  }
  printf("BUFFER = %s\n", buf);
#endif /* 0 */


  return buflen;
}



int
keymap_setform (FormPtr frm)
{
	FieldPtr fldp;
	MemHandle mh;
	MemPtr mp;
    int mlen;

/* Given a shorthand field name, assign the field value stored in keymap at entry number `mapentry'. */
#define SETFIELD(btn, mapentry) \
  fldp = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, buttons##btn##CodeField));\
  mlen = CODE_CHARS_MAX * 4; \
  mh = MemHandleNew(mlen); \
  mp = MemHandleLock(mh); \
  keymap_snprint(mp, mlen, keymap[mapentry].byteCharsActuallyUsed, keymap[mapentry].a_byteMapTo); \
  MemPtrUnlock(mp); \
  FldSetText(fldp, mh, 0, 32);

	SETFIELD(Datebook, eKeyHard1);
	SETFIELD(Address, eKeyHard2);
	SETFIELD(ToDoList, eKeyHard3);
	SETFIELD(MemoPad, eKeyHard4);
	SETFIELD(PageUp, eKeyPageUp);
	SETFIELD(PageDn, eKeyPageDn);
	SETFIELD(HotSync, eKeyCradle);
	return 0;
}











struct parser_s {
  char *buf;
  int len;
  int pos;

  char token[16];
  int tokenlen;
  int tokentype;
};

typedef struct parser_s parser_t;



int
keymap_parse_control (parser_t *parser)
{
  int ch;
  int ate;

  ate = 0;
  ch = parser->buf[parser->pos];
  parser->tokenlen = 1;
  ate = 1;
  if (parser->pos >= parser->len) {
    /* Nothing left to read. */
    parser->token[0] = PREFIX_CONTROL;
    return 0;
  }
  if (('A' <= ch) && (ch <= 'Z')) {
    ch = CTL(ch);
    parser->token[0] = ch;
  } else if (('a' <= ch) && (ch <= 'z')) {
    ch = CTL(ch - 32);
    parser->token[0] = ch;
  } else if (('[' <= ch) && (ch <= '_')) {
    ch = CTL(ch);
    parser->token[0] = ch;
  } else if ((ch == ' ') || (ch == '@')) {
    ch = 0;
    parser->token[0] = ch;
  } else {
    parser->token[0] = PREFIX_CONTROL;
    parser->token[1] = ch;
    parser->tokenlen = 2;
  }

#if 0
  printf("CONTROL CHARACTER: %d\n", parser->token[0]);
#endif /* 0 */
  return ate;
}

int
keymap_parse_escape (parser_t *parser)
{
  int ch;
  int ate;

  ate = 0;
  parser->tokenlen = 1;
  if (parser->pos >= parser->len) {
    /* Nothing left to read. */
    parser->token[0] = PREFIX_ESCAPE;
    return 0;
  }
  ch = parser->buf[parser->pos];
  ate = 1;
  switch (ch) {
    case 'a': /* alert (^G). */
      parser->token[0] = '\a';
      break;
    case 'b': /* backspace (^H). */
      parser->token[0] = '\b';
      break;
    case 'e': /* escape (^[). */
      parser->token[0] = '\e';
      break;
    case 'f': /* formfeed (^L). */
      parser->token[0] = '\f';
      break;
    case 'n': /* newline (^J). */
      parser->token[0] = '\n';
      break;
    case 'r': /* carriage return (^M). */
      parser->token[0] = '\r';
      break;
    case 't': /* tab (^I). */
      parser->token[0] = '\t';
      break;
    case 'v': /* vertical tab (^K). */
      parser->token[0] = '\v';
      break;
    case PREFIX_ESCAPE: /* backslash (\). */
      parser->token[0] = PREFIX_ESCAPE;
      break;
    case PREFIX_CONTROL: /* caret (^). */
      parser->token[0] = PREFIX_CONTROL;
      break;
    case PREFIX_META: /* hash (#). */
      parser->token[0] = PREFIX_META;
      break;
    default:
      if (('0' <= ch) && (ch <= '9')) {
        /* Parse as octal. */
        /* Terminates on: third character interpreted; end of string; non-digit */
        ate = 0;
        parser->token[0] = 0;
        while (ate < 3) {
          parser->token[0] *= 8;
          parser->token[0] += (ch - '0');
          ate++;
          ch = parser->buf[parser->pos + ate];
          if ((ch < '0') || ('9' < ch))
            break;
          if (parser->pos + ate >= parser->len)
            break;
        }
      } else {
        parser->token[0] = PREFIX_ESCAPE;
        parser->token[1] = ch;
        parser->tokenlen = 2;
        ate = 1;
      }
      break;
  }
#if 0
  printf("ESCAPED CHARACTER: %d\n", parser->token[0]);
#endif /* 0 */
  return ate;
}

int
keymap_parse (unsigned char *buf, int maxlen, parser_t *parser)
{
  unsigned ch;
  int buflen;
  int i;

  *buf = 0;
  buflen = 0;
  while (parser->pos < parser->len) {
    ch = parser->buf[parser->pos++];
    switch (ch) {
      case PREFIX_CONTROL:
        parser->pos += keymap_parse_control(parser);
        break;
      case PREFIX_ESCAPE:
        parser->pos += keymap_parse_escape(parser);
        break;
      default:
        parser->token[0] = ch;
        parser->tokenlen = 1;
#if 0
        printf("ASCII CHARACTER: %c\n", ch);
#endif /* 0 */
        break;
    }

    for (i = 0; (i < parser->tokenlen) && (buflen < maxlen); i++) {
#if 0
printf("parsed append %d\n", parsedlen);
#endif /* 0 */
      buf[buflen++] = parser->token[i];
    }
  } //while
#if 0
  printf("parse done\n");
  for (i = 0; i < parsedlen; i++) {
    printf("CHAR %d = %d\n", i, parsed[i]);
  }
#endif /* 0 */
  return buflen;
}




/* Sets keymap according to field entries of buttonForm. */
int
keymap_getform (FormPtr frm)
{
	FieldPtr fldp;
    int mlen;
	parser_t parser_opaque;

#define GETFIELD(btn, mapentry) \
  fldp = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, buttons##btn##CodeField));\
  mlen = CODE_CHARS_MAX * 4; \
  parser_opaque.buf = FldGetTextPtr(fldp); \
  parser_opaque.len = FldGetTextLength(fldp); \
  parser_opaque.pos = 0; \
  if (parser_opaque.len < 1) { \
    keymap_defaults(mapentry); \
  } else { \
    keymap[mapentry].byteCharsActuallyUsed = keymap_parse(keymap[mapentry].a_byteMapTo, CODE_CHARS_MAX, &parser_opaque); \
  }

	GETFIELD(Datebook, eKeyHard1);
	GETFIELD(Address, eKeyHard2);
	GETFIELD(ToDoList, eKeyHard3);
	GETFIELD(MemoPad, eKeyHard4);
	GETFIELD(PageUp, eKeyPageUp);
	GETFIELD(PageDn, eKeyPageDn);
	GETFIELD(HotSync, eKeyCradle);
	return 0;
}
