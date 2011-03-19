#ifndef PLAYOUT_H                                                              
#define PLAYOUT_H

#include "pdefs.h"

// Forward declarations:
class base_window;
class flow_layout;
class layout_info;
   
/* The layout_manager class can be inherited to provide different mechanisms
 * for placing a window's children on its surface. To do this, the layout-
 * manager is 'attached' to a particular window using 'set_layout'. The 
 * layout-manager will then recalculate the window's children whenever 
 * the window is resized. 
 */ 
class layout_manager
{
  protected:

    bool packing;   // TRUE when this manager is currently being packed (allows for re-entrancy)
    bool repack;    // TRUE when this manager should repack itself again 
    bool conserve;  // TRUE if this manager should try to resize itself to accomodate its children
    
    base_window* container; // The window whose children we are managing
    
    // The following variables are simply provided for convenience, as they are
    // used by most layout policies in some way.
    coord_int x_margin; // Gap between each object horizontally. 
    coord_int y_margin; // Gap between each object vertically.

    // Override this to place all children according to whatever algorithm...
    virtual void pack() { }
    
  public:
  
    layout_manager(coord_int x, coord_int y) 
    : container(0), x_margin(x), y_margin(y), conserve(false), repack(false),
      packing(false)
    { }
    
    void set_conserve(bool c) { conserve = c; }
    bool get_conserve() { return conserve; }
    void suggest_size(coord_int w, coord_int h);
    
    void set_container(base_window* c); // Associate the given window as our manager
  
    void pack_layout();
  
    // Returns the first layout_info amongst our container's children.
    layout_info* get_first_layinfo() const;
};

/* Classes derived from 'layout_info' can be associated with a particular window
 * to represent information on how that window should be packed by its parent's
 * layout manager. The base layout_info class maintains the window's recommended
 * width and height, and also specifies whether it should EXPAND to fill the
 * width and height it has been allocated rather than merely align itself within.
 * 
 * For the most part, inherited windows will use const public data members. This
 * is because it is not safe to try to 'reclaim' type-information from a window's
 * 'layinfo' pointer once a layout_info object has been attached to it. Thus, it
 * is simpler to specifically configure the layout_info object while attaching 
 * it, and then forget about its type until it is destroyed 
 */
enum li_expand_type // Shortcut for supplying 2 bools
{
  li_expand_none = 0,
  li_expand_w = 1,
  li_expand_h = 2,
  li_expand_both = 3 // Because 1+2 = 3 means we can mask for these enums
};    

class layout_info
{
  public:
  
    layout_info(li_expand_type e=li_expand_none, compass_orientation c=c_centre,
                coord_int pw=0, coord_int ph=0)
    : master(0), ideal_w(0), ideal_h(0), pad_w(pw), pad_h(ph), 
      expand_x(e & li_expand_w), expand_y(e & li_expand_h), align(c)
    { }
  
    void set_master(base_window* m); // Associate this info object with a window
    
    // Member functions to set basic properties, repacking the tree if required
    void set_expand(li_expand_type e) { expand_x = e & li_expand_w; expand_y = e & li_expand_h; pack(); }
    void set_expand_w(bool e) { expand_x = e; pack(); }
    void set_expand_h(bool e) { expand_y = e; pack(); }
    void set_align(compass_orientation a = c_centre) { align = a; pack(); } 
    void set(li_expand_type e =li_expand_none, coord_int p_w =1, coord_int p_h =1, compass_orientation c =c_centre);
  
    // If someone tries to resize our master, this function is called instead.
    virtual void resize(coord_int w, coord_int h);
  
    // Function to place our master in the given area using expand/ideal rules
    void consume(const zone& area) const; 
    
    void pack() const; // Repack our master's sibling list if necessary
  
    // Member access functions:
    coord_int get_ideal_w() const { return ideal_w; }
    coord_int get_ideal_h() const { return ideal_h; } 
    coord_int get_pad_w() const { return pad_w; }
    coord_int get_pad_h() const { return pad_h; }
    bool get_expand_w() const { return expand_x; }
    bool get_expand_h() const { return expand_y; }
    compass_orientation get_align() const { return align; }
    
    coord_int get_w() const { return ideal_w + pad_w + pad_w; }
    coord_int get_h() const { return ideal_h + pad_h + pad_h; }
    
