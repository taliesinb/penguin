#ifndef PBASEWIN_H     
#define PBASEWIN_H

#include <bitset>     // For flags

#include "pdefs.h"    // General definitions
#include "pevent.h"   // Base_window inherits from event_participant
#include "puser.h"    // Kb_event class and key constants

#define DAZ_R_CHILDREN 1                              
#define DAZ_R_PREVIOUS 2
#define DAZ_R_PARENT   4
#define DAZ_O_RECURSE  16
#define DAZ_O_CULL     32
#define DAZ_O_OCCLUDE  64
#define DAZ_F_SPYSUB   128

#define W_DEBUG_MINIMAL             1
#define W_DEBUG_SHOW_VIS_ZONES      2
#define W_DEBUG_STEP_DISPLAY        4
#define W_DEBUG_LOG_DISPLAY         8
#define W_DEBUG_DRAW_ARB_ZONES      16
#define W_DEBUG_NO_DELEGATE_DISPLAY 32
#define W_DEBUG_NO_UPDATE_SUB       64
#define W_DEBUG_NO_DISPLAY          128
#define W_DEBUG_IGNORE_VISZONES     256
#define W_DEBUG_NO_DAZ_OPT          512
#define W_DEBUG_DRAW_Z_COUNT        1024
#define W_DEBUG_DRAW_WIN_ID         2048
#define W_DEBUG_DRAW_D_COUNT        4096
#define W_DEBUG_NO_YSORT            8192

struct FONT; 
extern FONT* tfont;

// Forward Declarations:
class window_sub;
class window_manager;
class window_master;
class ptheme;
class event_knot;
class debug_info;
class layout_manager;
class layout_info;
class graphics_context;

/* This is class upon which all more specialised windows are to be built, ie, buttons,
   frames, boxes, images, check-boxes, lists, text-boxes, subliminal windows, etc.
   It has default versions of all the functions that derived windows are expected to
   perform: movement, resizing, subliminal window spying, vis_zone determination, z-
   position reshuffling, etc. Most important functions are also virtual and can thus
   be over-written by derived classes, including several callbacks pertaining to
   movement and resizing and other actions, allowing for flexibility within certain
   functions that would be tedious and dangerous to rewrite in each derived class.
*/

class base_window : public event_participant
{
  private:
 
    // Variables to-do with user-event handling
    coord_int click_x;
    coord_int click_y;
    coord_int mouse_x;
    coord_int mouse_y;         
    bt_int button_state;
         
  protected: 
  
    // Virtual callbacks for user-event handling
    virtual void event_mouse_down(bt_int button) { }
    virtual void event_mouse_up(bt_int button) { }
    virtual void event_mouse_on() { }
    virtual void event_mouse_off() { }
    virtual void event_mouse_move(coord_int x, coord_int y) { }
    virtual void event_mouse_drag(coord_int x, coord_int y) { }
    virtual void event_mouse_hold(int t) { }
  
    virtual void event_key_focus() { }
    virtual void event_key_unfocus() { }
    virtual void event_key_down(kb_event kb) { }
    virtual void event_key_up(kb_event kb) { }
    virtual void event_key_blink() { }
  
  public: 

    // Public access functions for user-event handling.
    coord_int get_mouse_x() const { return mouse_x; }
    coord_int get_mouse_y() const { return mouse_y; }
    coord_int get_click_x() const { return click_x; }
    coord_int get_click_y() const { return click_y; }
    bt_int get_button_state() const { return button_state; }

    void set_cursor(cursor_name cur);
    void set_keyfocus();
           
  public:
  
    enum private_flags // Flag system:
    {
      sys_dynamic = 0,
      sys_active,
      sys_loaded,  
      sys_loading,
      sys_delegated,  
      sys_first_load,   
      grx_sensitive,  
      vis_negative_clip,
      vis_positive_clip,
      vis_complete_clip,
      vis_visible,      
      vis_should_be_visible,    
      evt_dragged,
      evt_mouse_over,
      evt_keyfocus,       
      _last_private_flag // Remeber index of last flag in private_flags
    };
    enum protected_flags
    {
      vis_invisible = _last_private_flag,  // protected_flags starts off where private_flags ended
      sys_always_resize, 
      grx_linear_display,
      grx_object_display,
      grx_master,
      grx_subliminal,
      evt_snoop_keys,
      evt_snoop_clicks,
      _last_protected_flag // Remember index of last flag in protected_flags
    };
    enum public_flags
    {
      sys_z_fixed = _last_protected_flag, // public_flags starts off where protected_flags ended
      vis_ignore_estate,
      sys_auto_w,
      sys_auto_h,
      evt_disabled,
      evt_dialogue,
      _last_public_flag // Remember total number of flags
    };
    
