#include "pgrx.h"
#include <math.h>
#include "allegro.h"
#include "allegro/internal/aintern.h"

graphics_context::graphics_context(BITMAP* s, int x, int y, const ptheme& tt) 
: bmp(s), t(tt), ox(x), oy(y), ct(s->ct), cr(s->cr), cb(s->cb), cl(s->cl)
{ }  

graphics_context::~graphics_context()
{ 
  bmp->ct = ct; 
  bmp->cr = cr; 
  bmp->cb = cb; 
  bmp->cl = cl; 
}   

void graphics_context::clip(int x1, int y1, int x2, int y2)
{
  ASSERT(bmp);
  ASSERT(x1 >= x2);
  ASSERT(y1 >= y2);

  x2++;
  y2++;
  
  x1 = MID(0, x1, bmp->w-1);
  y1 = MID(0, y1, bmp->h-1);
  x2 = MID(0, x2, bmp->w);
  y2 = MID(0, y2, bmp->h);

  x1 = PLIM(x1, cl, cr);
  y1 = PLIM(y1, ct, cb);
  x2 = PLIM(x2, cl, cr);
  y2 = PLIM(y2, ct, cb);

  bmp->clip = TRUE;
  bmp->cl = x1;
  bmp->ct = y1;
  bmp->cr = x2;
  bmp->cb = y2;

  if (bmp->vtable->set_clip) bmp->vtable->set_clip(bmp);
}

void graphics_context::render_backdrop(coord_int& x, coord_int& y, const zone& z, coord_int w, coord_int h, int col, compass_orientation a, coord_int o) const
{  
  switch (a)
  {
    case c_north_west: 
      y = z.ay;                  
      x = z.ax;                  
      break;
    case c_north:      
      y = z.ay;                  
      x = (int)ceil((z.ax+z.bx)/2.0-w/2.0); 
      break;
    case c_north_east: 
      y = z.ay;                  
      x = z.bx-w;                
      break;
    case c_west:       
      y = (int)ceil((z.ay+z.by)/2.0-h/2.0); 
      x = z.ax;                  
      break;
    case c_centre:     
      y = (int)ceil((z.ay+z.by)/2.0-h/2.0); 
      x = (int)ceil((z.ax+z.bx)/2.0-w/2.0); 
      break;
    case c_east:
      y = (int)ceil((z.ay+z.by)/2.0-h/2.0); 
      x = z.bx-w;                
      break;
    case c_south_west: 
      y = z.by-h;                
      x = z.ax;                  
      break;
    case c_south:      
      y = z.by-h;                
      x = (int)ceil((z.ax+z.bx)/2.0-w/2.0); 
      break;
    case c_south_east: 
      y = z.by-h;                
      x = z.bx-w;                
      break;
  }
  
  x += o;
  y += o;
  
  zone* margins = new zone(z);    
  occlude(margins, &zone(x, y, x+w, y+h));

  while (margins)
  {
    zone* next = margins->next;
    rectfill(margins->ax, margins->ay, margins->bx, margins->by, col);

    delete margins;
    margins = next;
  }
}

int lines_in_multiline(FONT* font, const std::string& str, const zone& z, bool wrap)
{   
  int str_h = text_height(font);
  int spc_w = text_length(font, " ");
  int x_margin = z.ax + 4;
  int cur_x = x_margin;
  int cur_y = z.ay + 3;
  int lines = 1; 
   
  for (std::string::size_type cur = 0, last_word = 0; cur <= str.length(); cur++)
  {
    char c = str[cur];
    
    if ((c <= 32) || (cur == str.length()))
    {
      const char* sub = std::string(str, last_word, cur-last_word).c_str();
      
      int str_w = text_length(font, sub);

      if (wrap)
      if ((cur_x + str_w > z.bx && x_margin + str_w < z.bx)
      || (x_margin + str_w >= z.bx && cur_x != x_margin))
      {
        lines++;
        cur_x = x_margin;
        cur_y += str_h;
      }

      cur_x += str_w;
      cur_x += spc_w;
      last_word = cur + 1;
    }

    if (c == '\n')
    {
      lines++;
      cur_x = x_margin;
      cur_y += str_h;
    }
  } 
 
  return lines;
}

