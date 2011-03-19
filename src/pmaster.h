#ifndef PMASTER_H
#define PMASTER_H

#include "pbasewin.h"
#include "pgrx.h"

class window_master : public base_window
{
  private:  
      
    int display_delegation_depth; // Stores the number of delegation spans that are active 
    std::list<base_window*> display_delegation_list; // Stores list of currently delegated windows

    BITMAP* buffer; // The bitmap all children will draw to
    ptheme theme; // The theme all children will use
    
    int buffer_depth; // Colour-depth of the memory bitmap, 0 if a screen bitmap
  
  protected:
  
    // Overridden virtual functions:
    void pre_load();   // This allocates the bitmap and resets the theme
    void post_unload(); // This deallocates the bitmap                                                                                      
                                                                                                                          
  public:
  
    const ptheme& get_theme() { return theme; }
    BITMAP* get_buffer() { return buffer; }
  
    void begin_display_delegation(); // Called to start a display delegation span
    void end_display_delegation();   // Called to end a display delegation span
    void flush_display_delegation(); // Force display of all currently delegated windows
    
    void delegate(base_window* win); // Add a window to the delegation list
    bool should_delegate() { return (display_delegation_depth > 0); } // TRUE if delegation is in effect
    
    bool is_video() { return (buffer_depth == 0); } // True if the buffer is a video bitmap
   
    // Constructors, passed the desired colour-depth of the memory bitmap, or 0 to use the screen
    window_master(int depth =0);
    
    friend class base_window;
    friend class window_sub;
};

#endif MASTER_HPP
