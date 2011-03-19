#include "pbasewin.h"

#include <typeinfo>   // To allow for RTTI

#include "allegro.h"  
#include "pmanager.h" 
#include "playout.h"  
#include "psublim.h"
#include "putil.h"

/* A lot of people object to macros that are used heavily to save key-strokes
   as they can be hard to interpret for external programmers. In this case, I
   think the opposite is true, because the process of 'looping through children'
   can be immediately recognised in the case of the macro, which in the long
   form would much longer to see and digest as a simple loop. Most importantly,
   it prevents simple bugs along the lines of 
  
     for (base_window* loop = child; child; loop = loop->next)
  
   and such similar mistakes, which I have made many a time. */
 
#define LOOP_CHILDREN(a) for (base_window* a = child; a; a = a->next)

/* The mechanism to give each window a unique number needs a static member to
   know which id to give next. */
int base_window::new_id = 0; 

/* Low-level debugging flags are set here. */
int base_window::debug = 0;
                                                                        
/* This is incremented whenever a window is allocated, decremented whenever a
   window is deleted to catch memory-leaks.*/
int base_window::count = 0;

base_window::base_window() // Blank constructor.
: next(0), prev(0), parent(0), child(0), next_sub(0), master(0), manager(0),
  layout(0), layinfo(0), vis_list(0) // NULL all variables
{
  ax = ay = bx = by = cx = cy = dx = dy = c_cx = c_cy = c_dx = c_dy = 0;
  click_x = click_y = mouse_x = mouse_y = -1; 
  button_state = 0;
  
  set_flag(vis_visible);
  set_flag(grx_sensitive);
  set_flag(sys_first_load);
  
  win_id = base_window::new_id++; // Assign window a unique ID
  count++; // Increment the window count. Will be decremented on destruction
}

base_window::~base_window() // Destructor
{
  // Destroy any associated layout and layout_info objects, these are heap-based
  delete layinfo; 
  delete layout; 
  
  count--; // Decrement the window count
}

// Return the ptheme in use (from manager)
const ptheme& base_window::theme() const
{
  return manager->get_theme();
}

// Find the next sub in our chain that is not within our family
window_sub* base_window::get_next_external_sub()
{
  window_sub* sub = next_sub; 
  
  while (sub && ancestor_of(sub)) sub = sub->next_sub;
  
  return sub;
}

// Find the first window that is 'above' this window, 0 if none
base_window* base_window::superior() 
{
  if (child)  return child; // Return child if any
  if (next)   return next;  // otherwise, behave like 'next_or_uncle()'
  if (parent) return parent->next_or_uncle();
  return 0;
}

// Return the window at the began of the sibling list
base_window* base_window::most_prev()
{
  base_window* cur = this;

  while (cur->prev) cur = cur->prev;

  return cur;
}

// Return the window at the end of the sibling list
base_window* base_window::most_next()
{
  base_window* cur = this;
  
  while (cur->next) cur = cur->next;

  return cur;
}

// Recursively look for a window with a particular ID
base_window* base_window::find(int id)
{
  if (win_id == id) return this; // If we're the window, return ourselves

  base_window* temp; // Otherwise, pass the buck to our child/next window
  if (child && (temp = child->find(id))) return temp; // and return their results
  if (next && (temp = next->find(id))) return temp;
  return 0;
}

/* Used to find occluding windows. Returns the next window, if none, then our
 * parent's next. */
base_window* base_window::next_or_uncle()
{
  if (next)   return next;
  if (parent) return parent->next_or_uncle();
  
  return 0;
}

// Return the window at the end of our child's sibling list, 0 if no child
base_window* base_window::oldest_child()
{
  if (child)
  {
    base_window* cur = child;
    
    while (cur->next) cur = cur->next;
    
    return cur;
  } else return 0;
}

bool base_window::disabled()
{
  for (base_window* loop = this; loop; loop = loop->get_parent())
  {
    if (loop->flag(evt_dialogue)) return false;
    if (loop->flag(evt_disabled)) return true;
  }
  return false;
}

// Returns true if the given window exists within our descendants
bool base_window::ancestor_of(base_window* orphan) const
{
  if (this == orphan) return true;
  if (!child || !orphan) return false;

  // If we're not the orphan and we have children, ask them
  LOOP_CHILDREN(loop) if (loop->ancestor_of(orphan)) return true;

  return false; 
}

// Short-cuts to begin display delegation
void base_window::delegate_displays()
{
  if (master) master->begin_display_delegation();
}

// And end display delegation
void base_window::undelegate_displays()
{
  if (master) master->end_display_delegation();
}

/* This very clever function attempts to change this window's position in the
 * sibling list by the given number of steps forward. It goes backwards if a
 * negative number is used. It will work on a loaded or unloaded window, and
 * updates all tree-pointers as necessary, also rerenders sub-buffers where
 * required. */
 
void base_window::z_shift(int z)
{
  if (z == 0 || !parent) return; // Exit if there's no sibling list or work to do
  
  /* 'before' stores the window that we will eventually be inserted behind, 
     'after' stores the window that we will be inserted after, and 'under' is
     our current parent (this won't be changed, its just stored temporarily). */
  base_window* before = next, *after = prev, *under = parent;  
                            
  /* This is used to determine the point from which we will display backwards,
     when being shifted by a negative amount. */
  base_window* last = prev; 
  
  // Our clipped visible area, if any.
  zone* this_win = d_clipped(); 
  
  manager->set_tree_altered();
  
  extract(); // Remove ourselves from the tree, nulling all pointers

  if (z > 0) // We are going forwards?
  {    
  /* This loop goes forwards, counting the number of steps taken. It will stop
     when 'z' is reached, or when we reach the end of the sibling list. Each
     'step' increments the 'before' and 'after' pointers, keeping our bearings
     on where we should eventually go. The loop also updates vis-lists et al
     as necessary */
       
    for (int index = 0; index < z && before && !before->flag(sys_z_fixed); index++)
    {     
      after = before; 
      before = before->next; // Shift along our 'frame of reference'  
      
      // If we have just passed a window and they might be effected...
      if (after && after->visible() && visible() && window_intersect(after, this))
      {
        after->update_family_vislist(); // Update their vis-list
          
      /* BUT, because we aren't yet in the tree, their vis-list will not
         be occluded by our area. So we have to step through all vis-lists
         in their family and manually occlude ourselves */ 
        
        base_window* stop = before ? before : under->next_or_uncle();
        for (base_window* loop = after; loop != stop; loop = loop->superior())
          occlude(loop->vis_list, this_win);
          
      /* After we've done that, we check whether they are a subliminal window.
         If they are, then we have just removed ourselves from their point of
         view, and thus their sub-buffer needs to be updated. */        
         
        if (window_sub* qualified = dynamic_cast<window_sub*>(after))
        {
          qualified->update_sub();
          qualified->display();      
        } 
      }
    }
  
  } else // Otherwise, if we're going backwards
  {
    // Loop backwards, for z windows, or until we reach the beginning
    for (int index = 0; index > z && after && !after->flag(sys_z_fixed); index--)
    {
      // Simply recalculate the passed-window's vis-list, if need be
      if (visible() && after->visible() && window_intersect(after, this)) 
        after->update_family_vislist();                   
          
      before = before ? before->prev : after;
      after = after->prev;
    }
  }                                                      
       
  // Here, we insert this window at the frame-of-reference calculated above     
  parent = under;     
  if (before) { before->prev = this; next = before; }
  if (after) { after->next = this; prev = after; } else parent->child = this;
      
  // Next, we set all the more complicated tree-pointers  
  if (window_manager* qualified = dynamic_cast<window_manager*>(parent)) set_manager(qualified);
  else set_manager(parent->manager);

  set_master(parent->flag(grx_master) ? (dynamic_cast<window_master*>(parent)) : parent->master);

  set_next_sub(find_next_sub());

  // If we are a subliminal window, our sub-buffer obviously needs updating
  if (window_sub* qualified = dynamic_cast<window_sub*>(this)) 
  {
    qualified->update_sub();
    set_next_sub_behind(qualified);
  } 

  update_coords(); // Not really necessarry
  update_family_vislist(); 

  delegate_displays();
  
  // Pack our parent, as a lot of layout algorithms depend on z-order
  parent->pack(); 
   
  // These are the 'clean-up' operations, only necessarry if we're visible
  if (visible()) 
  {
    if (z > 0)
    {
      // For a forward-shift, just redisplaying the window is required      
      display_all();
      
    } else
    { 
    /* For a backward-shift, we need to update the sub-buffers of all subliminal 
       windows we moved past. We also need to redisplay ourselves if we are 
       subliminal (because our sub-buffer will have changed). Then we should
       redisplay all windows touching our area that we moved behind (filling the
       gap that is left by our dissappearance)! */      
      
      inform_sub_family(this_win);    
      
      if (flag(grx_subliminal)) display();      

      if (last) // Update the gap
      {
        set_flag_cascade(grx_sensitive, false); // Make sure we don't get displayed
        last->draw_arb_zones(this_win, DAZ_R_CHILDREN+DAZ_R_PREVIOUS+DAZ_F_SPYSUB);
        set_flag_cascade(grx_sensitive, true);
      }        
    }
  }
  
  delete this_win;
  
  undelegate_displays();
} 