int char_to_line(FONT* font, const std::string& str, const zone& z, int cpos, bool wrap)
{
  int str_h = text_height(font);
  int spc_w = text_length(font, " ");
  int x_margin = z.ax + 4;
  int cur_x = x_margin;
  int cur_y = z.ay + 3;
  int cur_line = 1;
     
  for (std::string::size_type cur = 0, last_word = 0; cur <= str.length(); cur++)
  {
    char c = str[cur];
    
    if ((c <= 32) || (cur == str.length()))
    {
      const char* sub = std::string(str, last_word, cur-last_word).c_str();
      
      int str_w = text_length(font, sub);

      if (wrap)
      if ((cur_x + str_w > z.bx && x_margin + str_w < z.bx)
      || (x_margin + str_w >= z.bx && cur_x != x_margin))
      {
        cur_x = x_margin;
        cur_y += str_h;
        cur_line++;
      }

      if (cur >= cpos)
      {
        return cur_line;
      }
                                       
      cur_x += str_w;
      cur_x += spc_w;
      last_word = cur + 1;
    }

    if (c == '\n')
    {    
      cur_x = x_margin;
      cur_y += str_h;
      cur_line++;
    }
  } 
  
  return cur_line;
}

int first_char_in_line(FONT* font, const std::string& str, const zone& z, int cpos, bool wrap)
{
  int str_h = text_height(font);
  int spc_w = text_length(font, " ");
  int x_margin = z.ax + 4;
  int cur_x = x_margin;
  int cur_y = z.ay + 3;
  int cur_line = 1, last_line = 0;
     
  for (std::string::size_type cur = 0, last_word = 0; cur <= str.length(); cur++)
  {
    char c = str[cur];
    
    if ((c <= 32) || (cur == str.length()))
    {
      const char* sub = std::string(str, last_word, cur-last_word).c_str();
      
      int str_w = text_length(font, sub);

      if (wrap)
      if ((cur_x + str_w > z.bx && x_margin + str_w < z.bx)
      || (x_margin + str_w >= z.bx && cur_x != x_margin))
      {
        cur_x = x_margin;
        cur_y += str_h;
        cur_line++;
        last_line = last_word;
      }

      if (cur >= cpos) return last_line;
                                             
      cur_x += str_w;
      cur_x += spc_w;
      last_word = cur + 1;
    }

    if (c == '\n')
    {    
      last_line = cur+1;
      cur_x = x_margin;
      cur_y += str_h;
      cur_line++;
    }
  } 
  
  return last_line;
}

int line_to_char(FONT* font, const std::string& str, const zone& z, int line, bool wrap)
{
  int str_h = text_height(font);
  int spc_w = text_length(font, " ");
  int x_margin = z.ax + 4;
  int cur_x = x_margin;
  int cur_y = z.ay + 3;
  int cur_line = 1;
     
  for (std::string::size_type cur = 0, last_word = 0; cur <= str.length(); cur++)
  {
    char c = str[cur];
    
    if ((c <= 32) || (cur == str.length()))
    {
      const char* sub = std::string(str, last_word, cur-last_word).c_str();
      
      int str_w = text_length(font, sub);

      if (wrap)
      if ((cur_x + str_w > z.bx && x_margin + str_w < z.bx)
      || (x_margin + str_w >= z.bx && cur_x != x_margin))
      {
        cur_x = x_margin;
        cur_y += str_h;
        cur_line++;
      }

      if (cur_line >= line)
      {
        return last_word;
      }
                                       
      cur_x += str_w;
      cur_x += spc_w;
      last_word = cur + 1;
    }

    if (c == '\n')
    {    
      cur_x = x_margin;
      cur_y += str_h;
      cur_line++;
    }
  } 
  
  return str.length();
}

int chars_in_line(FONT* font, const std::string& str, const zone& z, int line, bool wrap)
{
  int str_h = text_height(font);
  int spc_w = text_length(font, " ");
  int x_margin = z.ax + 4;
  int cur_x = x_margin;
  int cur_y = z.ay + 3;
  int cur_line = 1;
  int chars = 0;
     
  for (std::string::size_type cur = 0, last_word = 0; cur <= str.length(); cur++)
  {
    char c = str[cur];  
    
    if (line == cur_line && c > 32) chars++;
    
    if ((c <= 32) || (cur == str.length()))
    {
      std::string string_sub(str, last_word, cur-last_word);
      const char* sub = string_sub.c_str();
      
      int str_w = text_length(font, sub);

      if (wrap)
      if ((cur_x + str_w > z.bx && x_margin + str_w < z.bx)
      || (x_margin + str_w >= z.bx && cur_x != x_margin))
      {
        cur_x = x_margin;
        cur_y += str_h;
        cur_line++;
        if (line == cur_line) chars = string_sub.length();
      }

      if (cur_line > line) return chars;
                                       
      cur_x += str_w;
      cur_x += spc_w;
      last_word = cur + 1;
    }

    if (c == '\n')
    {    
      cur_x = x_margin;
      cur_y += str_h;
      cur_line++;
    }
  } 
  
  return chars;
}


