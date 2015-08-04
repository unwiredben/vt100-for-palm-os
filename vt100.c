/* vt100.c */
/* vt100 engine copyright 1997, Daniel Marks */

#include <Common.h>
#include <System/SysAll.h>

/*
  #include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
*/

#include "vt100.h"

#define STRICT_VT100

/* struct virtscreen newscr; */

void std_interpret_char(struct virtscreen *cur, int ch);
void std_interpret_ansi(struct virtscreen *cur, int ch);
void special_reading_ansi(struct virtscreen *cur, int ch);
void special_ansi_charset_0(struct virtscreen *cur, int ch);
void special_ansi_charset_1(struct virtscreen *cur, int ch);

void clear_region(struct virtscreen *cur, int y1, int x1, int y2, int x2, int atrb);


#define move_cursor(cur) CursorTo(cur->xpos,cur->ypos)

int init_virtscreen(struct virtscreen *cur, unsigned char *data,
                    int rows, int columns)
{

  cur->num_bytes = (rows * columns);

  cur->data = data;
  cur->data_off_scr = cur->data;

  cur->bottom_scroll = cur->rows = rows;
  cur->columns = columns;
  cur->cur_ansi_number = cur->top_scroll = 0;
  cur->ansi_elements = cur->ansi_reading_number = 0;
  cur->next_char_send = std_interpret_char;
  cur->old_attrib = cur->attrib = 0x07;
  cur->next_char_send = std_interpret_char;
  clear_region(cur,0,0,rows-1,columns-1,cur->attrib);
  cur->old_xpos = cur->old_ypos = 0;

  return (0);
}

void clear_region(struct virtscreen *cur, int y1, int x1, int y2, int x2, int atrb)
{
  unsigned char *ch = loc_in_virtscreen(cur,y1,x1);
  unsigned char *tr = loc_in_virtscreen(cur,y2,x2);
  int dt = 0x20 /* | (atrb << 8) */;

  while (ch<=tr)
    *ch++ = dt;

  EraseRegion(x1,y1,x2-x1+1,y2-y1+1);  
      /*XXX*/
}

void clear_virtscreen(struct virtscreen *cur, int mode)
{
  switch (mode)
  {
    case 0: clear_region(cur,cur->ypos,cur->xpos,
			 cur->rows-1,cur->columns-1,cur->attrib);
            break;
    case 1: clear_region(cur,0,0,cur->ypos,cur->xpos,cur->attrib);
            break;
    default: clear_region(cur,0,0,cur->rows-1,cur->columns-1,cur->attrib);
             cur->xpos = cur->ypos = 0;
             break;
  }
}

void clear_to_eol(struct virtscreen *cur, int mode)
{
  switch(mode)
    {
      case 0: 
          clear_region(cur,cur->ypos,cur->xpos,cur->ypos,
                       cur->columns-1,cur->attrib);
          break;
      case 1:
	  clear_region(cur,cur->ypos,0,cur->ypos,cur->xpos,
                       cur->attrib);
          break;
      default:
          clear_region(cur,cur->ypos,0,cur->ypos,cur->columns-1,
                       cur->attrib);
	}
}


void position_console(struct virtscreen *cur, int ypos, int xpos, int rel)
{
  if (rel)
    {
      cur->xpos += xpos;
      cur->ypos += ypos;
  } else
    {
      cur->xpos = xpos;
      cur->ypos = ypos;
    }
  if (cur->xpos < 0) cur->xpos = 0;
  if (cur->xpos >= cur->columns) cur->xpos = cur->columns-1;
  if (cur->ypos < 0) cur->ypos = 0;
  if (cur->ypos >= cur->rows) cur->ypos = cur->rows-1;
  move_cursor(cur);
}

