/* $Id: chcodes.h,v 1.1 1998/01/16 17:18:11 vassilii Exp $ */
#define CTL(c) ((c) - 'A' + 1)
#define CHAR_XOFF CTL('S')
#define CHAR_XON CTL('Q')

#define PREFIX_CONTROL '^'
#define PREFIX_ESCAPE '\\'
#define PREFIX_META '#'
