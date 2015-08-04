/* $Id: term.c,v 1.6 1998/02/21 20:09:26 vassilii Exp $ */
/* Copyright 1997, Brian J. Swetland <swetland@uiuc.edu>   */
/* Free for non-commercial use.  Share and Enjoy.          */

#define BENCHno
#define XON_XOFF

#if 0
#include <Common.h>
#include <System/SysAll.h>
#include <System/SerialMgr.h>
#include <System/SysEvtMgr.h>
#include <UI/UIAll.h>
#endif

#include <PalmOS.h>
#include <PalmCompatibility.h>
#include <PalmOSGlue.h>
#include <SerialMgrOld.h>

#ifdef BENCH
#include <System/TimeMgr.h>
#endif

#include "rsrc.h"
#include "vt100.h"
#include "keymap.h"
#include "chcodes.h"

/* UTIL STUFF */
static void SetCurrentMenu(Word rscID);
void CursorOff(void);
void CursorOn(void);

static MenuBarPtr CurrentMenu = NULL;

static void SetCurrentMenu(Word rscID)
{
    if (CurrentMenu)
        MenuDispose(CurrentMenu);
    
        /* Set the current menu and remember it. */
    CurrentMenu = MenuInit(rscID);
}

#define WIDTH 40
#define HEIGHT 25
#define CELLWIDTH 4
#define CELLHEIGHT 6

static int cursor_x, cursor_y;
static unsigned char screen[WIDTH*HEIGHT];
static struct virtscreen curscreen;

#define screenset(x,y,c) (screen[WIDTH*(y)+(x)] = c)

static int online = 0;  /* serial status */
static int local_echo = 0;
static int ctl = 0;
static int meta = 0;
static int tty_activity_resets_autooff = 1;

enum { 
	baud_1200, baud_2400, baud_4800, baud_9600, baud_19200,
    baud_28800, baud_38400, baud_57600, baud_115200,
    baud_230400, baud_460800,
	BAUD_MAX };
static UInt32 bauds[BAUD_MAX] = { 1200, 2400, 4800, 9600, 19200, 28800, 38400, 57600, 115200, 230400, 460800 };
static int iCurBaud = baud_2400;
static SerSettingsType serial_settings = { /* see <System/SerialMgr.h> */
	2400,

	(serSettingsFlagCTSAutoM
	| serSettingsFlagRTSAutoM
#ifdef XON_XOFF
	| serSettingsFlagXonXoffM /* XXX make it user-settable */
#endif
	| serSettingsFlagBitsPerChar8),

	5 * sysTicksPerSecond /* XXX serDefaultCTSTimeout */
};

#define serSettingsFlagParityM \
	(serSettingsFlagParityOnM | serSettingsFlagParityEvenM)
#define SELECTABLE_SERIAL_FLAGS_MASK (\
	serSettingsFlagStopBitsM \
	| serSettingsFlagParityM \
	| serSettingsFlagBitsPerCharM)

/* Check the resource IDs are defined consistently with the ser. flags */

#if !(\
   ((prefsStop1 & SELECTABLE_SERIAL_FLAGS_MASK) == serSettingsFlagStopBits1) \
&& ((prefsStop2 & SELECTABLE_SERIAL_FLAGS_MASK) == serSettingsFlagStopBits2) \
\
&& ((prefsData5 & SELECTABLE_SERIAL_FLAGS_MASK) == serSettingsFlagBitsPerChar5)\
&& ((prefsData6 & SELECTABLE_SERIAL_FLAGS_MASK) == serSettingsFlagBitsPerChar6)\
&& ((prefsData7 & SELECTABLE_SERIAL_FLAGS_MASK) == serSettingsFlagBitsPerChar7)\
&& ((prefsData8 & SELECTABLE_SERIAL_FLAGS_MASK) == serSettingsFlagBitsPerChar8)\
\
/*&& ((prefsParityEven & SELECTABLE_SERIAL_FLAGS_MASK) == \
	(serSettingsFlagParityOn | serSettingsFlagParityEvenM) )\
&& ((prefsParityOdd & SELECTABLE_SERIAL_FLAGS_MASK) == \
	(serSettingsFlagParityOn) )\
XXX for some reason, the preprocessor thinks the commented out ==s are false!*/)
#error Serial flags selector buttons have inconsistent IDs.
#endif