void delete_chars_in_line(struct virtscreen *cur, int char_move)
{
   unsigned char *next_move;
   unsigned char *end_move;
   unsigned char *start_move;
   unsigned char clr_with;

   clr_with = /*(cur->attrib << 8) |*/ 0x20; 
   start_move = loc_in_virtscreen(cur,cur->ypos,cur->xpos);
   end_move = loc_in_virtscreen(cur,cur->ypos,cur->columns);
   if ((cur->xpos + char_move) < cur->columns)
   {
     next_move = loc_in_virtscreen(cur,cur->ypos,cur->xpos+char_move);
     while (next_move<end_move)
       *start_move++ = *next_move++;
   }
   while (start_move<end_move) *start_move++ = clr_with;

       /* XXX */
   RefreshRegion(cur->xpos,cur->ypos,cur->columns-cur->xpos,1);   
}

void scroll_virt_up_at_cursor(struct virtscreen *cur, int dir)
{
   unsigned char clr_with;
   unsigned char *beginScr;
   unsigned char *endofScr;
   unsigned char *lastLine;
   unsigned char *nextLine;
   unsigned int real_scroll_length;

   if ((cur->ypos >= cur->top_scroll) &&
       (cur->ypos < cur->bottom_scroll))
   {
     clr_with = /*(cur->attrib << 8) |*/ 0x20;
     beginScr = loc_in_virtscreen(cur,cur->ypos,0);
     endofScr = loc_in_virtscreen(cur,cur->bottom_scroll,0);
     lastLine = endofScr - cur->columns;
     nextLine = beginScr + cur->columns;
     real_scroll_length = lastLine - beginScr;
         /*((unsigned int) lastLine) -
                          ((unsigned int) beginScr); */

     if (dir)
       {
	 MemMove(beginScr,nextLine,real_scroll_length);
         while (lastLine < endofScr)
	   *lastLine++ = clr_with;
       } else
	 {
	   MemMove(nextLine,beginScr,real_scroll_length);
           while (beginScr < nextLine)
	     *beginScr++ = clr_with;
	 }
         /* XXX */
   ScrollRegion(cur->ypos,cur->bottom_scroll-1);
   EraseRegion(0,cur->bottom_scroll-1,cur->columns,1);   
/*   RefreshRegion(0,cur->ypos,cur->columns,cur->bottom_scroll - cur->ypos);*/
   }
}

void scroll_virtscreen(struct virtscreen *cur)
 {
   int clr_with = /*(cur->attrib << 8) |*/ 0x20;
   unsigned char *beginScr = loc_in_virtscreen(cur,cur->top_scroll,0);
   unsigned char *endofScr = loc_in_virtscreen(cur,cur->bottom_scroll,0);
   unsigned char *lastLine = endofScr - cur->columns;
   unsigned char *nextLine = beginScr + cur->columns;
   unsigned int real_scroll_length = lastLine - beginScr
           /*((unsigned int) lastLine) -
                          ((unsigned int) beginScr) */;

/*   memmove2(beginScr,nextLine,real_scroll_length);*/
   MemMove(beginScr,nextLine,real_scroll_length);
   
   while (lastLine < endofScr)
     *lastLine++ = clr_with;

   ScrollRegion(cur->top_scroll,cur->bottom_scroll-1);
   EraseRegion(0,cur->bottom_scroll-1,cur->columns,1);   
 };

void set_scroll_region(struct virtscreen *cur, int low, int high)
{
   low--;
   high = (high >= cur->rows) ? cur->rows : high;
   low = (low < 0) ? 0 : low;

   cur->top_scroll = low;
   cur->bottom_scroll = high;
}

/*
  void change_attribute(struct virtscreen *cur, int attrib)
{
  cur->attrib = attrib;
}
*/

#define change_attribute(a,b) ;

void change_character(struct virtscreen *cur, int ch)
{
  *(loc_in_virtscreen(cur,cur->ypos,cur->xpos)) = ch /* | (cur->attrib << 8) */;

/*  RefreshRegion(cur->xpos,cur->ypos,1,1);*/
  RefreshChar(cur->xpos,cur->ypos);  
}


void special_ansi_charset_0(struct virtscreen *cur, int  ch)
{
  cur->next_char_send = std_interpret_char;
}

void special_ansi_charset_1(struct virtscreen *cur, int ch)
{
  cur->next_char_send = std_interpret_char;
}


