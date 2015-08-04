#ifndef _VT_100_H
#define _VT_100_H

#define loc_in_virtscreen(cur,y,x) (((cur)->data)+(((y)*((cur)->columns))+(x)))
#define MAX_ANSI_ELEMENTS 16
#define char_to_virtscreen(cur,ch) (((cur)->next_char_send)((cur),(ch)))

struct virtscreen
{
  int rows;
  int columns; 
  int num_bytes;
  unsigned char *data;
  unsigned char *data_off_scr;

  int xpos;
  int ypos;
  int top_scroll;
  int bottom_scroll;
  int attrib;
  int old_attrib;
  int old_xpos;
  int old_ypos;

  int cur_ansi_number;
  int ansi_elements;
  int ansi_reading_number;
  int ansi_element[MAX_ANSI_ELEMENTS];

  void (*next_char_send)(struct virtscreen *cur, int ch);
};


int init_virtscreen(struct virtscreen *cur, unsigned char *data,
                    int rows, int columns);

void CursorTo(int x, int y);
void EraseRegion(int rx, int ry, int w, int h);
void ScrollRegion(int top, int bottom);
void RefreshRegion(int rx, int ry, int w, int h);
void RefreshChar(int rx, int ry);

void VirtualKeyPress(Byte ascii);
#endif /* _VT_100_h */