static WinHandle FontWH,MainWH;

static void StartApplication(void);
static Boolean MainFormHandleEvent(EventPtr event);
static void EventLoop(void);
static int cursor_active;

static void StartSerial(void);
static void SetControlValueByRscId(FormPtr frm, Word objID, short Value);
void std_interpret_char(struct virtscreen *, int);

void RefreshRegion(int rx, int ry, int w, int h)
{
    int x,y;
    unsigned char c;
    RectangleType rect;
    rect.extent.x = 4;
    rect.extent.y = 6;

    for(y=ry;y<ry+h;y++){
        for(x=rx;x<rx+w;x++){
            c = screen[x+y*WIDTH] & 0x7F;
/*          if(c>127) c = 0;            */
            rect.topLeft.x = (c%32)*CELLWIDTH;
            rect.topLeft.y = (c/32)*CELLHEIGHT;
/*            WinFillRectangle(&rect,0);*/
            
            WinCopyRectangle(FontWH, MainWH, &rect,
                             x*CELLWIDTH,y*CELLHEIGHT,
                             scrCopy);
        }
    }
}
void RefreshChar(int rx, int ry)
{
    RectangleType rect;
    unsigned char c;
    rect.extent.x = 4;
    rect.extent.y = 6;

    c = screen[rx+ry*WIDTH] & 0x7F;
    rect.topLeft.x = (c%32)*CELLWIDTH;
    rect.topLeft.y = (c/32)*CELLHEIGHT;
    WinCopyRectangle(FontWH, MainWH, &rect,
                     rx*CELLWIDTH,ry*CELLHEIGHT,
                     scrCopy);
}

void EraseRegion(int rx, int ry, int w, int h)
{
    RectangleType rect;
    rect.extent.x = w*CELLWIDTH;
    rect.extent.y = h*CELLHEIGHT;
    rect.topLeft.x = rx*CELLWIDTH;
    rect.topLeft.y = ry*CELLHEIGHT;
    WinEraseRectangle(&rect, 0);
}

void ScrollRegion(int top, int bottom)
{
    RectangleType rect,rect2;

    rect.extent.x = WIDTH*CELLWIDTH;
    rect.extent.y = (bottom-top +1 )*CELLHEIGHT;
    rect.topLeft.x = 0;    
    rect.topLeft.y = (top /* +1 */)*CELLHEIGHT;

    CursorOff();
    WinScrollRectangle(&rect, winUp, CELLHEIGHT, &rect2);
    CursorOn();    
}

void CursorOff(void)
{
    if(cursor_active) {
        WinEraseLine(cursor_x*CELLWIDTH,cursor_y*CELLHEIGHT+5,
                     cursor_x*CELLWIDTH+4,cursor_y*CELLHEIGHT+5);
        cursor_active = 0;
    }
}
void CursorOn(void)
{
    if(!cursor_active) {
        WinDrawLine(cursor_x*CELLWIDTH,cursor_y*CELLHEIGHT+5,
                    cursor_x*CELLWIDTH+4,cursor_y*CELLHEIGHT+5);
        cursor_active = 1;
    }
}

void CursorTo(int x, int y)
{
    if(x==cursor_x && y==cursor_y) return;
    
    if(cursor_active)
        WinEraseLine(cursor_x*CELLWIDTH,cursor_y*CELLHEIGHT+5,
                     cursor_x*CELLWIDTH+4,cursor_y*CELLHEIGHT+5);
    cursor_x = x;
    cursor_y = y;
    if(cursor_active)
        WinDrawLine(cursor_x*CELLWIDTH,cursor_y*CELLHEIGHT+5,
                    cursor_x*CELLWIDTH+4,cursor_y*CELLHEIGHT+5);    
}

#define keydown(key) char_to_virtscreen(&curscreen,((unsigned char)(key)));