    bool flag(public_flags f) const {return flags[f]; } 	  // Everyone can get public,
    bool flag(protected_flags f) const{ return flags[f]; }	// and private,
    bool flag(private_flags f) const { return flags[f]; } 	// and protected flags
    void set_flag(public_flags f, bool b=true) { flags.set(f, b); } 	// Anyone can set public flags 
    void set_flag_cascade(public_flags f, bool b =true);

  private:

    base_window* next;        // Point to our immediately superior sibling
    base_window* prev;        // Point to our immediately inferior sibling
    base_window* parent;      // Point to our containing parent
    base_window* child;       // Point to our first contained window
    window_sub* next_sub;     // Points to the next subliminal window in our sub_chain.
    window_master* master;    // Points to the buffered window that contains us (if any)
    window_manager* manager;
    layout_manager* layout;
    layout_info* layinfo;
    zone* vis_list; // LL of zones rep. screen portions to be drawn to on a full display.
    
    // Size: 40 bytes
 
    coord_int ax; // Logical co-ordinates: relative to parent, don't need to be updated
    coord_int ay; // during a move...
    coord_int bx;
    coord_int by;
  
    coord_int cx; // Physical co-ordinates: relative to screen, are updated by
    coord_int cy; // update_coords() on movement and resizing.
    coord_int dx;
    coord_int dy;
   
    coord_int c_cx; // Clipped physical co-ordinates: physical co-ordinates that do
    coord_int c_cy; // not extend beyond the limits of our parent. If we are completely
    coord_int c_dx; // off-bounds, these will all be set to -1.
    coord_int c_dy;
    
    bitset<_last_public_flag> flags; // The actual bitset, using _last_public_flag to find total no of flags
    
    // Only private members can set protected flags
    void set_flag(private_flags f, bool b=true) { flags.set(f, b); }	 
    void set_flag_cascade(private_flags f, bool b =true);

    void set_master(window_master* n);
    void set_manager(window_manager* m);
    void set_next_sub_behind(window_sub* _sub);
    void set_next_sub(window_sub* _sub);
    
    void hide_helper(); // Helpers used by hide() and show() - they save and
    void show_helper(); // restore the visibility flags

    unsigned short win_id; // Used for debugging purposes
    static int new_id;     // Used to generate a new id for each window
      
    // Updates vis_zones of all windows behind this window in the sibling list.
    void update_vislist_behind(); 
    void update_family_vislist();
                          
    void extract(); // Lowlevel function to remove the window from the tree
              
    // Called by various interface functions to set new co-ordinates and redisplay as necessary.
    void move_resize(coord_int _ax, coord_int _ay, coord_int _bx, coord_int _by);
    
    // Displays all zones in the vis_list to surface. Does not handle sub-spying.
    void _display();
 
    void load(); // Loading-related functinos:
    void unload();
    void pre_load_all();
    void post_load_all();
    void pre_unload_all();
    void post_unload_all();
   
    base_window(const base_window&)
    { throw illegal_operation_exception(); }

    // Virtual function to draw this window.
    virtual void draw(const graphics_context& grx) { }  

  protected:

    // Add the given window as our child, initializing it if necessary
    // and assigning it the given layout-info object
    void add_child(base_window* new_child, layout_info* lm =0, bool front =true);
    void add_child(base_window& new_child, layout_info* lm =0, bool front =true)
    { add_child(&new_child, lm, front); } // Shortcut
    
    // Another form of 'add_child'
    void add_sibling(base_window& sibling, layout_info* lm =0, bool front =true)
    { if (parent) parent->add_child(&sibling, lm, front); }
                         
    void set_flag(protected_flags f, bool b=true) { flags.set(f, b); } 
    
    /* Sets the given flag to the given value, in addittion to every single such
     * flag in our descendants, to the given value. 
     */
    void set_flag_cascade(protected_flags f, bool b =true);
  
    const zone* get_vis_list() const { return vis_list; }
    
    const ptheme& theme() const; //Returns a reference to our theme.
    int get_color(int r, int g, int b) const; // Tries to construct the colour out of r,g,b.
    int get_depth() const; // Tries to return the colour-depth the current surface is at
    
    void update_estate();                
    
    // Make sure all co-ordinates are valid (only really useful for derived classes)
    void clip_coords();
    void update_coords();
  
    // Hooks, helpers, and such
    virtual void pre_load() { }
    virtual void post_load() { }
    virtual void pre_unload() { }
    virtual void post_unload() { }
    virtual void position_children() { }
    virtual void move_resize_hook(const zone& old_pos, bool was_moved, bool was_resized) { }                                    
  