/* Helper function to remove a window from the window tree. It doesn't redisplay
 * or unload, it is meant to be driven by other functions */
void base_window::extract()
{
  set_next_sub_behind(find_next_sub());
  if (parent && parent->child == this) parent->child = next;
  if (prev) prev->next = next; //  'Stitch' the tree pointers around us
  if (next) next->prev = prev;
  set_master(0);    // Null all our pointers:
  set_manager(0);
  set_next_sub(0);
  parent = 0;
  next = 0;
  prev = 0;
}

/* This function is designed specifically to remove and unload a window from
 * the window tree. It will unload the family & redisplay any exposed window */
void base_window::remove()
{ 
  if (!parent) return;
    
  base_window* old_parent = get_parent();
  base_window* old_next = get_next();
  base_window* old_prev = get_prev();
  zone* gap = d_clipped();
  
  extract(); // Remove our family from the window tree
  if (manager) manager->purge(this);  
  
  if (flag(sys_active))
  {
    unload(); // Unload our entire family 
    if (visible())
    {
      if (old_prev) old_prev->update_vislist_behind(); 
      else old_parent->update_vislist();
      old_parent->display_gap(gap, old_next); 
    }
  }
  
  delete_zonelist(gap); 
}

/* Function to make a particular window and its children invisible. It uses a
 * special technique to 'save' the state of visibility so that when this portion
 * of the tree is redisplayed, any sub-tree that were originally hidden will 
 * continue to be */
void base_window::hide()
{
  set_flag(vis_should_be_visible, false); // Indicate we are technically invisible

  if (flag(vis_visible)) // But only hide() if we're physically visible
  { 
    hide_helper(); // Save the 'visible' state of our children where necessary
   
    set_flag_cascade(vis_visible, false); // Unset the 'vis_visible' flags of our family
    
    if (flag(sys_active))
    {
      update_vislist_behind(); // Recalculate the vis-zones of windows under us
    
      zone* gap = d_clipped();
      get_parent()->display_gap(gap, this); // Display the gap that results from our dissappearance
      delete_zonelist(gap);
    }
  }
}

// Used by 'hide' to remember the state of any hidden windows 
void base_window::hide_helper()
{
  // Here, we save the state if our children only if we are visible. If we are invisible, then
  // the state of our children has already been saved, and should not be over-written
  if (flag(vis_visible))   
  {
    LOOP_CHILDREN(loop) // Save the state of our children
    {
      loop->set_flag(vis_should_be_visible, loop->flag(vis_visible)); 
      loop->hide_helper();
    }
  }
}  

/* Function to reveal a previously-hidden window and its family. It will 
 * redisplay as necessarry, recalculate any sub-buffers that exist within 
 * the family, and restore the previous state of visibility. */
void base_window::show()
{
  if (flag(vis_invisible)) return;

  set_flag(vis_should_be_visible, true); // Indicate we are technically visible

  // But only show() if we're physically invisible, and our parent is visible
  if (!flag(vis_visible) && (!get_parent() || get_parent()->flag(vis_visible))) 
  { 
    show_helper(); // Restore the original states of the 'vis_visible' flags in our family 
    
    if (flag(sys_active))
    {    
      update_vislist_behind(); // Recalculate OUR vis_zones and those of windows below us    
      display_all(); // Redisplay us and all our visible children
    }
  }
}

// Used by 'show' to restore the state of the revealed windows.
void base_window::show_helper()
{  
  set_flag(vis_visible, flag(vis_should_be_visible)); // Restore our original visibility
  
  if (window_sub* qualified = dynamic_cast<window_sub*>(this))
  {
    qualified->update_sub(); // If we're sub, time to recalculate our sub_buffer
  }
  
  // If we are technically visible, go on restoring our childrens visibility...
  if (flag(vis_visible)) 
  {
    LOOP_CHILDREN(loop) loop->show_helper(); 
    
  // If we are technically invisible, make all our children physically invisible        
  } else set_flag_cascade(vis_visible, false); 
}  

/* This work-horse function adjusts the logical co-ordinates of this window to
 * result in a new position and/or size. If the window is inactive, then no
 * other calculations need to be performed, but if it is active, it will:
 * 1) Set the new co-ordinates
 * 2) Update our co-ordinates and vis_list
 * 3) Call the move/resize hook, and transmit an event
 * 4) Repack and call 'position_children()' on this window if necessary
 * 5) Calculate the 'gap' that was left by our move/resize and fill it
 * 6) Redisplay us and our family
 */