void std_interpret_ansi(struct virtscreen *cur, int ch)
{
  switch (ch)
  {
    case '(':  cur->next_char_send = special_ansi_charset_0;
               return;
    case ')':  cur->next_char_send = special_ansi_charset_1;
               return;
    case '[':  cur->cur_ansi_number = 0;
               cur->ansi_elements = 0;
               cur->ansi_reading_number = 0;
               cur->next_char_send = special_reading_ansi;
               return;
    case '7':  cur->old_attrib = cur->attrib;
               cur->old_xpos = cur->xpos;
               cur->old_ypos = cur->ypos;
               break;
    case '8':  change_attribute(cur,cur->old_attrib);
               position_console(cur,cur->ypos,cur->xpos,0);
               break;
    case 'E':  cur->xpos = 0;
    case 'D':  cur->ypos++;
               if (cur->ypos >= cur->bottom_scroll)
               {
                 scroll_virtscreen(cur);
                 cur->ypos--;
               }
               move_cursor(cur);
               break; 
    case 'M':  cur->ypos--;
               if (cur->ypos < cur->top_scroll)
               {
                 cur->ypos++;
                 scroll_virt_up_at_cursor(cur,0);
               }
               move_cursor(cur);
               break;    /* recalculate screen pos */
  }
  if (ch != 27)
    cur->next_char_send = std_interpret_char;
}

void std_interpret_char(struct virtscreen *cur, int ch)
{
    
  switch (ch)
  {
    case 27: cur->next_char_send = std_interpret_ansi;
             return;
    case 12: clear_virtscreen(cur,2);
             break;
    case 13: cur->xpos = 0;        /* return = back to begin */
             break;
    case 10: cur->ypos++;
             if (cur->ypos >= cur->bottom_scroll)
                     /* if we're at bottom */
             {
               if (cur->ypos == cur->bottom_scroll)
		 scroll_virtscreen(cur);        /* and scroll it! */
               cur->ypos--;            /* go back up a line */
             }
             cur->xpos=0;
             break;  
    case 9:  position_console(cur,cur->ypos,cur->xpos+(8-(cur->xpos % 8)),0);
             break;
    case 8:  cur->xpos--;               /* backspace on screen */
             if (cur->xpos<0)
             {
               cur->xpos = cur->columns - 1;
               cur->ypos; // should possibly wrap around sometimes? -- vassilii
			   	// XXX
               if (cur->ypos<0)
		 cur->ypos=0;
             }
             break;
	case 7:
		SndPlaySystemSound(sndWarning);
		break;
    default: if (ch<0x20) /* BJS 4/24 */
                break;
             change_character(cur,ch);
             cur->xpos++;
             if (cur->xpos >= cur->columns)
	       {
#ifdef STRICT_VT100
		 cur->xpos = cur->columns-1;
#else
		 cur->xpos = 0;
                 cur->ypos++;
                 if (cur->ypos == cur->bottom_scroll)
		   {
		     cur->ypos--;
		     scroll_virtscreen(cur);
		   }
#endif
	       }
             break;
    }
  move_cursor(cur);
}