    // Returns the first relevant subliminal window within reach, NULL if none.
    window_sub* find_next_sub();
  
    // Specifically handles sub-spying and updates sub-windows where necessary within
    // the particular area, usually "clipped()". Pass 0 to use this automatically.
    void inform_sub(const zone* area);      
    void inform_sub_family(const zone* area);
    
    // Draws us to sub's sub-buffer, using 'list'. If 'update', calls 'sub_buffer_updated'
    void draw_to_sub(window_sub* sub, const zone* list, bool update);
    
    // Updates our vis_zones
    void update_vislist();
   
    // Given a zone-list and flags, gradually move up and back the tree, finding
    // and displaying any portions of windows that intersect with the given zone
    // list. Will handle sub_spying of any and all displayed windows as requested.
    void draw_arb_zones(zone*& arb_list, unsigned char arb_flags);
  
    // Return value based on how a particular point intersects this window
    // FALSE = out of range or subliminal, TRUE = spot on!
    virtual bool pos_visible(coord_int x, coord_int y) const;
  
    // Recursively searches for the window which lies under a particular point,
    base_window* find_window_under(coord_int x, coord_int y);
  
  public:

    // Read-only retrieval of window co-ordinates and other information
    coord_int get_ax() const { return ax; }
    coord_int get_ay() const { return ay; }
    coord_int get_bx() const { return bx; }
    coord_int get_by() const { return by; }
    
    coord_int get_cx() const { return cx; }
    coord_int get_cy() const { return cy; }
    coord_int get_dx() const { return dx; }
    coord_int get_dy() const { return dy; }
    
    coord_int get_ccx() const { return c_cx; }
    coord_int get_ccy() const { return c_cy; }
    coord_int get_cdx() const { return c_dx; }
    coord_int get_cdy() const { return c_dy; }
    
    coord_int w() const { return bx - ax; }
    coord_int h() const { return by - ay; }
    
    // These functions try to generate phyiscal co-ordinates that are relative
    // to this window, from the given window 'w'. 'w' must be our descendant
    coord_int local_ax(base_window& w) const;
    coord_int local_ay(base_window& w) const;
    coord_int local_bx(base_window& w) const;
    coord_int local_by(base_window& w) const;

    // Should be overridden by widgets, etc, to provide sensible defaults:
    virtual coord_int normal_w() const { return 0; }
    virtual coord_int normal_h() const { return 0; }
 
    bool disabled(); // Returns TRUE if this window is forbidden to receive user events
 
    int get_z_count() const; // Returns position of this window in the sibling list
    int get_win_id() const { return win_id; } 

    // Tree traversal functions. Should give these a good grilling, to make sure
    // they deal with masters sensibly
    base_window* superior();
    base_window* most_prev();
    base_window* most_next();
    base_window* next_or_uncle();
    base_window* oldest_child(); 

    // This function will return a zone-list of all areas that should be drawn for this
    // window, taking into account obscuring windows upto but not including (stop_window)
    // itself, starting with the given (draw_list) if any is provided. Should be passed
    // a clipped draw_list, ie, "d_clipped(this)". Passing nothing uses that automatically.
    zone* create_occluded_drawlist(base_window* stop_window =0, zone* draw_list =0);

    static int debug; // Flags controlling visually-displayed diagnostic information
    static int count; // A count of all windows constructed on the heap and stack.
  
    // Virtual destructor
    virtual ~base_window();
  
    // Window constructor. Uses default values for most things, expects them to
    // be properly set by the user later on.
    base_window();
      
    void set_layout(layout_manager* l); // Delete old, bind new layout manager to us
    layout_manager* get_layout() const { return layout; } // Return pointer to our layout manager 
    layout_info* get_layinfo() const { return layinfo; } // Returns pointer to our layout-info object
    layout_info& lay_info() const { return *layinfo; }
    layout_manager& lay_man() const { return *layout; }
    void set_layinfo(layout_info* li); // Deletes the old and attaches a new layout-info object to us
    void pack(); // Attempts to repack our parents, if all is good and well, and necessary
    bool is_laid_out(); // Returns true if our siblings are being layout-managed, and we have a layinfo object
  
    /* These marvelous functions provide a way for a class to specify exactly where its
    'estate' lies that its children can occupy. ex, ey specify the start of the
    estate on its surface, and ew, eh describe how large the estate is. These
    functions should return consistent values. */
       
    virtual coord_int e_ax() const { return 0; } // Beginning of estate
    virtual coord_int e_ay() const { return 0; }
    virtual coord_int e_bx() const { return w(); } // End of estate
    virtual coord_int e_by() const { return h(); }
    