void base_window::move_resize(coord_int _ax, coord_int _ay, coord_int _bx, coord_int _by)
{
  if (!flag(sys_active)) // If we are not active, then just update the co-ords
  {
    ax = _ax;
    ay = _ay;
    bx = _bx;
    by = _by;
    update_coords();
    
    return;
  }

  zone* gap_list = d_clipped(); // A zone of our original visible area

  zone old_pos(ax, ay, bx, by); // Remember the old co-ordinates

  manager->set_tree_altered(); // Inform the manager that a window in the tree has been moved

  ax = _ax;
  ay = _ay; // Set the new positions
  bx = _bx;
  by = _by;

  if (bx < ax) bx = ax; // Prevent terrible mayhem by disallowing negative
  if (by < ay) by = ay; // width and height, in case this happens

  update_coords(); // Update our clipped coords, our children's, etc.

  if (flag(vis_visible))
    update_vislist_behind(); // Update vis_lists of inferior windows, ourselves, and our children

  // Make sure any affected windows are only displayed once by using delegation
  delegate_displays();
  {
    // Determine whether we were moved and/or resized
    bool was_moved = (ax != old_pos.ax || ay != old_pos.ay);
    bool was_resized = (w() != old_pos.w() || h() != old_pos.h());

    if (was_moved || was_resized || flag(sys_always_resize)) // If so, call the hook & transmit the event
    {
      move_resize_hook(old_pos, was_moved, was_resized);
      transmit(move_resize_ei(old_pos, was_moved, was_resized)); 
    }
  
    // Find the difference between the new area and the old area (the 'gap')
    if (gap_list && flag(vis_visible)) occlude(gap_list, &clipped());
  
    // If we were resized...
    if (was_resized)
    {
      set_flag_cascade(grx_sensitive, false); // Make children don't get drawn
      pack(); // Ask our layout-manager, if any, to reposition our children
      position_children(); // Allow any client code to reposition children manually
      set_flag_cascade(grx_sensitive, true); 
    } 
   
    // If we have a gap to fill, fill it, and redisplay ourselves
    if (flag(grx_sensitive) && flag(vis_visible) && (was_moved || was_resized || 
        flag(sys_always_resize)))
    {
      if (gap_list) get_parent()->display_gap(gap_list, this);
      
      display_all(); // Display all windows in our family
    } 
    
    delete_zonelist(gap_list);
  }
  undelegate_displays();
}

/* This is a helper function to 'refill' a gap that has been exposed by any 
 * number of operations. It finds the relevant window to which it should apply
 * a 'draw_arb_zones' operation, and applies it. The 'gap_list' it is passed may
 * be modified by the process 
 */ 
void base_window::display_gap(zone*& gap_list, base_window* mid)
{
  if (!mid && (mid = oldest_child())) // If we were passed 0, try to use our oldest child
  {
    mid->draw_arb_zones(gap_list, DAZ_R_CHILDREN+DAZ_R_PREVIOUS+DAZ_R_PARENT+DAZ_O_RECURSE+DAZ_O_CULL+DAZ_O_OCCLUDE+DAZ_F_SPYSUB);
  }
  else if (mid && (mid = mid->get_prev())) // If we were passed a window, and it has a previous window
  {
    mid->draw_arb_zones(gap_list, DAZ_R_CHILDREN+DAZ_R_PREVIOUS+DAZ_R_PARENT+DAZ_O_RECURSE+DAZ_O_CULL+DAZ_O_OCCLUDE+DAZ_F_SPYSUB);
  } else 
  {
    draw_arb_zones(gap_list, DAZ_F_SPYSUB); // If there truly is nothing, just draw ourselves
  }
}

/* This function should be called whenever the dimensions of the estate of a 
 * window are changed. It recalculates the positions of all the children, and
 * displays them as well. 
 */
void base_window::update_estate()
{
  update_coords();
  update_family_vislist();

  if (layout)
  {
    set_flag_cascade(grx_sensitive, false);
    layout->pack_layout(); 
    set_flag_cascade(grx_sensitive, true);
  } 
  
  display_all();
}  

// Applies 'update_vislist' to all windows in this family.
void base_window::update_family_vislist()
{
  update_vislist(); 
  LOOP_CHILDREN(loop) loop->update_family_vislist();
}

// Applies 'update_vislist' to this family and inferior windows (including parent)
void base_window::update_vislist_behind()
{
  for (base_window* loop = this; loop; loop = loop->prev)
    loop->update_family_vislist();
    
  if (parent) parent->update_vislist();
}

/* Recalculates the vis-list of a given window. Uses 'create_occludes_drawlist'
 * for this purpose, but also y-sorts the vis-zones to reduce flicker 
 */
void base_window::update_vislist()
{
  delete_zonelist(vis_list); // Delete previous vis_list, if any
  vis_list = 0;
  
  if (flag(vis_visible))
  {
    zone* new_list = create_occluded_drawlist(0); // Create a new vis_list
    if (debug & W_DEBUG_NO_YSORT)
    {
      vis_list = new_list; // Simply assign it if y-sorting is turned off
      return;
    }
  
    // Otherwise, put the zones in ascending order of 'ay', to reduce flicker
    while (new_list)
    {
      zone* temp = new_list->next;
      new_list->next = 0;
      push_sort(vis_list, new_list);
      new_list = temp;
    }
  }
}

/* This function takes a list of arbitrary zones "arb_list" and checks whether
 * any of them intersect with this window's vis_zones. If so, the intersecting 
 * portion is drawn. Additionally, it will recurse backwards and upwards towards
 * the parent (depending on flags), and occlude all windows displayed from the
 * arb-list. It will also redisplay any arb-zones that intersect with a window
 * to its next subliminal windows. Most operations are controlled by flags:
 *   DAZ_R_PREVIOUS   - Recurse to children
 *   DAZ_R_CHILDREN   - Recurse to previous windows
 *   DAZ_R_PARENT     - Recurse to parent (if any) on reaching beginning of sibling list
 *   DAZ_O_RECURSE    - Optimize recursion by checking dependancies of the arb-list.
 *   DAZ_O_CULL       - Remove any perfect fits with our vis_zones.
 *   DAZ_O_OCCLUDE    - Occlude against our window after recurse to children.
 *   DAZ_F_SPYSUB     - Indicates the subliminal-window spying should take place.
 */
