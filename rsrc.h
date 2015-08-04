#define OkBtn 0xff // use the same OK/Cancel IDs for all forms
#define CancelBtn 0xfe

// 0x2000 and further -- used by junk id-s for defunct labels
// (XXX should probably switch to AUTOID...)
//
// Each id of a form-specific resource
//	m.s. byte = uid of the parent form resource
//	l.s. byte = uid. of the object in the resource, 0 for the resource itself
//		(child obj. id can match the parent form id, should be avoided)
// (it is OK for a child object id to be shared across several forms
// -- we use it for OkBtn and CancelBtn).
#define mainForm      0x1000  
#define mainOnline    0x1001
#define mainESC       0x1004
#define mainCTL       0x1005
#define mainMETA      0x1006
#define mainBRK    	0x1007

// In the menu each id's l.s. byte = m.s. byte of the form to invoke
#define mainMenu      		0x1100
#define main_menuButtons	0x1113
#define main_menuComm_prefs	0x1112
#define main_menuAbout		0x1114
#define main_menuGraffiti	0x1115 // !!! not a form id

// We use the l.s. byte here to reflect appropriate serial settings bits,
// see also <System/SerialMgr.h> and term.c
// m.s. byte for those special ids is 
//	1c for the data bits group
//	1b for the parity bits group
//	1a for the stop bits group
#define prefsForm 0x1200
#define prefsBaud 0x1201
#define prefsBaudList 0x1202
#define prefsBaudLabel 0x1203
#define prefsLocalEchoCheck 0x1204
#define prefsResetAutoOffCheck 0x1205
#define prefsXonXoffCheck 0x1206

#define prefsDataLabel 0x1210
// cf. serSettingsFlagBitsPerChar5, ..., serSettingsFlagBitsPerChar8
#define prefsData5	0x1c00
#define prefsData6	0x1c40
#define prefsData7	0x1c80
#define prefsData8	0x1cC0
#define prefsDataCommon 0x1c00
#define prefsDataGroup 1

#define prefsPatityLabel 0x1220 

// cf. serSettingsFlagParityOnM | serSettingsFlagParityEvenM
#define prefsParityEven	0x1b06
#define prefsParityOdd	0x1b02
#define prefsParityNone	0x1b00
#define prefsParityCommon 0x1b00
#define prefsParityGroup 2

#define prefsStopLabel 0x1240
#define prefsStopCommon 0x1a00
#define prefsStop1	0x1a00 // cf. serSettingsFlagStopBits1
#define prefsStop2	0x1a01 // cf. serSettingsFlagStopBits2
#define prefsStopGroup 3


#define CODE_CHARS_MAX 5
#define buttonsForm		0x1300

#define buttonsDatebookLabel 0x1309
#define buttonsDatebookCodeField 0x1310
#define buttonsAddressLabel 0x1311
#define buttonsAddressCodeField 0x1312
#define buttonsToDoListLabel 0x1313
#define buttonsToDoListCodeField 0x1314
#define buttonsMemoPadLabel 0x1315
#define buttonsMemoPadCodeField 0x1316
#define buttonsPageUpLabel 0x1317
#define buttonsPageUpCodeField 0x1318
#define buttonsPageDnLabel 0x1319
#define buttonsPageDnCodeField 0x1320
#define buttonsHotSyncLabel 0x1321
#define buttonsHotSyncCodeField 0x1322

#define infoForm      0x1400

#define hlpButtons 0x1e01
#define hlpPrefs 0x1e02
#define fontBitmap    0x1fff