void graphics_context::render_multiline(const std::string& str, const zone& z, int bc, int cpos, int line, bool wrap) const
{
  if (!intersect(bmp, real(z))) return;
  clipper clip(*this, z.ax, z.ay, z.bx, z.by);
    
  text_mode(bc);

  int str_h = text_height(t.font);
  int spc_w = text_length(t.font, " ");
  int x_margin = z.ax + 4;
  int cur_x = x_margin;
  int cur_y = z.ay + 3;
  int cur_line = 0;
  
  rectfill(z.ax, z.ay, x_margin-1, z.by, bc);
  rectfill(z.ax, z.ay, z.bx, z.ay+2, bc);
   
  for (std::string::size_type cur = 0, last_word = 0; cur <= str.length(); cur++)
  {
    char c = str[cur];
    
    if ((c <= 32) || (cur == str.length()))
    {
      const char* sub = std::string(str, last_word, cur-last_word).c_str();
      
      int str_w = text_length(t.font, sub);

      if (wrap)
      if ((cur_x + str_w > z.bx && x_margin + str_w < z.bx)
      || (x_margin + str_w >= z.bx && cur_x != x_margin))
      {
        if (cur_line >= line)
        {
          rectfill(cur_x, cur_y, z.bx, cur_y+str_h-1, bc);
          cur_y += str_h;
        } else cur_line++;
        
        cur_x = x_margin;
      }
      if (cur_line >= line) rectfill(cur_x+str_w, cur_y, cur_x+str_w+spc_w-1, cur_y+str_h-1, bc);

      if (cur_y >= z.by) break;
      
      if (cur_line >= line) 
      {
        textout(sub, cur_x, cur_y);
        
        if (cur >= cpos && cpos >= last_word) 
        {
          int cx = cur_x;
          for (std::string::size_type n = last_word; n < cpos; n++) 
            cx += t.font->vtable->char_length(t.font, str[n]);
     
          vline(cx-1, cur_y, cur_y+str_h-1, t.text);
        }  
      }   
              
      cur_x += str_w;
      cur_x += spc_w;
      last_word = cur + 1;
    }

    if (c == '\n')// && ++line > sline)
    {
      if (cur_line >= line)
      {
        rectfill(cur_x, cur_y, z.bx, cur_y+str_h-1, bc);
        cur_y += str_h;
      } else cur_line++;
       
      cur_x = x_margin;
    }
  } 
  
  if (cur_y < z.by)
  {
    rectfill(cur_x, cur_y, z.bx, cur_y+str_h-1, bc);
    rectfill(z.ax, cur_y+str_h, z.bx, z.by, bc);
  }
} 

void find_multiline_coords(FONT* font, const std::string& str, const zone& z, int cpos, coord_int& cx, coord_int& cy, int line, bool wrap)
{
  int str_h = text_height(font);
  int spc_w = text_length(font, " ");
  int x_margin = z.ax + 4;
  int cur_x = x_margin;
  int cur_y = z.ay + 3;
  int cur_line = 0;
   
  for (std::string::size_type cur = 0, last_word = 0; cur <= str.length(); cur++)
  {
    char c = str[cur];
    
    if ((c <= 32) || (cur == str.length()))
    {
      const char* sub = std::string(str, last_word, cur-last_word).c_str();
      
      int str_w = text_length(font, sub);

      if (wrap)
      if ((cur_x + str_w > z.bx && x_margin + str_w < z.bx)
      || (x_margin + str_w >= z.bx && cur_x != x_margin))
      {
        if (cur_line >= line)
        {
          cur_y += str_h;
        } else cur_line++;
        
        cur_x = x_margin;
      }

      if (cur_y >= z.by) break;
      
      
      if (cur >= cpos && cpos >= last_word) 
      {
        cx = cur_x, cy = cur_y;
        for (std::string::size_type n = last_word; n < cpos; n++) 
          cx += font->vtable->char_length(font, str[n]);
     
        return;
      }   
              
      cur_x += str_w;
      cur_x += spc_w;
      last_word = cur + 1;
    }

    if (c == '\n')
    {
      if (cur_line >= line)
      {
        cur_y += str_h;
      } else cur_line++;
       
      cur_x = x_margin;
    }
  } 
  cx = cur_x;
  cy = cur_y;  
}