void base_window::draw_arb_zones(zone*& arb_list, unsigned char arb_flags)
{
  if (!arb_list) return; // If there is no work to be done, exit
  
  zone* our_win = d_clipped();  // Our clipped area
  /* These flags determine whether we'll recurse to our prev/child windows. If
     we are optimizing recursion, this will be determined automatically. 
     Otherwise, it is set to the default, which is the values of the user flags */
  bool recurse_to_child = (arb_flags & DAZ_O_RECURSE) ? false : arb_flags & DAZ_R_CHILDREN;
  bool recurse_to_prev = (arb_flags & DAZ_O_RECURSE) ? false : arb_flags & DAZ_R_PREVIOUS;  
  bool matched = (arb_flags & DAZ_O_RECURSE) ? false : true; // Set to true if the arb-list touches us at all

  // If we are visible, loop through the vis-lists and check for overlaps
  if (vis_list && visible() && flag(grx_sensitive) && master)
  {
    // The context we will be using to draw to the master
    graphics_context grx(master->get_buffer(), get_cx(), get_cy(), master->get_theme());   
    // Loop through the arb_list
    for (zone* arb = arb_list, *next_zone = 0; arb; arb = next_zone)
    {
      // Remember the next zone to use, because we might delete this one
      next_zone = arb->next;                                 
      if (arb_flags & DAZ_O_RECURSE) // If we should optimize recursion
      {
        int result = our_win->check_intersect(arb); // Does 'arb' overlap our window?
        // If any arbs do not perfectly fit within this window, we MUST recurse to previous
        if (result) recurse_to_prev = arb_flags & DAZ_R_PREVIOUS;
        // If any arbs intersect at all with this window, we MUST recurse to children
        if (result != -1)
        {
          recurse_to_child = arb_flags & DAZ_R_CHILDREN;
          matched = true;
        } else continue; // If there are no overlaps, don't check the vis-list
      }

      // Loop through the vis-list, for each arb-zone
      for (zone* vis = vis_list; vis; vis = vis->next)
      {
        zone shared(*vis); 
        int clips = shared.clip(arb); // Determine whether this vis overlaps arb 
        if (clips == -1) continue;    // If not, continue

        grx.clip(&shared); // Otherwise, set the clipping rectangle and draw the overlap
        draw(grx);

        // Remove the arb if it fitted perfectly, because then it can be thrown out
        if (!clips && arb_flags & DAZ_O_CULL)  
        { 
          if (arb_list) arb_list = arb_list->remove(arb);
          break;
        }
      }
    }
  } else 
  {
    /* If we aren't visible, just assume that we need to recurse and that the
       arb-list touched us. I need to optimize this. */
    recurse_to_child = arb_flags & DAZ_R_CHILDREN;
    recurse_to_prev = arb_flags & DAZ_R_PREVIOUS;
    matched = true; 
  }

  if (visible() && arb_list)
  {
    // Recurse to children, if need be
    if (recurse_to_child && child && !flag(grx_master))
    {
      oldest_child()->draw_arb_zones(arb_list, (arb_flags & 0xFB) | 2);
    }
  
    // Try to inform any sub-windows, if we touched the arb-list...
    if (arb_flags & DAZ_F_SPYSUB && matched && flag(grx_sensitive))
    {
      inform_sub(arb_list);
    }
  
    /* And lastly, occlude our area from the arb-list, so it doesn't cause
       any unnecessarry sub-spying in previous windows */
    if (arb_flags & DAZ_O_OCCLUDE && matched) occlude(arb_list, our_win);
  }

  if (recurse_to_prev && arb_list) // If we should recurse backwards
  {
    /* If we have a previous window, recurse to it. If not, recurse to our 
       parent if the right flag is set. */
    if (prev) prev->draw_arb_zones(arb_list, arb_flags);
    else if (arb_flags & DAZ_R_PARENT && parent)
    {
      parent->draw_arb_zones(arb_list, arb_flags & 0xF8);
    }
  }

  delete our_win; // Delete our clipped zone
}

/* Flag to determine whether the given co-ordinate touches us. Should be over-
 * riden by subliminal windows to return true if the given co-ordinate is opaque.
 */
bool base_window::pos_visible(coord_int x, coord_int y) const
{
  if (x < get_ccx() || y < get_ccy() || x > get_cdx() || y > get_cdy()) return false;
  else return true;
}

/* This useful helper function will try to construct a zone-list representing the
 * area of this window which can be seen from the vantage point of 'stop_window'.
 * If 'stop_window' is null, then the vantage point becomes the screen. The draw
 * list will usually be constructed from the window's clipped area, but a custom
 * draw_list can be passed and the function will START with that. After the draw
 * list is constructed, it will be returned - the caller should deallocate it...
 */
zone* base_window::create_occluded_drawlist(base_window* stop_window, zone* draw_list)
{
  // If window is completely invisible, return
  if (!draw_list && !(draw_list = d_clipped())) return 0; 

  zone* occ_list = 0; // This will contain a list of all obscuring areas

  /* Starting at the first superior window, loop forwards, adding the area of
     each window to the occluding list. */
  for (base_window* loop = superior(); loop && loop != stop_window; loop = loop->next_or_uncle())
  {
    if (loop->visible()) push_back(occ_list, loop->d_clipped());
  }

  if (occ_list) // If and when we have a list of occluding windows...
  {
    occlude(draw_list, occ_list); // Occlude the original draw-list by it.
    delete_zonelist(occ_list); // And delete the occluding list
  }
  
  return draw_list; 
}

/* VERY private helper function to actually display a window. It does this by
 * looping through all the zones in the window's vis_list, setting the clipping
 * rectangle of the context around that zone, and calling the virtual 'draw()'
 * function. It also displays debugging info if necessary, and transmits a
 * display event.
 */
void base_window::_display()
{
  { // Create the graphics context, setting the origin to the win's top-left corner
    graphics_context context(master->get_buffer(), get_cx(), get_cy(), master->get_theme());
    
    // Loop through all zones in the vis_list
    for (zone* loop = vis_list; loop; loop = loop->next)
    {
      context.clip(loop); // Set the clipping rectangle of the bitmap to that zone
      draw(context); // Draw the window
    }
  }
 
  display_count++;
 
  text_mode(-1);  
  if (debug & W_DEBUG_STEP_DISPLAY) readkey();
  if (debug & W_DEBUG_DRAW_WIN_ID) textprintf(master->get_buffer(), font, cx+1,cy+1, makecol(255,255,0), "%d", int(win_id));
  if (debug & W_DEBUG_DRAW_Z_COUNT) textprintf_right(master->get_buffer(), font, dx-1, cy+1, makecol(255,255,255), "%d", get_z_count());
  if (debug & W_DEBUG_DRAW_D_COUNT) textprintf_right(master->get_buffer(), font, dx-1, cy+1, 0, "%d", display_count % 100);
 
  // Emit an event signalling that we have been displayed
  transmit(display_ei()); 
}

/* Public function to attempt to display this window. It only displays it if
 * it is actually visible and willing, and also delays the display command 
 * if delegation is turned on. It also performs sub-spying as well.
 */
void base_window::display()
{
  if (visible() && flag(sys_active) && flag(grx_sensitive) && master)
  {
    inform_sub(&clipped()); // Draw to any subliminal windows we are under

    if (master->should_delegate()) // If we should delegate,
       master->delegate(this); // add our address to the list
    else
      _display(); // Otherwise, call the low-level display function
  }
}

// Function that simply displays a family of trees by recursion.
void base_window::display_all()
{  
  LOOP_CHILDREN(loop) loop->display_all(); // Recurse to all our children  
  display(); // And then display ourselves 
}

// Calls 'inform_sub' for every member of this family, through recursion
void base_window::inform_sub_family(const zone* list)
{
  inform_sub(list); // Inform any superior sub-windows of changes       
  LOOP_CHILDREN(loop) loop->inform_sub_family(list); // Recurse to our children
}  

/* Attempts to display this window to the sub-buffers of any hungry superior
 * subliminal windows. Makes use of the hlper function 'draw_to_sub', which 
 * actually does the drawing and calls the "sub_buffer_updated" hook.
 */