    coord_int e_w() const { return e_bx() - e_ax(); } // Returns the width of the estate 
    coord_int e_h() const { return e_by() - e_ay(); } // Returns the height of the estate
    
    // Returns zone of our clipped co-ordinates. Returns -1,-1,-1,-1 if fully clipped.
    zone clipped() const
    { return zone(c_cx, c_cy, c_dx, c_dy); }
  
    // Return a pointer to a zone of our clipped physical co-ords. Returns NULL
    // if we are fully clipped.
    zone* d_clipped() const
    { return flag(vis_complete_clip) ? 0 : new zone(c_cx, c_cy, c_dx, c_dy); }
  
    void hide();   // Make this window and its children invisible
    void show();   // Make this window, et al, visible
    void remove(); // Remove this window from the window tree, and unload it.
  
    /* Change the z-order of this window within its sibling list. The enums 
     * provide shortcuts for common values 
     */
    enum z_shift_values
    {
      z_front = 65535,
      z_back = -65535,
      z_raise = 1,
      z_lower = -1,
    };       
    
    /* Change the order of this window in the sibling list, moving it forward
       by the given number. If 'z' is negative, the window is moved backwards */                          
    void z_shift(int z =z_front); 
  
  
    /* These various functions are used to change certain of the window co-ordinates. They
       propogate those changes down to their children, recalculate vis_zones, and update
       their own internal screen co-ordinates and width and height values, as well as
       redisplaying themselves, and "filling the gap" made by any movement taking place.
    */
    static const coord_int normal_size = -32768;
    
    void move(coord_int _ax, coord_int _ay);
    void relative_move(coord_int x, coord_int y);
    void resize(coord_int width =normal_size, coord_int height =normal_size);
    void resize(coord_int x, coord_int y, coord_int width, coord_int height);
    void set_w(coord_int width =normal_size);
    void set_h(coord_int height =normal_size);
    void move(coord_int _ax, coord_int _ay, coord_int _bx, coord_int _by);
    void place(coord_int _ax, coord_int _ay, coord_int _bx, coord_int _by);
    void place(compass_orientation c, coord_int side =0);
                                                                                                     
    // All these functions are related to display. All of them will automatically handle
    // subliminal-spying as well.
    void display(); // Display all zones in the vis_list to surface
    void display_all(); 
  
    // Prints information on this window tree to the given ostream in a console-format.
    void print(ostream& str = cerr);
  
    // Special callback that is applied to every window in this tree. If the callback
    // at any point returns a non-zero integer, the recursion will stop and the value
    // will be returned by this function.
    int for_all(int(func)(base_window*));
   
    // Returns the various tree-pointers of this window. 
    window_master* get_master() { return master; }
    window_sub* get_next_sub()  { return next_sub; }
    base_window* get_child()    { return child; }
    base_window* get_parent()   { return parent; }
    base_window* get_next()     { return next; }
    base_window* get_prev()     { return prev; }
    window_sub* get_next_external_sub();
    window_manager* get_manager() { return manager; } 
        
    base_window* find(int _id);
  
    // Returns true if the given window lies in our family.
    bool ancestor_of(base_window* orphan) const;
    
    // Returns true if we might be visible.
    bool visible() { return !flag(vis_complete_clip) && flag(vis_visible); }
          
    void delegate_displays();   // Begins display delegation, if appropriate
    void undelegate_displays(); // Ends display delegation
    
    // Attempts to redraw windows to fill in the given gap, starting from 'mid'
    // and going backwards. If mid is 0, then start at our oldest sibling.
    void display_gap(zone*& gap_list, base_window* mid =0);
  
    // Returns the type-name of this window. Uses RTTI, but fixes some dodgy numbers
    // that seem to happen in the DJGPP version of RTTI.
    const char* type_name() const;                                                        
  
    unsigned short display_count;
  
    // Overrides event_participant stub
    base_window& window() { return *this; }
  
    friend ostream& operator<<(ostream&, base_window&);

    friend bool console_command();
    friend class window_manager;
    friend class window_master;
    friend class layout_info;
    friend class window_sub;
    friend class drag_helper;    
};

// Event-info type definitions:

struct system_ei : public event_info
{ DEFINE_EI(system_ei, event_info) };

struct display_ei : public system_ei
{ DEFINE_EI(display_ei, system_ei) };
 
struct user_ei : public event_info
{ DEFINE_EI(user_ei, event_info) };

struct mouse_ei : public user_ei
{ DEFINE_EI(mouse_ei, user_ei) 

  coord_int x;
  coord_int y;
  bt_int buttons;