std::string::size_type find_multiline_index(FONT* font, const std::string& str, const zone& z, coord_int cx, coord_int cy, int line, bool wrap)
{
  int str_h = text_height(font);
  int spc_w = text_length(font, " ");
  int x_margin = z.ax + 4;
  int cur_x = x_margin;
  int cur_y = z.ay + 3;
  int cur_line = 0;
   
  for (std::string::size_type cur = 0, last_word = 0; cur <= str.length(); cur++)
  {
    char c = str[cur];
    
    if ((c <= 32) || (cur == str.length()))
    {
      const char* sub = std::string(str, last_word, cur-last_word).c_str();
      
      int str_w = text_length(font, sub);

      if (wrap)
      if ((cur_x + str_w > z.bx && x_margin + str_w < z.bx)
      || (x_margin + str_w >= z.bx && cur_x != x_margin))
      {
        if (cur_line >= line)
        {
          cur_y += str_h;
        } else cur_line++;

        if (cur_y > cy) return last_word ? last_word-1 : 0;
   
        cur_x = x_margin;        
      }
      
      if (cur_line >= line)
      if ((cur_x + str_w + spc_w > cx) && (cur_y + str_h > cy))
      {
        int x = cur_x;
        for (std::string::size_type n = last_word; n <= cur; n++)
        {
          x += font->vtable->char_length(font, str[n]);
          if (x > cx + font->vtable->char_length(font, str[n])/2) 
            return n;
        }
      }

      cur_x += str_w;
      cur_x += spc_w;
      last_word = cur + 1;
    }

    if (c == '\n')
    {
      if (cur_line >= line)
      {
        cur_y += str_h;
      } else cur_line++;
       
      cur_x = x_margin;
    }
    
  } 
  return str.length();
} 

void graphics_context::render_line(compass_orientation align, const char* str, const zone& z, int fc, int bc, int cpos, coord_int off_x, coord_int off_y) const
{
  //if (!intersect(bmp, real(z))) return;

  clipper clip(*this, z.ax, z.ay, z.bx, z.by);
  
  int text_w = text_length(t.font, str);
  int text_h = text_height(t.font);
  coord_int mx = 4;
  coord_int my = 4;

  coord_int ax = z.ax + mx;
  coord_int ay = z.ay + my;
  coord_int bx = z.bx - mx;   
  coord_int by = z.by - my;

  float start_x, start_y, end_x, end_y;
  switch (align)
  {
    case c_north_west: start_y = ay; start_x = ax; break;
    case c_north: start_y = ay; start_x = (ax + bx) / 2.0 - text_w / 2.0; break;
    case c_north_east: start_y = ay; start_x = bx - text_w; break;
    case c_west: start_y = (ay + by) / 2.0 - text_h / 2.0; start_x = ax; break;
    case c_centre: start_y = (ay + by) / 2.0 - text_h / 2.0; start_x = (ax + bx) / 2.0 - text_w / 2.0; break;
    case c_east: start_y = (ay + by) / 2.0 - text_h / 2.0; start_x = bx - text_w; break;
    case c_south_west: start_y = by - text_h; start_x = ax; break;
    case c_south: start_y = by - text_h; start_x = (ax + bx) / 2.0 - text_w / 2.0; break;
    case c_south_east: start_y = by - text_h; start_y = bx - text_w; break;
  }

  start_x = int(ceil(start_x)) + off_x;
  start_y = int(ceil(start_y)) + off_y;
  end_x = start_x + text_w - 1;
  end_y = start_y + text_h - 1;

  zone text_zone = zone(start_x, start_y, end_x, end_y);
  zone* margins = new zone(z);
  occlude(margins, &text_zone);

  while (margins)
  {
    zone* next = margins->next;
    rectfill(margins->ax, margins->ay, margins->bx, margins->by, bc);

    delete margins;
    margins = next;
  }

  text_mode(bc);
  textout(str, (int)start_x, (int)start_y, fc);
 
  if (cpos > -1)
  {
    for (int ch = 0, n = 0; (ch = ugetxc(&str)) && n < cpos; n++) start_x += t.font->vtable->char_length(t.font, ch);
   
    vline((int)start_x - 1, int(start_y), int(end_y), fc);
  }
}