void base_window::inform_sub(const zone* list)
{
  // If there is no work to be done, leave
  if (!next_sub || !master || !visible() || !list || !flag(sys_active)) return;

  delegate_displays();

  // Loop through the sub_chain
  for (window_sub* sub = next_sub; sub; sub = sub->next_sub)
  {
    // If the sub-window requires (visible, etc), draw to it.
    if (sub->visible() && sub->get_sub_buffer()) draw_to_sub(sub, list, true);
  }

  undelegate_displays();
}

/* This function attempts to draw this window to the subliminal window (sub)'s
 * sub_buffer, in any areas that overlap with (list). It will calculate the
 * occluded draw-list to that particular sub, and after drawing to that sub,
 * it will call that sub's 'sub_buffer_updated' hook if (update) is true.
 */ 
void base_window::draw_to_sub(window_sub* sub, const zone* list, bool update)
{
  // If we do not touch the subliminal window at all, leave now!
  if (c_cx>sub->c_dx || c_cy>sub->c_dy || c_dx<sub->c_cx || c_dy<sub->c_cy) return;
  
  coord_int sub_cx = sub->get_cx(); // Store the sub-window's physical co-ords
  coord_int sub_cy = sub->get_cy(); // to reduce memory accesses
  zone our_win = clipped(); // Zones representing our clipped co-ordinates,
  zone sub_win = sub->clipped(); // and the sub_window's clipped co-ordinates
  
  zone* draw_list = 0; // Will store the list of zones to draw to the sub
  
  for (const zone* loop = list; loop; loop = loop->next) // Loop through the list
  { 
    zone list_zone(*loop); // Make copy of the current zone in the list     
    zone temp1(our_win); // Make a copy of the our clipped area    
    
    if (temp1.clip(&list_zone) != -1) // If our area overlaps the list's zone...
      // If THAT overlap also intersects the subliminal window, add it to the drawlist
      if (zone* temp2 = temp1.intersect(&sub_win)) push_front(draw_list, temp2);
  }

  // If we have a draw_list, occlude it up to the point of the subliminal window
  if (draw_list) draw_list = create_occluded_drawlist(sub, draw_list);  
  if (draw_list) 
  { 
    {
      // Set up a graphics context that points to the sub-window's sub_buffer and
      // is offset so that we will draw to it correctly
      graphics_context context(sub->get_sub_buffer(), cx-sub_cx, cy-sub_cy, theme());    
      
      // Iterate through the list, drawing zones in the draw_list
      for (zone* loop = draw_list; loop; loop = loop->next)
      {
        loop->offset(-sub_cx, -sub_cy); // Normalise the draw-zone to our co-ords
        context.clip(loop);
        draw(context);
      }
    }
    // If we have been instructed to call the hook, call the hook
    if (update) sub->sub_buffer_updated(draw_list); 
    delete_zonelist(draw_list); // Lastly, delete the draw-list
  }  
}  
             
// Public function to 'clean-up' and call pre/post_unload() for all windows.
void base_window::unload()
{
  pre_unload_all();  // 'pre' phase traversal of tree
  post_unload_all(); // 'post' phase traversal of tree
}

// Public function to setup and call pre/post_load() for all windows.
void base_window::load()
{
  pre_load_all();    // 'pre' phase traversal of tree
  post_load_all();   // 'post' phase traversal of tree
}
                                              
// Helper function that executes the 1st unloading stage, calls hooks, recurses
void base_window::pre_unload_all()
{
  set_flag(sys_first_load, false); // Indicate the first 'life' is over
  set_flag(sys_loaded, false); // Indicate we are no longer loaded
  set_flag(sys_active, false); // Indicate we are no longer active 
  pre_unload();            // Call the hook  
  transmit(unload_ei());   // Broadcast an 'unload' event_info object
    
  clear_receive_list();       // Untie any event_knots from/to us
  delete_zonelist(vis_list); // Clear our vis-list (we won't need it anymore)
  vis_list = 0;
               
  LOOP_CHILDREN(loop) loop->pre_unload_all(); // Recurse to our children
}

// Helper function that calls the 2nd hook
void base_window::post_unload_all()
{
  post_unload(); // Call the hook
  
  LOOP_CHILDREN(loop) loop->post_unload_all(); // Recurse to our children
}

// Helper function that executes the 1st loading stage, calls hooks, recurses
void base_window::pre_load_all()
{
  set_flag(sys_loading); // Set this now, and unset after we are fully loaded

  if (!w()) set_w(normal_size);
  if (!h()) set_h(normal_size);
  
  pre_load();            // Call the hook
  
  set_flag(sys_loaded);  // This flag indicates that we have been pre_loaded
  update_coords();         
  pack();                // Pack our children using our layout_manager, if any
  position_children();   // Virtual func for derived windows to setup their children, if any
  transmit(load_ei());   // Broadcast a 'load' event_info object

  LOOP_CHILDREN(loop) loop->pre_load_all(); // Recurse to our children
}

// Helper function to perform the 2nd loading stage, basically just calls the hook
void base_window::post_load_all()
{
  // Recurse to all our children first, to ensure that 'post_load()' can assume
  // that all its children are active
  LOOP_CHILDREN(loop) loop->post_load_all();
  
  set_flag(sys_active);         // Indicate we are fully loaded
  
  post_load();                  // Call the hook
  
  set_flag(sys_loading, false); // Indicate we are no longer loading 
  update_vislist(); 
}

// Function to calculate the physical and clipped co-ordinates from the logicals
void base_window::update_coords()
{
  cx = ax;
  cy = ay;

  if (parent && !parent->flag(grx_master))
  {
    cx += parent->cx + (flag(vis_ignore_estate) ? 0 : parent->e_ax());
    cy += parent->cy + (flag(vis_ignore_estate) ? 0 : parent->e_ay());
  }
  dx = cx + w();
  dy = cy + h();

  clip_coords();

  // Update co-ordinates of ALL our children
  LOOP_CHILDREN(loop) loop->update_coords();
}

// Helper function to calculate the clipped co-ordaintes
void base_window::clip_coords()
{
  // Set our clipped co-ords to our physical coords, for now
  c_cx = cx; c_cy = cy; c_dx = dx; c_dy = dy; 

  set_flag(vis_complete_clip, false); // Reset all the flags
  set_flag(vis_positive_clip, false);
  set_flag(vis_negative_clip, false);

  if (!parent) return; // Can't do any clipping without a parent
  
  // These represent the limits to which our coords must be clipped
  coord_int max_x, max_y, min_x, min_y; 
  
  // Find the limits:
  if (parent->flag(grx_master)) 
  { // If our parent's a master, there is no multi-tiered clipping
  
    if (flag(vis_ignore_estate))
    {
      min_x = 0; 
      min_y = 0; 
      max_x = parent->w(); 
      max_y = parent->h();
    } else { 
      min_x = parent->e_ax(); 
      min_y = parent->e_ay(); 
      max_x = parent->e_bx(); 
      max_y = parent->e_by();
    }
  } else 
  {
    if (flag(vis_ignore_estate)) 
    {
      min_x = parent->c_cx; // For a normal window, the limits are the clipped 
      min_y = parent->c_cy; // co-ordinates of the parent, which takes care of
      max_x = parent->c_dx; // multi-tiered clipping.
      max_y = parent->c_dy;  
    } else 
    { 
      // If we aren't ignoring the estate, we have to add it to the equation
      min_x = parent->cx + parent->e_ax(); if (parent->c_cx > min_x) min_x = parent->c_cx;
      min_y = parent->cy + parent->e_ay(); if (parent->c_cy > min_y) min_y = parent->c_cy; 
      max_x = parent->cx + parent->e_bx(); if (parent->c_dx < max_x) max_x = parent->c_dx;
      max_y = parent->cy + parent->e_by(); if (parent->c_dy < max_y) max_y = parent->c_dy; 
    }        
  }

  // Check for complete clipping. If our parent is completely clipped, we are too.
  if (parent->flag(vis_complete_clip) || c_cx > max_x || c_cy > max_y || 
      c_dx < min_x || c_dy < min_y)
  {
    c_cx = c_cy = c_dx = c_dy = -1;
    set_flag(vis_complete_clip); 
    
  } else
  {
    // If not, check if we are partially clipping on either side
    if (c_dx > max_x)
    {
      c_dx = max_x;
      set_flag(vis_positive_clip); 
    }
    if (c_dy > max_y)
    {
      c_dy = max_y;
      set_flag(vis_positive_clip);
    }
    if (c_cx < min_x)
    {
      c_cx = min_x;
      set_flag(vis_negative_clip);
    }
    if (c_cy < min_y)
    {
      c_cy = min_y;
      set_flag(vis_negative_clip);
    }
  }
}