void special_reading_ansi(struct virtscreen *cur, int ch)
{
  if ((ch>='0') && (ch<='9'))
  {
    cur->cur_ansi_number = (cur->cur_ansi_number * 10) + (ch - '0');
    cur->ansi_reading_number = 1;
    return;
  }

  if ((cur->ansi_reading_number) || (ch == ';'))
  {
     if (cur->ansi_elements<MAX_ANSI_ELEMENTS)
      cur->ansi_element[cur->ansi_elements++] = cur->cur_ansi_number;
     cur->ansi_reading_number = 0;
  }
  cur->cur_ansi_number = 0;
  switch (ch)
  {
    case '?':
    case ';':   return;
    case 'D':   position_console(cur,0,(cur->ansi_elements) ?
			 -cur->ansi_element[0] : -1,1);
                break;
    case 'a':
    case 'C':   position_console(cur,0,(cur->ansi_elements) ?
			 cur->ansi_element[0] : 1,1);
                break;
    case 'A':   position_console(cur,(cur->ansi_elements) ? 
		         -cur->ansi_element[0] : -1,0,1);
                break;
    case 'e':
    case 'B':   position_console(cur,(cur->ansi_elements) ?
			 cur->ansi_element[0] : 1,0,1);
                break;
    case '`':
    case 'G':   
                {
                  int temp = cur->ansi_elements ? cur->ansi_element[0] : 1;
		  if (temp)
		    temp--;
                  position_console(cur,cur->ypos,temp,0);
		}
                break;
    case 'E':   position_console(cur,cur->ypos + 
		         ((cur->ansi_elements) ? cur->ansi_element[0] : 1),
			 0,0);
                break;
    case 'F':   position_console(cur,cur->ypos - 
		         ((cur->ansi_elements) ? cur->ansi_element[0] : 1),
			 0,0);
                break;
    case 'd':   
                {
                  int temp = cur->ansi_elements ? cur->ansi_element[0] : 1;
		  if (temp)
		    temp--;
                  position_console(cur,temp,cur->xpos,0);
		}
                break;
    case 'f':
    case 'H':
      {
        int row = (cur->ansi_elements > 0) ? cur->ansi_element[0] : 1;
        int col = (cur->ansi_elements > 1) ? cur->ansi_element[1] : 1;
	if (row)
	  row--;
	if (col)
	  col--;
        position_console(cur,row,col,0);
      }
      break;
    case 'J':   clear_virtscreen(cur,(cur->ansi_elements) ?
		       cur->ansi_element[0]: 0);
                break;
    case 'L':   {
                  int lines = (cur->ansi_elements) ? 
		     cur->ansi_element[0] : 1;
                  while (lines>0)
                  {
                    scroll_virt_up_at_cursor(cur,0);
                    lines--;
                  }
                }
                break;
    case 'M':   {
                  int lines = (cur->ansi_elements) ? 
		     cur->ansi_element[0] : 1;
                  while (lines>0)
                  {
                    scroll_virt_up_at_cursor(cur,1);
                    lines--;
                  }
                }
                break;
    case 'P':   delete_chars_in_line(cur,(cur->ansi_elements) ? 
			 cur->ansi_element[0] : 1);
                break;
    case 'K':   clear_to_eol(cur,cur->ansi_elements ? 
		    cur->ansi_element[0] : 0);
                break;
    case 's':   cur->old_xpos = cur->xpos;
                cur->old_ypos = cur->ypos;
                break;
    case 'u':   position_console(cur,cur->old_ypos,cur->old_xpos,0);
                break;
    case 'r':   {
                  int low = (cur->ansi_elements > 0) ?
		      cur->ansi_element[0] : 1;
                  int high = (cur->ansi_elements > 1) ?
		      cur->ansi_element[1] : cur->rows;
                  if (low<=high)
                    set_scroll_region(cur,low,high);
                }
                break;
    case 'm':
       {
          int count = 0;
          int cthing;
          if (!cur->ansi_elements)
            change_attribute(cur,0x07);
          while (count < cur->ansi_elements)
          {
            cthing = cur->ansi_element[count];
            switch (cthing)
            {
              case 0:
              case 27: change_attribute(cur,0x07);
                       break;
              case 1:  change_attribute(cur,cur->attrib | 0x08);
                       break;
              case 5:  change_attribute(cur,cur->attrib | 0x80);
                       break;
              case 7:  change_attribute(cur,0x70);
                       break;
              case 21:
              case 22: change_attribute(cur,cur->attrib & 0xF7);
                       break;
              case 25: change_attribute(cur,cur->attrib & 0x7F);
                       break;
              default:
                if ((cthing>=30) && (cthing<=37))
		  {
		    change_attribute(cur,(cur->attrib & 0xF8) | (cthing-30));
		  }
                if ((cthing>=40) && (cthing<=47))
		  {
		    change_attribute(cur,(cur->attrib & 0x8F) | 
				     ((cthing-40) << 4));
		  }
                break;
	      }
            count++;
	  }
	}
      break;
      
  }
  cur->next_char_send = std_interpret_char;
}