std::string::size_type find_line_index(FONT* font, compass_orientation align, const char* str, const zone& z, coord_int cx, coord_int ox, coord_int oy)
{
  int text_w = text_length(font, str);
  int text_h = text_height(font);
  
  coord_int mx = 4;
  coord_int my = 4;

  coord_int ax = z.ax + mx;
  coord_int ay = z.ay + my;
  coord_int bx = z.bx - mx;
  coord_int by = z.by - my;

  float start_x, start_y, end_x, end_y;
  switch (align)
  {
    case c_north_west: start_y = ay; start_x = ax; break;
    case c_north: start_y = ay; start_x = (ax + bx) / 2.0 - text_w / 2.0; break;
    case c_north_east: start_y = ay; start_x = bx - text_w; break;
    case c_west: start_y = (ay + by) / 2.0 - text_h / 2.0; start_x = ax; break;
    case c_centre: start_y = (ay + by) / 2.0 - text_h / 2.0; start_x = (ax + bx) / 2.0 - text_w / 2.0; break;
    case c_east: start_y = (ay + by) / 2.0 - text_h / 2.0; start_x = bx - text_w; break;
    case c_south_west: start_y = by - text_h; start_x = ax; break;
    case c_south: start_y = by - text_h; start_x = (ax + bx) / 2.0 - text_w / 2.0; break;
    case c_south_east: start_y = by - text_h; start_y = bx - text_w; break;
  }

  start_x = int(ceil(start_x)) + ox;
  start_y = int(ceil(start_y)) + oy;
  end_x = start_x + text_w - 1 + ox;
  end_y = start_y + text_h - 1 + oy;
  
  const char* old_str = str;
  coord_int old_x = int(start_x);
  for (int ch = 0, n = 0; (ch = ugetxc(&str)); start_x += font->vtable->char_length(font, ch), n++)
  {     
    if (start_x >= cx) 
    {
      return n - (((cx - old_x) < (start_x - cx)) ? 1 : 0);
    }
    old_x = int(start_x);
  }
  return strlen(old_str);
}

void graphics_context::draw_frame(coord_int ax, coord_int ay, coord_int bx, coord_int by, frame_type ft) const
{
  ax += ox;
  ay += oy;
  bx += ox;
  by += oy;
  
  int a, b, c, d;
  
  /*  A A A A A A A A C
      A B B B B B B D C
      A B           D C
      A B           D C
      A B           D C
      A D D D D D D D C
      C C C C C C C C C
  */
  
  switch(ft)
  {
    case ft_bevel_in:
      a = t.frame_low;
      b = t.frame_black;
      c = t.frame_white;
      d = t.frame_high;
      break;
    case ft_bevel_out:
      a = t.frame_high;
      b = t.frame_white;
      c = t.frame_black;
      d = t.frame_low;
      break;
    case ft_shallow_in:
      a = t.frame_low;
      b = t.frame;
      c = t.frame_white;
      d = t.frame;
      break;
    case ft_shallow_out:
      a = t.frame_white;
      b = t.frame;
      c = t.frame_low;
      d = t.frame;
      break;
    case ft_drop_in:
      a = t.frame_low;
      b = t.frame_white;
      c = t.frame_white;
      d = t.frame_low;
      break;
    case ft_drop_out:
      a = t.frame_white;
      b = t.frame_low;
      c = t.frame_low;
      d = t.frame_white;
      break;
    case ft_button_out:
      a = t.frame_white;
      b = t.frame;
      c = t.frame_black;
      d = t.frame_low;
      break;
    case ft_button_in:
      b = d = t.frame_low;
      a = c = t.frame_black;
      break;
    case ft_none:
      a = b = c = d = t.frame;
      break;
  }

  ::hline(bmp, ax, ay, bx-1, a);
  ::vline(bmp, ax, ay, by-1, a);
  ::hline(bmp, ax+1, ay+1, bx-2, b);
  ::vline(bmp, ax+1, ay+1, by-2, b);

  ::hline(bmp, ax, by, bx, c);
  ::vline(bmp, bx, ay, by, c);
  ::hline(bmp, ax+1, by-1, bx-1, d);
  ::vline(bmp, bx-1, ay+1, by-1, d);
}

frame_type invert_frame(frame_type ft)
{
  switch (ft)
  {
    case ft_bevel_in: return ft_bevel_out;     case ft_bevel_out: return ft_bevel_in;
    case ft_shallow_in: return ft_shallow_out; case ft_shallow_out: return ft_shallow_in;
    case ft_drop_in: return ft_drop_out;       case ft_drop_out: return ft_drop_in;
    case ft_button_in: return ft_button_out;   case ft_button_out: return ft_button_in;
    default: return ft;
  }
}