/* These functions are used to interface with the work-horse 'move_resize' 
 * function. Most of them treat co-ordinates that are equal to the constant
 * 'normal_size' as special. It either means "use the previous value for this
 * coordinate" or "use the widget's default for this dimension". These functions
 * also repack the parent where necessarry. Note that 'moving' has no effect 
 * when the window is being managed by a layout manager.
 */
 
// Move our window to these co-ordinates
void base_window::move(coord_int _ax, coord_int _ay)
{
  if (is_laid_out()) return;             
  move_resize(_ax, _ay, _ax+w(), _ay+h());
}

// Move our window BY these co-ordinates (mickeys)
void base_window::relative_move(coord_int x, coord_int y)
{
  if (is_laid_out()) return;             
  move_resize(ax+x, ay+y, bx+x, by+y);
}

/* Resize our window to this width and height. If either value is 'normal_size', 
 * then the widget's default dimension is used for that value. */
void base_window::resize(coord_int width, coord_int height)
{
  if (is_laid_out()) 
    layinfo->resize((width==normal_size) ? normal_w() : width, (height==normal_size) ? normal_h() : height);
  else 
    move_resize(ax, ay, ax + ((width==normal_size) ? normal_w() : width), ay + ((height==normal_size) ? normal_h() : height));
}

/* Position the window at the x and y co-ordinates, and use the given width and
 * height. */
void base_window::resize(coord_int x, coord_int y, coord_int width, coord_int height)
{
  if (is_laid_out()) 
    layinfo->resize((width==normal_size) ? normal_w() : width, (height==normal_size) ? normal_h() : height);
  else 
    move_resize(x, y, x + ((width==normal_size) ? normal_w() : width), y + ((height==normal_size) ? normal_h() : height));
}

// Set the window's width - if 'normal_size', the widget's default width is used.
void base_window::set_w(coord_int width)
{
  if (is_laid_out()) 
    layinfo->resize((width==normal_size) ? normal_w() : width, layinfo->get_ideal_h());
  else 
    move_resize(ax, ay, ax + ((width==normal_size) ? normal_w() : width), by);
}

// Set the window's height - if 'normal_size', the widget's default height is used.
void base_window::set_h(coord_int height)
{
  if (is_laid_out()) 
    layinfo->resize(layinfo->get_ideal_w(), (height==normal_size) ? normal_h() : height);    
  else 
    move_resize(ax, ay, bx, ay + ((height==normal_size) ? normal_h() : height));
}

/* This adjusts the window's four co-ordinates manually. If any of them are set
 * to 'normal_size', then the window's original co-ordinate will be used for
 * that value.
 */
void base_window::place(coord_int _ax, coord_int _ay, coord_int _bx, coord_int _by)
{
  if (is_laid_out()) return; 
  move_resize((_ax==normal_size) ? ax : _ax, (_ay==normal_size) ? ay : _ay, 
              (_bx==normal_size) ? bx : _bx, (_by==normal_size) ? by : _by);
}
                                                              
/* This behaves like 'place', as it sets all four co-ordinates manually, but it
 * doesn't use the special behaviour for 'normal_size' */                                                              
void base_window::move(coord_int _ax, coord_int _ay, coord_int _bx, coord_int _by)
{
  if (is_laid_out()) layinfo->resize(_bx - _ax, _by - _ay);
  else move_resize(_ax, _ay, _bx, _by);
}

/* This last function is quite useful. It places the window 'up against the
 * side' of its parent, in the direction given by C. If C is a corner direction
 * (NE, SE, etc), then the window maintains its original width and height. 
 * Otherwise, it is stretched to fill up the entire 'side' it is placed against.
 * However, its thickness in the perpendicular direction is determined by 'side'.
 * If 'side' is 0, its original dimension is used. If side is 'normal_size', then
 * the widget's default dimension is used for that value. Complex stuff, innit?
 */
void base_window::place(compass_orientation c, coord_int side)
{
  if (is_laid_out()) return;

  coord_int width = w();
  coord_int height = h(); 
  coord_int x, y;
  switch (c) 
  {
    case c_north_west: 
      y = 0; x = 0;                  
      break;
    case c_north:    
      if (side) { width = parent->e_w(); height = (side != normal_size) ? side : normal_h(); }
      y = 0; x = coord_int(parent->e_w()/2.0-width/2.0); 
      break;
    case c_north_east: 
      y = 0; x = parent->e_w()-width;                
      break;
    case c_west:       
      if (side) { height = parent->e_h(); width = (side != normal_size) ? side : normal_w(); }
      y = coord_int(parent->e_h()/2.0-height/2.0); x = 0;                  
      break;
    case c_centre:     
      y = coord_int(parent->e_h()/2.0-height/2.0); x = coord_int(parent->e_w()/2.0-width/2.0); 
      break;
    case c_east:                      
      if (side) { height = parent->e_h(); width = (side != normal_size) ? side : normal_w(); }
      y = coord_int(parent->e_h()/2.0-height/2.0); x = parent->e_w()-width;                
      break;
    case c_south_west: 
      y = parent->e_h()-height; x = 0;                  
      break;
    case c_south:      
      if (side) { width = parent->e_w(); height = (side != normal_size) ? side : normal_h(); }
      y = parent->e_h()-height; x = coord_int(parent->e_w()/2.0-width/2.0); 
      break;
    case c_south_east: 
      y = parent->e_h()-height; x = parent->e_w()-width;                
      break;
  }  
  move_resize(x, y, x+width, y+height);
}

/* This function adds a given window to this window as a new child, assigning it
 * the layout_info object provided (if any), and placing at the front or back of
 * the sibling list if front is true or false respectively. If the new child needs
 * to be loaded, it will be loaded and displayed. NOTE: The new child MUST be at
 * the top of its window tree, if there is more than one window involved. 
 */ 