static UInt SerialRefNum;

#ifdef BENCH
char test0[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

char test1[] = "RandomText"
"RandomText"
"RandomText"
"RandomText";

void Result(char *r, int l, ULong time)
{
    char buf[80];    
    int i;

    sprintf(buf,"%s: %ld",r,time);
    
    for(i=0;buf[i];i++) screenset(i,l,buf[i]);
    RefreshRegion(0,l,40,1);
    
}

void TestRegion(char *test)
{
    int i;
    for(i=0;i<40;i++) screenset(i,0,test[i]);
}

#define B(txt,v) for(i=0;i<40;i++) screenset(i,0,(v));\
    t = TimGetTicks();\
    for(i=0;i<100;i++) RefreshRegion(0,0,40,1);\
    t0 = TimGetTicks() - t;\
    Result(txt,line++,t0);
    
void memmove2(unsigned char *dst, unsigned char *src, int count);

void BenchMark(void)
{
    int i,line =3;
    
    ULong t0;
    ULong t;
/*
    t = TimGetTicks();
    for(i=0;i<10000;i++) memmove2(screen,screen+40,40*23);
    t0 = TimGetTicks() - t;
    Result("mm2",2,t0);
    
    t = TimGetTicks();
    for(i=0;i<10000;i++) MemMove(screen,screen+40,40*23);
    t0 = TimGetTicks() - t;
    Result("MM",3,t0);
    */
    t = TimGetTicks();
    for(i=0;i<10000;i++) WinDrawChars(test1,1,0,20);
    t0 = TimGetTicks() - t;
    Result("wdc",2,t0);
    
    
/*
    t = TimGetTicks();
    for(i=0;i<100;i++) ScrollRegion(0,22);
    t0 = TimGetTicks() - t;
    Result("scroll   ",2,t0);
    
    B("fill    0",0);
    B("fill    1",1);
    B("fill    2",2);
    B("fill   32",32);
    B("fill    i",i);
    B("fill i+32",i+32);*/
        
}
#endif

/***********************************************************************
 * FUNCTION:     StartApplication
 *
 * DESCRIPTION:  This routine sets up the initial state of the application.
 ***********************************************************************/
static void StartApplication(void)
{
    FormPtr frm;
    VoidHand FontBH;
    BitmapPtr FontBP;
#ifndef ALLOW_ACCESS_TO_INTERNALS_OF_BITMAPS
    UInt16 fontwidth, fontheight, fontrowbytes;
#endif /* no struct-banging. */
    Word error;
    UInt16 prefsize;
    Int16 prefver;
    UInt32 prefval;

    cursor_active = 1;    
    
    MainWH = WinGetDrawWindow();
        /* Get the resource and data */
    FontBH = DmGetResource(bitmapRsc, fontBitmap);
    FontBP = MemHandleLock(FontBH);
    
#ifdef ALLOW_ACCESS_TO_INTERNALS_OF_BITMAPS
    FontWH = WinCreateOffscreenWindow(FontBP->width, FontBP->height,
                                      screenFormat, &error);
#else /* no struct-banging. */
    BmpGlueGetDimensions(FontBP, &fontwidth, &fontheight, &fontrowbytes);
    FontWH = WinCreateOffscreenWindow(fontwidth, fontheight,
                                      screenFormat, &error);
#endif /* no struct-banging. */
    ErrFatalDisplayIf(error, "Error loading glyphs");
    WinSetDrawWindow(FontWH);
    WinDrawBitmap(FontBP, 0, 0);
  
        /* We've copied it local, let it go. */
    MemPtrUnlock(FontBP);
    DmReleaseResource(FontBH);
    
    WinSetDrawWindow(MainWH);
    
    frm = FrmInitForm(mainForm); 
    FrmSetActiveForm(frm);
    FrmDrawForm(frm);

    MainWH = WinGetActiveWindow();

    init_virtscreen(&curscreen,screen,HEIGHT,WIDTH);
    
    SetCurrentMenu(mainMenu);

/* Set program state from stored prefs. */
    /* Baud rate */
    prefsize = sizeof(prefval);
    prefver = PrefGetAppPreferences(CREATORID, 1, &prefval, &prefsize, true);
    if (prefver != noPreferenceFound) {
      iCurBaud = prefval;
    }
    serial_settings.baudRate = bauds[iCurBaud];
    /* serial flags. */
    prefsize = sizeof(prefval);
    prefver = PrefGetAppPreferences(CREATORID, 2, &prefval, &prefsize, true);
    if (prefver == VERSION) {
      serial_settings.flags = prefval;
    }
    /* Local echo */
    prefsize = sizeof(prefval);
    prefver = PrefGetAppPreferences(CREATORID, 3, &prefval, &prefsize, true);
    if (prefver == VERSION) {
      local_echo = prefval;
    }
    /* Auto-off */
    prefsize = sizeof(prefval);
    prefver = PrefGetAppPreferences(CREATORID, 4, &prefval, &prefsize, true);
    if (prefver == VERSION) {
      tty_activity_resets_autooff = prefval;
    }
    /* Online */
    prefsize = sizeof(prefval);
    prefver = PrefGetAppPreferences(CREATORID, 5, &prefval, &prefsize, true);
    if (prefver == VERSION) {
      if (prefval) {
        StartSerial();
      }
      online = prefval;
    }
    SetControlValueByRscId(frm, mainOnline, online);
    /* Screen buffer.  Crazy shit. */
    if (online) {
      prefsize = sizeof(curscreen);
      prefver = PrefGetAppPreferences(CREATORID, 6, &curscreen, &prefsize, false);
       /* Don't want this saved across hard-reset restores. */
      if (prefver == VERSION) {
        prefsize = curscreen.rows * curscreen.columns;
        prefver = PrefGetAppPreferences(CREATORID, 7, curscreen.data, &prefsize, false);
        if (prefver == VERSION) {
          curscreen.data_off_scr = curscreen.data;
          curscreen.next_char_send = std_interpret_char;
          CursorOff();
          cursor_x = curscreen.xpos;
          cursor_y = curscreen.ypos;
          RefreshRegion(0, 0, curscreen.columns, curscreen.rows);
          CursorOn();
        } else {
          init_virtscreen(&curscreen,screen,HEIGHT,WIDTH);
        }
      } else {
        init_virtscreen(&curscreen,screen,HEIGHT,WIDTH);
      }
    }
    /* Keymap preferences. */
    keymap_getprefs(CREATORID, 8);


#ifdef BENCH
    BenchMark();    
#endif    
#ifdef HW_CURSOR
    InsPtSetHeight(5);
    InsPtSetLocation(0,0);
    InsPtEnable(1);
#endif
}

#define SBUFSIZE (1024 * 8)
#define SBUFSIZE_SYS SBUFSIZE+32 /* see the SerSetReceiveBuffer() docs */
#define RECV_LOW_WATER (SBUFSIZE * 1 / 32)
#define RECV_HIGH_WATER (SBUFSIZE * 1 / 8)
unsigned char serbuf[SBUFSIZE];
unsigned char serbuf_system[SBUFSIZE_SYS];
static void SendSerial(int c)
{
    char buf;
    buf = c;
    SerSend10(SerialRefNum, &buf, 1);    
}

static void RecvSerial(void)
{
#	ifdef XON_XOFF
	static int bXoffSent = 0;
	static ULong ulBytesWhenXoffSent = 0L; /* XXX 
			* needed for debugging only;
			* used to find good buffer size and high/low water marks
			*/
#	endif

    ULong bytes;
    char *c;

    for(;;) {
        if(SerReceiveCheck(SerialRefNum, &bytes)){            
            if(serErrLineErr){
                SerClearErr(SerialRefNum);
            }
            continue;            
        }

#		ifdef XON_XOFF
		if (bXoffSent && bytes < RECV_LOW_WATER) {
			bXoffSent = 0;
			SendSerial(CHAR_XON);
		}
#		endif
        if (!bytes) return;

		/* Something was received! */
		if (tty_activity_resets_autooff)
			EvtResetAutoOffTimer();

		if (bytes > SBUFSIZE){
			char s[128];
			StrPrintF(
				s, 
				"Buffer overrun: %ld/%d"
#ifdef XON_XOFF
				"; low:high = %d:%d"
				"; xoff sent (%d) at %ld"
#endif
				, bytes, SBUFSIZE
#ifdef XON_XOFF
				, RECV_LOW_WATER, RECV_HIGH_WATER
				, bXoffSent, ulBytesWhenXoffSent
#endif
			);
			ErrDisplay(s);
		}

#		ifdef XON_XOFF
		if (!bXoffSent && bytes > RECV_HIGH_WATER) {
			SendSerial(CHAR_XOFF);
			bXoffSent = 1;
			ulBytesWhenXoffSent = bytes;
		}
#		endif

        if (SerReceive10(SerialRefNum, serbuf, bytes, -1)==serErrLineErr) {
            SerClearErr(SerialRefNum);
            break;                
        }
        CursorOff();    
        for(c=serbuf;bytes;bytes--,c++) if(*c) keydown(*c);
        CursorOn();        
    }    
}

static void StartSerial(void)
{
    if(online) return;
    
    if(SysLibFind("Serial Library",&SerialRefNum)){
        ErrDisplay("Cannot Find Serial Lib");        
    } else {
        if(SerOpen(SerialRefNum, 0, bauds[iCurBaud])){
            ErrDisplay("Cannot Open Serial Lib");
        }
		else { /* See also <System/SerialMgr.h> */ /* <Core/System/SerialMgrOld.h> */
            /* Maximal theoretical speed of 1Mbit (36*12*2400?) */
            /* 19200 bps is fastest safe speed without CTS handshaking. */
#if 0 /* WTF is CTS handshaking? */
            if (bauds[iCurBaud] > 19200) {
              serial_settings.flags |= (serSettingsFlagCTSAutoM | serSettingsFlagRTSAutoM);
              serial_settings.ctsTimeout = serDefaultCTSTimeout;
            } else {
              serial_settings.flags &= ~serSettingsFlagCTSAutoM;
            }
#endif
			SerSetSettings(SerialRefNum, &serial_settings);                
			ErrFatalDisplayIf(
				SerSetReceiveBuffer(
					SerialRefNum, (void *)serbuf_system, SBUFSIZE_SYS),
				"Can't set own receive buffer!");

			online = 1;        
		}
    }    
    
}

static void StopSerial(void)
{
    if(online) {
		/* XXX according to the manual on SerSetReceiveBuffer(), */
		/* it must be restored BEFORE the port is closed, and not AFTER that. */
		/* However, interchanging the calls to SerClose() and */
		/* SerSetReceiveBuffer() causes "invalid chunk pointer" fatal */
		/* raised by the memory manager when this point is reached. */
		SerClose(SerialRefNum);
		SerSetReceiveBuffer(SerialRefNum, NULL, 0);
			/* restore the default system queue */
		online = 0;    
	}
}

static void StopApplication(void)
{ 
    UInt16 prefsize;
    Int16 prefver;
    UInt32 prefval;
#ifdef HW_CURSOR
    InsPtEnable(0);    
#endif
/* Store settings. */
    prefver = VERSION;
    prefsize = sizeof(prefval);

    /* Baud rate. */
    prefval = iCurBaud;
    PrefSetAppPreferences(CREATORID, 1, prefver, &prefval, prefsize, true);
    /* Serial flags. */
    prefval = serial_settings.flags;
    PrefSetAppPreferences(CREATORID, 2, prefver, &prefval, prefsize, true);
    /* Local echo */
    prefval = local_echo;
    PrefSetAppPreferences(CREATORID, 3, prefver, &prefval, prefsize, true);
    /* Auto-off */
    prefval = tty_activity_resets_autooff;
    PrefSetAppPreferences(CREATORID, 4, prefver, &prefval, prefsize, true);
    /* Online */
    prefval = online;
    PrefSetAppPreferences(CREATORID, 5, prefver, &prefval, prefsize, true);

    if (online) {
      StopSerial();
/* Save buffer. */
      prefsize = sizeof(curscreen);
      PrefSetAppPreferences(CREATORID, 6, prefver, &curscreen, prefsize, false);
      prefsize = curscreen.num_bytes;
      PrefSetAppPreferences(CREATORID, 7, prefver, curscreen.data, prefsize, false);
    }
    /* Keymap preferences. */
    keymap_setprefs(CREATORID, 8);

        /* Release Glyph Bitmaps */
    WinDeleteWindow(FontWH, false);
} 

static void SetControlValueByRscId(FormPtr frm, Word objID, short Value)
{
	FrmSetControlValue(frm,
		FrmGetObjectIndex(frm, objID),
		Value);
}

#define ModifierToggle_Ctl()\
	(SetControlValueByRscId(frmActive, mainCTL,  ctl ^= 1));
#define ModifierToggle_Meta()\
	(SetControlValueByRscId(frmActive, mainMETA,  meta ^= 1));

static Boolean MainFormHandleEvent(EventPtr event)
{
	FormPtr frmActive = FrmGetActiveForm();
    switch(event->eType){
    case nilEvent :
        if(online) RecvSerial();
        return(true);
       
    case ctlSelectEvent :
		switch (event->data.ctlSelect.controlID) {
			case mainCTL:
				ModifierToggle_Ctl();
				return(true);            

			case mainMETA:
				ModifierToggle_Meta();
				return(true);            

			case mainESC:
				VirtualKeyPress(27);
				return(true);            

			case mainOnline:
				if(online){
					StopSerial();                
				} else {
					StartSerial();
				}
				SetControlValueByRscId(frmActive, mainOnline, online);
				return(true);

			case mainBRK:
				if (online) {
					SetControlValueByRscId(frmActive, mainBRK, 1);
					SerControl(SerialRefNum, serCtlStartBreak, 0, 0);
					SysTaskDelay(sysTicksPerSecond * 3 / 10); /* 300 ms */
					SerControl(SerialRefNum, serCtlStopBreak, 0, 0);
					SetControlValueByRscId(frmActive, mainBRK, 0);
				}
				return(true);
        }
        break;
        
    case menuEvent :
            /* First clear the menu status from the display. */
        MenuEraseStatus(CurrentMenu);        
		{
			Word idItem = event->data.menu.itemID;
			if (idItem == main_menuGraffiti) {
				SysGraffitiReferenceDialog(referenceDefault);
			}
			else
			{
				Word idForm = (idItem & 0xff) << 8;
					/* see the rsrc.h for the IDs conventions required here */
				FormPtr frm = FrmInitForm(idForm);
				ListPtr pBaudList = 0; /* Make gcc happy. */
					/* init. in first switch, reused in the second */

				/* We could have vectorized the form-specific init
				* and postprocessing, however this doesn't 
				* make sense for < 3 forms.
				* Still this could be done any moment (with clever resource IDs)
				*/

				/* Set the form controls state according to the program state */
				switch (idForm) {
					case prefsForm:
						SetControlValueByRscId(
							frm, prefsLocalEchoCheck,
							local_echo);

						SetControlValueByRscId(frm,
							prefsResetAutoOffCheck,
							tty_activity_resets_autooff);

						SetControlValueByRscId(frm,
							prefsXonXoffCheck,
							serial_settings.flags & serSettingsFlagXonXoffM);

						SetControlValueByRscId(frm,
								prefsDataCommon | (serial_settings.flags 
									& serSettingsFlagBitsPerCharM),
							1);
						SetControlValueByRscId(frm,
								prefsStopCommon | (serial_settings.flags 
									& serSettingsFlagStopBitsM),
							1);
						SetControlValueByRscId(frm,
								prefsParityCommon | (serial_settings.flags 
									& serSettingsFlagParityM),
							1);
						SetControlValueByRscId(frm,
								prefsParityCommon | (serial_settings.flags 
									& serSettingsFlagParityM),
							1);
						SetControlValueByRscId(frm,
								prefsParityCommon | (serial_settings.flags 
									& serSettingsFlagParityM),
							1);

						pBaudList = FrmGetObjectPtr(frm,
							FrmGetObjectIndex(frm, prefsBaudList));
						LstSetSelection(pBaudList, iCurBaud);
						{
							ControlType *p = FrmGetObjectPtr(frm,
								FrmGetObjectIndex(frm, prefsBaud));
							CtlSetLabel(p, 
								LstGetSelectionText(pBaudList, iCurBaud));
						}
						break;

					case buttonsForm:
						keymap_setform(frm);
						break;
					default:
				}

				if (FrmDoDialog(frm) == OkBtn) {
					/* We need to react to what was done by the user 
					 * to that form
					 * only if he pressed OK (not cancel). Forms that do not
					 * require our attn (currently the about form)
					 * do not have OkBtn
					 * (they may have a button called "OK",
					 * yet it is a diff. id.) */
					switch (idForm) {
						case prefsForm:
							local_echo = FrmGetControlValue(frm,
								FrmGetObjectIndex(frm, prefsLocalEchoCheck));
							tty_activity_resets_autooff = FrmGetControlValue(frm,
								FrmGetObjectIndex(frm, prefsResetAutoOffCheck));
							iCurBaud = LstGetSelection(pBaudList);
							serial_settings.baudRate = bauds[iCurBaud];
							serial_settings.flags &= ~SELECTABLE_SERIAL_FLAGS_MASK;
							if (FrmGetControlValue(frm,FrmGetObjectIndex(frm, prefsXonXoffCheck))) {
								serial_settings.flags |= serSettingsFlagXonXoffM;
							} else {
								serial_settings.flags &= ~serSettingsFlagXonXoffM;
							}
							serial_settings.flags |= (
									FrmGetObjectId(frm,
										FrmGetControlGroupSelection(frm, 
											prefsDataGroup))
									| FrmGetObjectId(frm,
										FrmGetControlGroupSelection(frm, 
											prefsParityGroup))
									| FrmGetObjectId(frm,
										FrmGetControlGroupSelection(frm, 
											prefsStopGroup))
								) & SELECTABLE_SERIAL_FLAGS_MASK;
								
							if(online){
								StopSerial();
								StartSerial();
							}
							break;

						case buttonsForm:
							keymap_getform(frm);
							break;

						default:
					}
				};
				FrmDeleteForm(frm);
			}
		}
        return(true);
        
    case frmUpdateEvent :
        return(true);
#ifdef SELECTION        
    case penMoveEvent :
        return(false);
        
    case penDownEvent :
        return(false);
        
    case penUpEvent :
        return(false); 
#endif

    case keyDownEvent :
		{
			unsigned i;
			if((i = event->data.keyDown.chr) != 0){
				if (ctl) {
					i &= 0x1f;
					ModifierToggle_Ctl();
				}
				if (meta) {
					i |= 0x80;
					ModifierToggle_Meta();
				}
				if(i==10) i=13;            
				VirtualKeyPress((Byte)i);
				return(true);            
			}
		}
	default:
    }
    
    return(false);
}


static void EventLoop(void) 
{ 
    EventType event;
    Word   error;

    do{
        EvtGetEvent(&event, online ? 0 : evtWaitForever);
		if (! keymap_RemapEvent(&event) )
			if (! SysHandleEvent (&event))
				if (! MenuHandleEvent(CurrentMenu, &event, &error))
					if (! MainFormHandleEvent(&event))
						FrmHandleEvent(FrmGetActiveForm(), &event);
    } while (event.eType != appStopEvent);

//    if(online) StopSerial();
}


DWord PilotMain(Word cmd, Ptr cmdPBP, Word launchFlags)
{
    if (cmd == sysAppLaunchCmdNormalLaunch) {  
        StartApplication();
        EventLoop();  
        StopApplication();
    }
    return(0);
}

void VirtualKeyPress(Byte ascii)
{
	if (local_echo) keydown(ascii);
	if(online) SendSerial(ascii);
}