  mouse_ei(coord_int xx, coord_int yy, bt_int bb) 
  : x(xx), y(yy), buttons(bb) { }

  mouse_ei() : x(-1), y(-1), buttons(0) { }
};    

struct mouse_down_ei : public mouse_ei
{ DEFINE_EI(mouse_down_ei, mouse_ei)    

  mouse_down_ei(coord_int xx, coord_int yy, bt_int bb) 
  : mouse_ei(xx,yy,bb) { }
};

struct mouse_up_ei : public mouse_ei
{ DEFINE_EI(mouse_up_ei, mouse_ei) 

  coord_int click_x; // Position of mouse when mouse_down
  coord_int click_y;

  mouse_up_ei(coord_int xx, coord_int yy, bt_int bb, coord_int cx, coord_int cy) 
  : mouse_ei(xx,yy,bb), click_x(cx), click_y(cy) { }
};

struct mouse_move_ei : public mouse_ei
{ DEFINE_EI(mouse_move_ei, mouse_ei)              

  coord_int x_move; // Distance mouse moved
  coord_int y_move;    

  mouse_move_ei(coord_int xx, coord_int yy, coord_int xm, coord_int ym) 
  : mouse_ei(xx, yy, 0), x_move(xm), y_move(ym) { }
};

struct mouse_hold_ei : public mouse_ei
{ DEFINE_EI(mouse_hold_ei, mouse_ei)              

  int hold_time; // Number of frames button has been held down                             

  mouse_hold_ei(coord_int xx, coord_int yy, bt_int bb, int t) 
  : mouse_ei(xx, yy, bb), hold_time(t) { }
};

struct mouse_drag_ei : public mouse_ei
{ DEFINE_EI(mouse_drag_ei, mouse_ei)              

  coord_int x_move; // Distance mouse dragged
  coord_int y_move;

  mouse_drag_ei(coord_int xx, coord_int yy, bt_int bb, coord_int xm, coord_int ym)
  : mouse_ei(xx, yy, bb), x_move(xm), y_move(ym) { }
};

struct mouse_focus_ei : public mouse_ei
{ DEFINE_EI(mouse_focus_ei, mouse_ei) 

  bool focused;

  mouse_focus_ei(bool f) : focused(f) { }
};

struct mouse_on_ei : public mouse_focus_ei
{ DEFINE_EI(mouse_on_ei, mouse_focus_ei) 

  mouse_on_ei() : mouse_focus_ei(true) { }
};

struct mouse_off_ei : public mouse_focus_ei
{ DEFINE_EI(mouse_off_ei, mouse_focus_ei) 

  mouse_off_ei() : mouse_focus_ei(false) { }
};

struct key_ei : public user_ei
{ DEFINE_EI(key_ei, user_ei) 

  kb_event key; 

  key_ei(kb_event k) : key(k) { }
};

struct key_down_ei : public key_ei
{ DEFINE_EI(key_down_ei, key_ei)                  

  key_down_ei(kb_event k) : key_ei(k) { }
};

struct key_up_ei : public key_ei
{ DEFINE_EI(key_up_ei, key_ei)                    

  key_up_ei(kb_event k) : key_ei(k) { }
};
    
struct key_focus_ei : public system_ei
{ DEFINE_EI(key_focus_ei, system_ei) };

struct key_unfocus_ei : public system_ei
{ DEFINE_EI(key_unfocus_ei, system_ei) };

struct load_ei : public system_ei
{ DEFINE_EI(load_ei, system_ei) };

struct unload_ei : public system_ei
{ DEFINE_EI(unload_ei, system_ei) };

struct move_resize_ei : public system_ei
{ DEFINE_EI(move_resize_ei, system_ei)          

  const zone& old_pos;
  bool moved;
  bool resized;                           

  move_resize_ei(const zone& p, bool m, bool r) 
  : old_pos(p), moved(m), resized(r) { }
};

/* Class to make the handling of mouse-clicking and dragging easier. We delegate
 * events to it, and it moves us on our behalf. It is passed the minimum width
 * and height we want for its constructor.
 */
class drag_helper
{
  private:
  
    compass_orientation drag_mode; // Used internally to keep track of how its being dragged
    coord_int min_w;
    coord_int min_h;
  
  public:
  
    void down(base_window* win);
    void up(base_window* win);
    void drag(base_window* win, coord_int x, coord_int y);   
    
    drag_helper(coord_int w =16, coord_int h =16)
    : min_w(w), min_h(h)
    { }
};  

// Returns true if the two given windows intersect on the physical screen:
bool window_intersect(base_window*, base_window*);

#endif