void base_window::add_child(base_window* new_child, layout_info* lm, bool front)
{                   
  // If we are given an invalid child (none, already active, or non-root), return        
  if (!new_child || new_child->parent || new_child->flag(sys_loaded)) return;
  
  if (child) // If we already have a child, 'stitch' it into the sibling list
  {
    if (front) // If the new child is supposed to go in front
    {
      base_window* last = oldest_child(); // Find the oldest child
      last->next = new_child; // Set its 'next' to the new_child
      new_child->prev = last; // Set the new_child's 'prev' to it
      
    } else { // If the new child is going behind
    
      new_child->next = child; // Set its 'next' to our first child
      child->prev = new_child; // Set our first child's 'prev' to it
      child = new_child; // Set our first child to the new child
    } 
    
  } else // If we don't have a child, then make this the first one
  {   
    child = new_child; 
  }

  new_child->parent = this; // Set the new child's parent to us

  // If the new window is a subliminal window, set all the relevant pointers behind it
  if (window_sub* qualified = dynamic_cast<window_sub*>(new_child))
    new_child->set_next_sub_behind(qualified);

  // If we are a manager, set the new child's family to point to us as manager
  if (window_manager* qualified = dynamic_cast<window_manager*>(this))
    new_child->set_manager(qualified);
  else 
    new_child->set_manager(manager); // Otherwise, get them to point to our manager
  
  // If we are a master, set the new child's family to point to us as master. 
  new_child->set_master(flag(grx_master) ? (dynamic_cast<window_master*>(this)) : master);
    
  // Set the new childs subliminal pointer to point to its next subliminal window
  new_child->set_next_sub(new_child->find_next_sub());
  
  // If we were passed a layout_info object, bind it to the new child
  if (lm) new_child->set_layinfo(lm);
  
  if (!flag(vis_visible)) 
  {
    new_child->set_flag(vis_should_be_visible, new_child->flag(vis_visible));
    hide_helper(); 
    new_child->set_flag_cascade(vis_visible, false);
  }
  
  if (!new_child->flag(sys_active) && !new_child->flag(sys_loading) && flag(sys_active))
  {
    new_child->load();
    new_child->display_all();
  }  
}

bool base_window::is_laid_out()
{
  if (parent && parent->layout && layinfo) return true; else return false;
}

/* This function sets a new layout manager for the this window, deleting any 
 * previously existing layout manager. The supplied layout manager can be NULL.
 * The window will be repacked if it is currently active.
 */ 
void base_window::set_layout(layout_manager* l)
{
  delete layout; 
  
  if ((layout = l)) 
  {
    layout->set_container(this);
    if (flag(sys_loaded)) layout->pack_layout();
  }
}

/* This is similar to 'set_layout'. The argument can be null, and no layout will
 * be used. If a layout-info object is used, the parent will be repacked 
 */ 
void base_window::set_layinfo(layout_info* l)
{
  delete layinfo;

  if ((layinfo = l))
  {
    l->set_master(this);
    parent->pack();
  }
}

/* This interesting function will set the given flag to the given value in this
 * window, AS WELL AS ITS CHILDREN. In other words, it effects the entire tree.
 * It has three versions: one for each type of flag.
 */
void base_window::set_flag_cascade(private_flags f, bool b)
{
  set_flag(f, b);
  LOOP_CHILDREN(loop) loop->set_flag_cascade(f, b);
}

void base_window::set_flag_cascade(protected_flags f, bool b)
{
  set_flag(f, b);
  LOOP_CHILDREN(loop) loop->set_flag_cascade(f, b);
}

void base_window::set_flag_cascade(public_flags f, bool b)
{
  set_flag(f, b);
  LOOP_CHILDREN(loop) loop->set_flag_cascade(f, b);
}

/* Helper function to set a particular window and its tree to use the supplied
 * argument as their master 
 */
void base_window::set_master(window_master* n)
{
  master = n;
  
  // If WE are a master, our children should use us and not 'n'
  if (!flag(grx_master)) LOOP_CHILDREN(loop) loop->set_master(n);
}

// Helper function to set the manager for all windows in this family
void base_window::set_manager(window_manager* m)
{
  manager = m;
  LOOP_CHILDREN(loop) loop->set_manager(m);
}

// This function returns the first superior subliminal window from this window
window_sub* base_window::find_next_sub()
{
  // Loop from our next/uncle window until the master is reached
  for (base_window* loop = next_or_uncle(); loop; loop = loop->next_or_uncle())
  {
    // If the current window is a subliminal window (according to its flags), return it
    if (loop->flag(grx_subliminal)) return dynamic_cast<window_sub*>(loop);
  }
  return 0;
}

// This asks the window-manager to give this window the focus
void base_window::set_keyfocus()
{
  manager->set_keyfocus(this);
}

// This asks the window-manager to change the mouse-cursor to the given enum
void base_window::set_cursor(cursor_name cur)
{
  manager->set_cursor(cur);
} 

/* This function is used to work backwards in the tree, setting the 'next_sub'
 * of any starving windows to point to the given sub. It goes back until
 * it reaches the limit of the subliminal window's jurisdiciton or until another
 * subliminal window is discovered.
 */
void base_window::set_next_sub_behind(window_sub* sub)
{
  base_window* loop;
  for (loop = prev; loop; loop = loop->prev)
  {
    base_window* end_n = loop->next;
    for (base_window* n = loop; n != end_n; n = n->superior())
    {
      if (n->next_sub && loop->ancestor_of(n->next_sub)) continue;

      n->next_sub = sub;
    }

    if (dynamic_cast<window_sub*>(loop)) return;
  }

  if (!loop && parent) parent->next_sub = sub;
}

/* This helper function attempts to set the next subliminal window of all the
 * windows within this family, if they are willing and don't already have an
 * internal next_sub.
 */
void base_window::set_next_sub(window_sub* sub)
{
  // Loop through every window within our family, stopping at our next/uncle window
  for (base_window* loop = this, *end_loop = loop->next_or_uncle();
      loop != end_loop;
      loop = loop->superior())
  {
    // If the window already has a pointer to another internal window, skip it
    if (loop->next_sub && ancestor_of(loop->next_sub)) continue;
    loop->next_sub = sub;
  } 
}

/* This dodgy function is used to remove the preceeding 'numbers' that happen
 * in DJGPP's implementation of the type_info naming scheme. It returns a pointer
 * to the middle of the std::string, sans the initial numbers 
 */
const char* base_window::type_name() const
{
  const char* type = typeid(*this).name();
  while (*type && *type >= '0' && *type <= '9') type++;
  if (*type) return type; else return 0;
}

/* This function 'prints' the family of this window to the given std::ostream, which 
 * should usually be a std::string-stream handled by the console. It makes use of 
 * some custom symbols in order to make the ASCII tree more readable. It is 
 * complex and boring, so there is no commenting.
 */
