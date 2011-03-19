#ifndef PMANAGER_H
#define PMANAGER_H

#include "pdefs.h"
#include "pgrx.h"
#include "pevent.h"
#include "pmaster.h"
#include "psublim.h"
#include <vector>
#include <list>

class init_specifier;

/* Window Manager

   The window manager's primary function is to interpet mouseclicks and keypresses and
   dispatch events to the necessary windows. 
*/

class window_manager : public window_master
{
  private:
     
    base_window* io_win; // The top window that events should be dispatched from
     
    base_window* keyfocus; // The window to which key-presses will be dispatched 
    base_window* target; // The window currently residing under the mouse cursor
    base_window* drag_target; // The window that is currently being dragged
    
    base_window* o_target;
    base_window* gloop;
  
    BITMAP* cursor_library[cursor_last]; // Stores all the standard cursors for use by windows
    FONT* console_font;
   
    int frame;
    int hold_t;
  
    masked_image* cursor;
  
    bool poll_in_action;
    bool tree_altered;  
    bool caret_blink; 
    
    int caret_blink_count;

    void press_key(int key, int scan, int shift, int other); // Helper functions
    void up_key(int key, int scan, int shift, int other);
    void down_key(int key, int scan, int shift, int other);
  
    void process_mouse();    // Both called every frame by poll()
    void process_keyboard();
    void poll();
     
    BITMAP* get_cursor_bmp();
    void set_cursor_bmp(BITMAP* image);

  public:
    
    ~window_manager();
    window_manager();
  
    void set_console_font(FONT* f) { console_font = f; }
    void do_gui(base_window& iow, bool (*func)(window_manager&));
    void suspend();
    void resume();
     
    base_window* get_keyfocus() { return keyfocus; }
    base_window* get_target() { return target; }
    masked_image* get_cursor() { return cursor; }
      
    void draw(); 
    void purge(base_window* win);
    
    void clear_cursor_library();
    void load_cursor(cursor_name cur, string file);
    void set_cursor(BITMAP *image);
    void set_cursor(cursor_name cur =cursor_normal);    
    void set_tree_altered() { tree_altered = true; }
    void set_keyfocus(base_window* new_active);
  
    coord_int get_cursor_x() { return mouse_x; }
    coord_int get_cursor_y() { return mouse_y; }

    bool show_caret() { return caret_blink; }
    void set_caret(bool b) { caret_blink_count = 0; caret_blink = b; }
    
    void close_gui();

    struct poll_ei : public system_ei
    { DEFINE_EI(poll_ei, system_ei)
  
      const int frame;
  
      poll_ei(int f) : frame(f) { }
    };
    
};

void close_gui();

#endif