void ptheme::load()  // Take into account colour-depth in constructor!
{
  dither_pattern = create_bitmap(2,2);
  
  up_arrow = create_bitmap(7, 4);
  down_arrow = create_bitmap(7, 4);
  left_arrow = create_bitmap(4, 7);
  right_arrow = create_bitmap(4, 7);
  
  font = ::font;

  white = makecol(255,255,255);
  black = makecol(0,0,0);

  frame_white = makecol(255,255,255);
  frame_high = makecol(224,220,216);
  frame = makecol(212,208,200);
  frame_low = makecol(128,128,128);
  frame_black = makecol(0,0,0);

  text = makecol(0,0,0);

  pane = makecol(255,255,255);

  bar = makecol(12,42,130);
  bar_inactive = makecol(10,10,100);
  bar_text = makecol(255,255,255);
  
  putpixel(dither_pattern, 0, 0, white);
  putpixel(dither_pattern, 0, 1, frame);
  putpixel(dither_pattern, 1, 0, frame);
  putpixel(dither_pattern, 1, 1, white);  
  
  hline(up_arrow, 0, 0, 2, frame); putpixel(up_arrow, 3, 0, frame_black); hline(up_arrow, 4, 0, 6, frame);
  hline(up_arrow, 0, 1, 1, frame); hline(up_arrow, 2, 1, 4, frame_black); hline(up_arrow, 5, 1, 6, frame);
  putpixel(up_arrow, 0, 2, frame); hline(up_arrow, 1, 2, 5, frame_black); putpixel(up_arrow, 6, 2, frame);
  hline(up_arrow, 0, 3, 6, frame_black);
  
  hline(down_arrow, 0, 3, 2, frame); putpixel(down_arrow, 3, 3, frame_black); hline(down_arrow, 4, 3, 6, frame);
  hline(down_arrow, 0, 2, 1, frame); hline(down_arrow, 2, 2, 4, frame_black); hline(down_arrow, 5, 2, 6, frame);
  putpixel(down_arrow, 0, 1, frame); hline(down_arrow, 1, 1, 5, frame_black); putpixel(down_arrow, 6, 1, frame);
  hline(down_arrow, 0, 0, 6, frame_black);
  
  hline(left_arrow, 0, 0, 2, frame); putpixel(left_arrow, 3, 0, frame_black);
  hline(left_arrow, 0, 1, 1, frame); hline(left_arrow, 2, 1, 3, frame_black);
  putpixel(left_arrow, 0, 2, frame); hline(left_arrow, 1, 2, 3, frame_black);
  hline(left_arrow, 0, 3, 3, frame_black);
  putpixel(left_arrow, 0, 4, frame); hline(left_arrow, 1, 4, 3, frame_black);
  hline(left_arrow, 0, 5, 1, frame); hline(left_arrow, 2, 5, 3, frame_black);
  hline(left_arrow, 0, 6, 2, frame); putpixel(left_arrow, 3, 6, frame_black);
  
  hline(right_arrow, 3, 0, 1, frame); putpixel(right_arrow, 0, 0, frame_black);
  hline(right_arrow, 3, 1, 2, frame); hline(right_arrow, 1, 1, 0, frame_black);
  putpixel(right_arrow, 3, 2, frame); hline(right_arrow, 2, 2, 0, frame_black);
  hline(right_arrow, 0, 3, 3, frame_black);
  putpixel(right_arrow, 3, 4, frame); hline(right_arrow, 2, 4, 0, frame_black);
  hline(right_arrow, 3, 5, 2, frame); hline(right_arrow, 1, 5, 0, frame_black);
  hline(right_arrow, 3, 6, 1, frame); putpixel(right_arrow, 0, 6, frame_black);
}

void ptheme::unload()
{
  destroy_bitmap(dither_pattern);
  destroy_bitmap(left_arrow);
  destroy_bitmap(right_arrow);
  destroy_bitmap(up_arrow);
  destroy_bitmap(down_arrow);
  
  left_arrow = right_arrow = up_arrow = down_arrow = 0;
  dither_pattern = 0;
  font = 0;
  
  white = black = frame_white = frame_high = frame = frame_low = frame_black = text = pane = bar = bar_inactive = bar_text = 0;
}  

void graphics_context::set_mode_normal() const 
{ 
  drawing_mode(DRAW_MODE_SOLID, 0, 0, 0); 
}

void graphics_context::set_mode_dither() const 
{ 
  drawing_mode(DRAW_MODE_COPY_PATTERN, t.dither_pattern, ox, oy); 
}

void graphics_context::putpixel(int x, int y, int col) const 
{ 
  ::putpixel(bmp, x+ox, y+oy, col); 
}

void graphics_context::rectfill(int x1, int y1, int x2, int y2, int col) const 
{ 
  ::rectfill(bmp, x1+ox, y1+oy, x2+ox, y2+oy, col); 
}

void graphics_context::rectfill(const zone* z, int col) const 
{ 
  ::rectfill(bmp, z->ax+ox, z->ay+oy, z->bx+ox, z->by+oy, col); 
}

