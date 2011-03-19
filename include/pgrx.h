#ifndef PGRX_H
#define PGRX_H

#include "pdefs.h"
#include <string>

struct BITMAP;
struct FONT;

// Used to provide a consistent set of colours, bitmaps, and patterns for widgets to use
struct ptheme
{
  FONT* font; // Basic font for text rendering

  BITMAP* dither_pattern; // Checker-board pattern for scrollbars
  BITMAP* up_arrow; // Little black arrow pointing UP
  BITMAP* down_arrow; // Little black arrow pointing DOWN
  BITMAP* left_arrow; // Little black arrow pointing LEFT
  BITMAP* right_arrow; // Little black arrow pointing RIGHT
  
  int white; // The colour closest to white this mode can display
  int black; // The colour closest to black this mode can display
  int frame_white; // Brighest high-light on top-left frame edge
  int frame_high;  // Mild high-light on frame, used for softer bevels
  int frame;       // Colour of surface of window
  int frame_low;   // Dark grey used for shadow on frame
  int frame_black; // Black, used for very edge of frame
  int pane;        // Colour of any editable-text surface
  int pane_disabled; // Colour of any non-editable-text surface
  int text;          // Colour most text should appear (usually black)
  int bar;           // Colour of top-level window-bar
  int bar_inactive;  // Colour of top-level window-bar when deselected
  int bar_text;      // Colour of top-level window-bar's text
  
  void load();    // Allocates all resources
  void unload();  // De-allocates all resources
}; 

// Enum representing a type of frame to be drawn by 'draw_frame'
enum frame_type
{
  ft_bevel_in,
  ft_bevel_out,
  ft_shallow_in,
  ft_shallow_out,
  ft_drop_in,
  ft_drop_out,
  ft_button_in,
  ft_button_out,
  ft_none
}; 
frame_type invert_frame(frame_type ft); // Returns the opposite of 'ft' (in/out)

// Complex text functions
int char_to_line(FONT* font, const string& str, const zone& z, int cpos, bool wrap =true);
int first_char_in_line(FONT* font, const string& str, const zone& z, int cpos, bool wrap =true);
int line_to_char(FONT* font, const string& str, const zone& z, int cpos, bool wrap =true);
int chars_in_line(FONT* font, const string& str, const zone& z, int line, bool wrap=true);
int lines_in_multiline(FONT* font, const string& str, const zone& z, bool wrap =true);
void find_multiline_coords(FONT* font, const string& str, const zone& z, int i, coord_int& cx, coord_int& cy, int line =0, bool wrap =true);
string::size_type find_multiline_index(FONT* font, const string& str, const zone& z, coord_int cx, coord_int cy, int line =0, bool wrap =true);
string::size_type find_line_index(FONT* font, compass_orientation align, const char* str, const zone& z, coord_int cx, coord_int ox=0, coord_int oy =0);

class graphics_context
{
  private:
  
    BITMAP* const bmp; // Bitmap onto which graphical operations will be applied
    const ptheme& t; // Theme that should be used when referencing standard colours
    
    const int ox; // Horizontal offset, applied to all operations before they are performed
    const int oy; // Vertical offset,       "            "
    
    const int ct; // Clip rectangle as it was before the graphics context 
    const int cr; // changed it.
    const int cb;
    const int cl;
    
    // Returns a zone, changed by the current horizontal and vertical offsets
    zone real(const zone& z) const { return zone(z.ax+ox,z.ay+oy,z.bx+ox,z.by+oy); }
               
  public:
  
    int get_ox() const { return ox; } // Return offsets
    int get_oy() const { return oy; }
  
    void set_mode_normal() const; // Switch between normal drawing, 
    void set_mode_dither() const; // and drawing with dither pattern.
  
    // Narrow the bitmap's clipping rectangle
    void clip(int x1, int y1, int x2, int y2); 
    void clip(const zone* z) { clip(z->ax, z->ay, z->bx, z->by); }
          
    void draw_frame(coord_int ax, coord_int ay, coord_int bx, coord_int by, frame_type ft) const;
    void render_backdrop(coord_int& x, coord_int& y, const zone& z, coord_int w, coord_int h, int col, compass_orientation a =c_centre, coord_int o =0) const;
    void render_multiline(const string& str, const zone& z, int bc, int cpos=-1, int line =0, bool wrap =true) const;
    void render_line(compass_orientation align, const char* str, const zone& z, int fc, int bc, int cpos =-1, coord_int ox=0, coord_int oy =0) const;

    void putpixel(int x, int y, int col) const;
    void rectfill(int x1, int y1, int x2, int y2, int col) const;
    void rectfill(const zone* z, int col) const;
    void fill(int x, int y, int col) const;
    void line(int x1, int y1, int x2, int y2, int col) const;
    void do_line(int x1, int y1, int x2, int y2, int d, void (*proc)(BITMAP*, int, int, int)) const;
    void circle(int x, int y, int r, int col) const;
    void circle_fill(int x, int y, int r, int col) const;
    void hline(int x1, int y, int x2, int col) const;
    void vline(int x, int y1, int y2, int col) const;
    void blit(BITMAP* src, int sx, int sy, int dx, int dy, int w, int h) const;
    void blit(BITMAP* src, int x, int y) const;
    void rect(int x1, int y1, int x2, int y2, int c) const;
    void textout(const char* str, int x, int y) const;
    void textout(const char* str, int x, int y, int col) const;
    void stretch_blit(BITMAP* src, int sx, int sy, int sw, int sh, int dx, int dy, int dw, int dh) const;

    int font_width(const string& s) const; // Width of the current font for that string
    int font_height() const; // Height of the current font
    
    operator BITMAP*() const { return bmp; } // Return our bitmap if we are treated like one
    
    const ptheme& theme() const { return t; } // Return our theme
    
    graphics_context(BITMAP* s, int x, int y, const ptheme& tt); 
    ~graphics_context();
        
    friend class clipper;   
};  

struct clipper
{
  private:
 
    BITMAP* surface;
    int ax;
    int ay;
    int bx;
    int by;

  public:
                                                                                                       
    clipper(const graphics_context& c, coord_int _ax, coord_int _ay, coord_int _bx, coord_int _by);  
    ~clipper();
};

void bmp_clip(BITMAP* s, coord_int x1, coord_int y1, coord_int x2, coord_int y2);
bool intersect(BITMAP*, const zone& z);
bool intersect(BITMAP* s, coord_int x, coord_int y);
bool intersect(BITMAP* s, coord_int ax, coord_int ay, coord_int bx, coord_int by);
void reversed_blit(BITMAP* src, BITMAP* dst, int sx, int sy, int dx, int dy, int w, int h);
                                                       
#endif