void base_window::print(std::ostream& str)
{
  static int indent = 0;
  const char* type = type_name();

  str << type << "(" << win_id << ") - [" << cx << "," << cy << "," << dx << ", " << dy << "]";
  if (next_sub) str << " (sub: " << next_sub->win_id << ")";
  str << "\n";

  indent += 3;
  
  LOOP_CHILDREN(loop)
  {                          
    for (int a = 0; a < indent-2; a++)
    {
      if (a % 3) 
        str << " "; 
      else 
      {
        if (a == indent-3) 
        {
          str << (loop->next ? char(28) : char(29));
        } else if (parent)
        { 
          base_window* b = loop;          
          for (int c = indent-3; c != a; b = b->parent, c -= 3);
          str << (b->next ? char(27) : char(32));
        } else 
		  std::cerr << char(27);
      }
    }

    str << char(30) << char(31);    
    loop->print(str);
  }
  
  indent -= 3;
}

/* This bizarre little function calls the function pointed to by 'func' for
 * each window in its family. If any of these calls returns a non-zero value,
 * this value is returned immediately by 'for_all'.
 */
int base_window::for_all(int(func)(base_window*))
{
  if (int temp = func(this)) return temp;
  else
  {
    LOOP_CHILDREN(loop)
    {
      if (int temp = loop->for_all(func)) return temp;
    }
  }
  return 0;
}

// This function returns the number of windows 'high' we are in the sibling list.
int base_window::get_z_count() const
{
  int count = -1;
  for (const base_window* loop = this; loop; loop = loop->prev)
  {
    count++;
  }
  return count;
}

/* This non-member function returns true if the given two windows technically
 * intersect. It presumes the windows belong to the same master, and also doesn't
 * take into account clipping.
 */
bool window_intersect(base_window* a, base_window* b)
{
  if (a->get_dx() < b->get_cx() || a->get_dy() < b->get_cy() || 
      a->get_cx() > b->get_dx() || a->get_cy() > b->get_dy()) return false;
  else return true;
}

/* This function returns a pointer to the window which resides under the given
 * point. It checks all its children to determine if it should recurse to
 * to any of them. If not, it returns the result of 'pos_visible', which should
 * return true or false if that part of the window is 'solid'.
 */
base_window* base_window::find_window_under(coord_int x, coord_int y)
{
  if (!visible()) return false;

  // Loop backwards through all our children (if any)
  for (base_window* loop = oldest_child(); loop; loop = loop->prev)
  {
    // If the point lies within that particular child...
    if (x>loop->c_cx && x<loop->c_dx && y>loop->c_cy && y<loop->c_dy)
    {
      // Recurse to it to find that point
      if (base_window* result = loop->find_window_under(x, y)) return result;
    }
  }

  // If it did not touch any of our children, check if it touches us. 
  return pos_visible(x, y) ? this : 0;
}


/* This public function attempts to repack this window's children, if there is
 * a layout manager and the window is active
 */
void base_window::pack()
{
  if (layout && flag(sys_loaded)) layout->pack_layout();
}

// Ostream inserter. Simply lists this window's type followed by it's ID.
std::ostream& operator<<(std::ostream& os, base_window& w)
{
  os << w.type_name() << "{" << w.win_id << "}" << std::endl;
  
  return os;
}
                                                 
/* The following functions attempt to 'localise' the co-ordinates of a contained 
 * window. They loop upwards until they hit this window, summing all the logical
 * and estate dimensions to find the given window's position relative to us
 */
coord_int base_window::local_ax(base_window& w) const
{
  coord_int x = w.get_ax();
  for (base_window* loop = w.get_parent(); loop; loop = loop->get_parent(), x += loop->e_ax() + loop->get_ax())
    if (loop == this) break;
  return x;
}

coord_int base_window::local_ay(base_window& w) const
{
  coord_int y = w.get_ay();
  for (base_window* loop = w.get_parent(); loop; loop = loop->get_parent(), y += loop->e_ay() + loop->get_ay())
    if (loop == this) break;
  return y;
}

coord_int base_window::local_bx(base_window& w) const
{
  coord_int x = w.get_bx(); 
  for (base_window* loop = w.get_parent(); loop; loop = loop->get_parent(), x += loop->e_ax() + loop->get_ax())
    if (loop == this) break;
  return x;
}

coord_int base_window::local_by(base_window& w) const
{
  coord_int y = w.get_by(); 
  for (base_window* loop = w.get_parent(); loop; loop = loop->get_parent(), y += loop->e_ay() + loop->get_ay())
    if (loop == this) break;
  return y;
}

/* This function simply creates returns a packed colour in an integer to 
 * represent the given R, G, and B values. It uses the colour depth of this 
 * window's bitmap surface.
 */
int base_window::get_color(int r, int g, int b) const
{
  return makecol_depth(get_depth(), r, g, b);
}

// This function returns the colour depth of this window's bitmap surface. 
int base_window::get_depth() const
{
  return bitmap_color_depth(master->get_buffer());
}

/* Called by the master window whenever it receives a mouse-click. This helper
 * sets the drag mode and mouse cursor as necessary.
 */
void drag_helper::down(base_window* win)
{
  coord_int x, y;
  
  // Calculate whether the click was on the borders of the window
  
  if (win->get_click_x() >= win->w()-6) x = 1;  // Right border
  else if (win->get_click_x() <= 6)     x = -1; // Left border
  else                                  x = 0;  
  
  if (win->get_click_y() >= win->h()-6) y = 1;  // Bottom border
  else if (win->get_click_y() <= 6)     y = -1; // Top border
  else                                  y = 0;  
 
  drag_mode = get_dir(x, y); // Set the drag-mode based on this
  
  // Use compass direction to get mouse-cursor
  win->set_cursor(compass_to_cursor(drag_mode)); 
}



void drag_helper::up(base_window* win)
{
  win->set_cursor(cursor_normal);
}

void drag_helper::drag(base_window* win, coord_int x, coord_int y)
{
  switch (get_y(drag_mode))
  {
    case -1: if (win->h() - y < min_h) y = win->h() - min_h; break;
    case 1:  if (win->h() + y < min_h) y = -(win->h() - min_h); break;
  }
  
  switch (get_x(drag_mode))
  {
    case -1: if (win->w() - x < min_w) x = win->w() - min_w; break;
    case 1:  if (win->w() + x < min_w) x = -(win->w() - min_w); break;
  } 
               
  switch (drag_mode)
  {
    case c_centre: win->relative_move(x, y); win->set_cursor(cursor_move); break;
    case c_north: win->place(base_window::normal_size, win->get_ay()+y, base_window::normal_size, base_window::normal_size); break;
    case c_north_east: win->place(base_window::normal_size, win->get_ay()+y, win->get_bx()+x, base_window::normal_size); break;
    case c_east: win->set_w(win->w()+x); break;
    case c_south_east: win->resize(win->w()+x, win->h()+y); break;
    case c_south: win->set_h(win->h()+y); break;
    case c_south_west: win->place(win->get_ax()+x, base_window::normal_size, base_window::normal_size, win->get_by()+y); break;
    case c_west: win->place(win->get_ax()+x, base_window::normal_size, base_window::normal_size, base_window::normal_size); break;
    case c_north_west: win->place(win->get_ax()+x, win->get_ay()+y, base_window::normal_size, base_window::normal_size); break;
  }
}