void graphics_context::line(int x1, int y1, int x2, int y2, int col) const
{
  ::line(bmp, x1+ox, y1+oy, x2+ox, y2+oy, col);
}

void graphics_context::do_line(int x1, int y1, int x2, int y2, int d, void (*proc)(BITMAP*, int, int, int)) const
{
  ::do_line(bmp, x1, y1, x2, y2, d, proc);
}

void graphics_context::circle(int x, int y, int r, int col) const
{
  ::circle(bmp, x+ox, y+oy, r, col);
}

void graphics_context::circle_fill(int x, int y, int r, int col) const
{
  ::circlefill(bmp, x+ox, y+oy, r, col);
}

void graphics_context::fill(int x, int y, int col) const
{
  ::floodfill(bmp, x+ox, y+oy, col);
}

void graphics_context::hline(int x1, int y, int x2, int col) const 
{ 
  ::hline(bmp, x1+ox, y+oy, x2+ox, col); 
}

void graphics_context::vline(int x, int y1, int y2, int col) const 
{ 
  ::vline(bmp, x+ox, y1+oy, y2+oy, col); 
}

void graphics_context::blit(BITMAP* src, int sx, int sy, int dx, int dy, int w, int h) const 
{ 
  ::blit(src, bmp, sx, sy, dx+ox, dy+oy, w, h); 
}

void graphics_context::blit(BITMAP* src, int x, int y) const
{
  ::blit(src, bmp, 0, 0, x+ox, y+oy, src->w, src->h);
}

void graphics_context::rect(int x1, int y1, int x2, int y2, int c) const 
{ 
  ::rect(bmp, x1+ox, y1+oy, x2+ox, y2+oy, c); 
}

void graphics_context::textout(const char* str, int x, int y) const 
{ 
  ::textout(bmp, t.font, str, x+ox, y+oy, t.text); 
}

void graphics_context::textout(const char* str, int x, int y, int col) const 
{ 
  ::textout(bmp, t.font, str, x+ox, y+oy, col); 
}

void graphics_context::stretch_blit(BITMAP* src, int sx, int sy, int sw, int sh, int dx, int dy, int dw, int dh) const 
{ 
  ::stretch_blit(src, bmp, sx, sy, sw, sh, dx+ox, dy+oy, dw, dh); 
}

int graphics_context::font_width(const std::string& s) const
{
  return text_length(t.font, s.c_str());
}

int graphics_context::font_height() const
{
  return text_height(t.font);
}

clipper::clipper(const graphics_context& c, coord_int _ax, coord_int _ay, coord_int _bx, coord_int _by)
: surface(c)
{
  ax = surface->cl; ay = surface->ct; bx = surface->cr; by= surface->cb;
  _ax += c.ox; _ay += c.oy; _bx += c.ox; _by += c.oy;
  
  if (_ax >= surface->cr || _ay >= surface->cb || _bx < surface->cl || _by < surface->ct)
  
    bmp_clip(surface, 0, 0, 0, 0);
  
  else
        
  bmp_clip(surface, PLIM(_ax, surface->cl, surface->cr-1), PLIM(_ay, surface->ct, surface->cb-1), PLIM(_bx, surface->cl, surface->cr-1), PLIM(_by, surface->ct, surface->cb-1)); 
}

clipper::~clipper()
{
  surface->cl = ax; surface->ct = ay; surface->cr = bx; surface->cb = by;
}

void bmp_clip(BITMAP* s, coord_int x1, coord_int y1, coord_int x2, coord_int y2)
{
  ASSERT(s);
  ASSERT(x1 >= x2);
  ASSERT(y1 >= y2);

  x2++;
  y2++;
  
  s->clip = TRUE;
  x1 = MID(0, x1, s->w-1);
  y1 = MID(0, y1, s->h-1);
  x2 = MID(0, x2, s->w);
  y2 = MID(0, y2, s->h);
  
  s->ct = y1;
  s->cl = x1;    
  s->cr = x2;
  s->cb = y2;    
        
  if (s->vtable->set_clip) s->vtable->set_clip(s);
}

bool intersect(BITMAP* s, coord_int x, coord_int y)
{
  if (x < s->cl || x >= s->cr || y < s->ct || y >= s->cb)
    return false;
  else
    return true;
}

bool intersect(BITMAP* s, coord_int ax, coord_int ay, coord_int bx, coord_int by)
{
  if (bx < s->cl || ax >= s->cr || by < s->ct || ay >= s->cb)
    return false;
  else
    return true;
}

bool intersect(BITMAP* s, const zone& z)
{
  if (z.bx < s->cl || z.ax >= s->cr || z.by < s->ct || z.ay >= s->cb)
    return false;
  else
    return true;
}