    // Return the next layout in our master's sibling list
    layout_info* get_next() const; 

    friend class layout_manager;
    friend class flow_layout;

  private:
  
    base_window* master; // The window which we are providing information for
    coord_int ideal_w; // Independant of its real width and height, simply the
    coord_int ideal_h; // width and height it would be if left alone
    coord_int pad_w;  // The amount we will seperate the edges of the window 
    coord_int pad_h; // from the area it will be given to occupy, the 'gap'    
    bool expand_x; // Set to true if the ideal dimensions should be ignored
    bool expand_y; // and the window should fill all the space given to it
    mutable bool hidden; // Set to true if the window is hidden due to lack of space 
    // If we use less space than we are given, this is how we fit within it:
    compass_orientation align; 
};

/* This class implements an algorithm where windows are placed from left to
 * right in the order of the sibling list. When there is no more space on this
 * row, the windows are then placed into the next 'row' on this window, where
 * the height of each row is determined by the heighest widget on that row. */
 
class flow_layout : public layout_manager
{
  public:

    void pack(); // Custom pack algorithm implemented here

    flow_layout(coord_int x =2, coord_int y =2, bool f=false, bool c=false)
    : layout_manager(x, y), fill(f), centre(c)
    { }
  
  private:
    
    bool fill;
    bool centre;
};

/* Grid-layout uses a system where the container's surface is divided up into
 * a series of 'cells'. The grid_layout class is told the number of cells it
 * should calculate, vertically and horizontally, and then can assign a window
 * to any of these cells through a custom layout-info object called 'grid_info'. 
 */
class grid_layout : public layout_manager
{
  public:

    void pack(); // Custom pack algorithm implemented here

    grid_layout(coord_int w =0, coord_int h =0, coord_int gw =2, coord_int gh =2)
    : layout_manager(gw, gh), grid_w(w), grid_h(h)
    { }

  private:

    int grid_w; // The number of cells across
    int grid_h; // The number of cells down
};

class grid_info : public layout_info // Used in conjunction with grid_layout
{
  private:
  
    int x_index;  // The column this window should be placed into
    int y_index;  // The row...
    int x_extend; // The number of columns it should occupy (1 default)
    int y_extend; // THe number of rows...

  public:

    // Assigned the location of the window intially, and its width and height
    // in cells. After a grid_info object has been created, it cannot be changed
    grid_info(coord_int x, coord_int y, coord_int xe =1, coord_int ye =1, 
              li_expand_type e=li_expand_both, compass_orientation c=c_centre,
              coord_int pw=1, coord_int ph=1)
    : layout_info(e, c, pw, ph), x_index(x), y_index(y), x_extend(xe), y_extend(ye)
    { }
    
    friend class grid_layout;
};

/* This is a rather complex algorithm that allows window to be placed within
 * the container in one of nine possible positions. These are represented by
 * the 8 points of the compass, as well as a 'centre' position. As children
 * are added, they are placed so that they fit into the remaining space, and
 * any number of windows can be added in the same 'direction' and they will
 * be placed next to each-other. */

class compass_layout : public layout_manager
{
  public:

    void pack();

    compass_layout(coord_int x=0, coord_int y=0) : layout_manager(x, y)
    { }
};

class compass_info : public layout_info
{
  public:
    compass_info(compass_orientation p, li_expand_type e=li_expand_both, 
                 compass_orientation c=c_centre, coord_int pw=1, coord_int ph=1)
    : layout_info(e, c, pw, ph), pos(p)
    { }
    
    friend class compass_layout;
    
  private:                      
    compass_orientation pos;
};
  
class gobbler_layout : public layout_manager
{
  public:
  
    void pack();

    gobbler_layout(coord_int x=0, coord_int y=0) : layout_manager(x, y)
    { }
};

class gobbler_info : public layout_info
{
  public:
        
    gobbler_info(compass_direction d, float b, li_expand_type e=li_expand_both, 
                 compass_orientation c=c_centre, coord_int pw=1, coord_int ph=1)
    : layout_info(e, c, pw, ph), bias(b), dir(d)
    { }
    
    float get_bias() { return bias; }
    compass_direction get_dir() { return dir; }
           
  private:
  
    float bias;
    compass_direction dir;
};
    
#endif LAYOUT_HPP