void reversed_blit(BITMAP* src, BITMAP* dst, int sx, int sy, int dx, int dy, int w, int h)
{
  int x, y;
  
  if ((sx >= src->w) || (sy >= src->h) ||
      (dx >= dst->cr) || (dy >= dst->cb))
     return;

  /* clip src left */
  if (sx < 0) {
    w += sx;
    dx -= sx;
    sx = 0;
  }

   /* clip src top */
  if (sy < 0) {
    h += sy;
    dy -= sy;
    sy = 0;
  }

  /* clip src right */
  if (sx+w > src->w)
    w = src->w - sx;
    
  /* clip src bottom */
  if (sy+h > src->h)
    h = src->h - sy;

  /* clip dst left */
  if (dx < dst->cl) {
    dx -= dst->cl;
    w += dx;
    sx -= dx;
    dx = dst->cl;
  }

  /* clip dst top */
  if (dy < dst->ct) {
    dy -= dst->ct;
    h += dy;
    sy -= dy;
    dy = dst->ct;
  }

  /* clip dst right */
  if (dx+w > dst->cr)
    w = dst->cr - dx;

  /* clip dst bottom */
  if (dy+h > dst->cb)
    h = dst->cb - dy;

  /* bottle out if zero size */
  if ((w <= 0) || (h <= 0))
    return;

  switch(bitmap_color_depth(dst))
  {
    case 8:
      for (y = 0; y < h; y++)
      {
        unsigned char* s = ((unsigned char*)(bmp_read_line(src, sy + y)) + sx);
        unsigned char* d = ((unsigned char*)(bmp_write_line(dst, dy + y)) + dx);

        for (x = w - 1; x >= 0; s++, d++, x--)
        {
          unsigned long c;

          bmp_select(src);
          c = bmp_read8((unsigned long)(s));

          bmp_select(dst);
          bmp_write8((unsigned long)(d), ~c);
        }
      }
      bmp_unwrite_line(src);
      bmp_unwrite_line(dst);

    break;
    case 15:
      for (y = 0; y < h; y++)
      {
        unsigned short* s = ((unsigned short*)(bmp_read_line(src, sy + y)) + sx);
        unsigned short* d = ((unsigned short*)(bmp_write_line(dst, dy + y)) + dx);

        for (x = w - 1; x >= 0; s++, d++, x--)
        {
          unsigned long c;

          bmp_select(src);
          c = bmp_read15((unsigned long)(s));

          bmp_select(dst);
          bmp_write15((unsigned long)(d), ~c);
        }
      }
      bmp_unwrite_line(src);
      bmp_unwrite_line(dst);

    break;
    case 16:
      for (y = 0; y < h; y++)
      {
        unsigned short* s = ((unsigned short*)(bmp_read_line(src, sy + y)) + sx);
        unsigned short* d = ((unsigned short*)(bmp_write_line(dst, dy + y)) + dx);

        for (x = w - 1; x >= 0; s++, d++, x--)
        {
          unsigned long c;

          bmp_select(src);
          c = bmp_read16((unsigned long)(s));

          bmp_select(dst);
          bmp_write16((unsigned long)(d), ~c);
        }
      }
      bmp_unwrite_line(src);
      bmp_unwrite_line(dst);

    break;
    case 24:
      for (y = 0; y < h; y++)
      {
        unsigned char* s = ((unsigned char*)(bmp_read_line(src, sy + y)) + 3 * sx);
        unsigned char* d = ((unsigned char*)(bmp_write_line(dst, dy + y)) + 3 * dx);

        for (x = w - 1; x >= 0; s += 3, d += 3, x--)
        {
          unsigned long c;

          bmp_select(src);
          c = bmp_read24((unsigned long)(s));

          bmp_select(dst);
          bmp_write24((unsigned long)(d), ~c);
        }
      }
      bmp_unwrite_line(src);
      bmp_unwrite_line(dst);

    break;
    case 32:
      for (y = 0; y < h; y++)
      {
        unsigned long* s = ((unsigned long*)(bmp_read_line(src, sy + y)) + sx);
        unsigned long* d = ((unsigned long*)(bmp_write_line(dst, dy + y)) + dx);

        for (x = w - 1; x >= 0; s++, d++, x--)
        {
          unsigned long c;

          bmp_select(src);
          c = bmp_read32((unsigned long)(s));

          bmp_select(dst);
          bmp_write32((unsigned long)(d), ~c);
        }
      }
      bmp_unwrite_line(src);
      bmp_unwrite_line(dst);
  }
}


