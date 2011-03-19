#include "playout.h"

#include "pbasewin.h" // For moving/resizing the windows

#include <stack> // For temporary values in coordinate justification calculations

layout_info* layout_manager::get_first_layinfo() const
{
  for (base_window* loop = container->get_child(); loop; loop = loop->get_next())
    if (loop->get_layinfo()) return loop->get_layinfo();

  return 0;
}

/* This function attempts to pack the layout manager. If a packing session is
 * already underway (and we have been recursed to by 'suggest_size'), then 
 * make sure we pack twice and quite. If not, start packing, and only stop
 * when the 'suggests' have run out */
void layout_manager::pack_layout()
{
  if (!packing)
  {
    packing = true;
    do
    {
      repack = false;
      pack();
    } while (repack);
    
    packing = false;
    
  } else
  {
    repack = true;
  }
}

/* Used to allow containers to resize themselves to accomodate children. This is
 * only done if the 'conserve' flag has been set. In this event, the container
 * is resized using the normal methods (if the container is itself being managed
 * then the container's parent will be repacked and there might be a nasty loop.
 * That's why 'conserve' is set to false before resizing - to prevent recursion
 */
void layout_manager::suggest_size(coord_int w, coord_int h)
{
  if (conserve)
  { 
    conserve = false;
    
    if (w == -1) w = container->e_w();
    if (h == -1) h = container->e_h();
    
    w += (container->e_ax() + (container->w() - container->e_bx()));
    h += (container->e_ay() + (container->h() - container->e_by()));
    
    container->resize(w, h);
    
    conserve = true;
  }
}

// Find the next layout info object belonging to our master's next windows
layout_info* layout_info::get_next() const
{ 
  for (base_window* loop = master->get_next(); loop; loop = loop->get_next())
    if (loop->get_layinfo()) return loop->get_layinfo();
    
  return 0;
}

void layout_info::pack() const // Repack our master's sibling list if necessary
{ 
  if (master && master->parent) master->parent->pack(); 
}  

void layout_manager::set_container(base_window* c)
{
  container = c;
}

void layout_info::set(li_expand_type e, coord_int p_w, coord_int p_h, compass_orientation c)
{ 
  expand_x = e & li_expand_w; 
  expand_y = e & li_expand_h; 
  align = c; 
  pad_w = p_w; 
  pad_h = p_h; 
  pack();
}

void layout_info::resize(coord_int w, coord_int h)
{
  ideal_w = w;
  ideal_h = h;
  pack();
}

void layout_info::set_master(base_window* m)
{
  master = m;

  ideal_w = master->w();
  ideal_h = master->h();
  
  if (ideal_w == 0) ideal_w = master->normal_w();
  if (ideal_h == 0) ideal_h = master->normal_h();
}

/* This function takes a reference to a zone representing the area it has been
 * offered by the layout manager. It then uses its own 'expand' and 'ideal' 
 * variables to decide on the portion of this zone it will occupy. It then
 * resizes itself to occupy this portion. If the portion is 0, the window is
 * hidden.
 */
void layout_info::consume(const zone& area) const
{
  zone z(area);  // Create a new zone (z) that has been clipped off on all sides
  z.ax += pad_w; // according to our 'pad' variables.
  z.ay += pad_h;
  z.bx -= pad_w;
  z.by -= pad_h; 
  z.ax = PMAX(z.ax, area.bx); // Make sure the area isn't padded out of existence
  z.ay = PMAX(z.ay, area.by);
  z.bx = PMIN(z.bx, z.ax);
  z.by = PMIN(z.by, z.ay);   

  // Find ultimate width and height based on rules
  coord_int w = expand_x ? z.w() : PMAX(ideal_w, z.w()); 
  coord_int h = expand_y ? z.h() : PMAX(ideal_h, z.h()); 
   
  // Find the 'spare' width and height which are left, if any. In other words,
  // the amount of width/height that is still left which we should align against.
  coord_int sw = (z.w() - w) / 2;
  coord_int sh = (z.h() - h) / 2;

  coord_int ax, ay; // Set to our master's final x and y based on orientation 
  switch (align)
  {
    case c_north_west: ax = z.ax;       ay = z.ay;       break;
    case c_north:      ax = z.ax + sw;  ay = z.ay;       break;
    case c_north_east: ax = z.bx - w;   ay = z.ay;       break;
    case c_west:       ax = z.ax;       ay = z.ay + sh;  break;
    case c_centre:     ax = z.ax + sw;  ay = z.ay + sh;  break;
    case c_east:       ax = z.bx - w;   ay = z.ay + sh;  break;
    case c_south_west: ax = z.ax;       ay = z.by;       break;
    case c_south:      ax = z.ax + sw;  ay = z.by;       break;
    case c_south_east: ax = z.bx - w;   ay = z.by;       break;
  }
                                
  if (w <= 0 || h <= 0) // If we are too small, hide ourselves by setting extreme co-ordinates
  {
    master->move_resize(-32768, -32768, -32768, -32768);
    hidden = true;
  } else
  {
    master->move_resize(ax, ay, ax + w, ay + h); // Place our master at this new position
    hidden = false;
  }
}

/* Left->right packing algorithm. If it is supposed to make sure that all objects
 * in each row are offered the same height, it needs to temporarily store the
 * objects until it knows the maximum height for that row. That is where the 
 * stack comes in. Additionally, if the entire row is supposed to be centred, 
 * then it needs to know the total width of the row before it can do this. */
void flow_layout::pack()
{
  coord_int x = x_margin; // Current x co-ordinate
  coord_int y = y_margin; // Current y co-ordinate
  coord_int max_y = 0;    // Maximum height for this row  
  coord_int c_w = container->e_w(); // Maximum width
  coord_int c_h = container->e_h(); // Maximum height
 
	std::stack< std::pair<layout_info*, zone> > chunk_stack;
  
  bool skip_row = false;
  
  // Loop through all lay-info objects belonging to container's children
  for (layout_info* loop = get_first_layinfo(); loop; loop = loop->get_next())
  {
    // Find preferred width and height of window (if expand_w, then this is 
    // the width of the entire row)
    coord_int pref_w = loop->get_w();
    coord_int pref_h = loop->get_h();
  
    if ((skip_row || (x + pref_w > (c_w - x_margin))) && max_y) // If this window will go over the edge
    {
      while (!chunk_stack.empty())
      {
        zone pos = chunk_stack.top().second;                       
        if (fill) pos.by = pos.ay + max_y;
        if (centre) pos.offset((c_w -x)/2,0); 
        chunk_stack.top().first->consume(pos);      
        chunk_stack.pop();
      }
    
      x = x_margin; // Start from the left again
      y += max_y + 1 + y_margin; // Drop down one row 
      max_y = 0; // Start counting the height from scratch       
      skip_row = false;
    }
    
    if (!fill && !centre)
      loop->consume(zone(x, y, x + pref_w, y + pref_h)); // Resize the window to fit its new position
    else
		chunk_stack.push(std::pair<layout_info*, zone>(loop, zone(x, y, x + pref_w, y + pref_h)));
    
    x += pref_w + 1 + x_margin; // Advance x position by width+spacing+1 
    if (pref_h > max_y) max_y = pref_h; // Keep track of the maximum height of row
    
    skip_row = loop->get_expand_w();
  }
        
  while (!chunk_stack.empty())
  {
    zone pos = chunk_stack.top().second;                       
    if (fill) pos.by = pos.ay + max_y;
    if (centre) pos.offset(((c_w - x_margin)-x)/2,0); 
    chunk_stack.top().first->consume(pos);      
    chunk_stack.pop();
  }
      
  suggest_size(-1, y + max_y + y_margin);
}

void grid_layout::pack()
{                                       
  int w = grid_w, h = grid_h;
       
  if (!grid_w || !grid_h)    
    for (layout_info* loop = get_first_layinfo(); loop; loop = loop->get_next())
    {
      if (grid_info* gi = dynamic_cast<grid_info*>(loop))
      {
        if (!grid_w && gi->x_index+gi->x_extend > w) w = gi->x_index+gi->x_extend;
        if (!grid_h && gi->y_index+gi->y_extend > h) h = gi->y_index+gi->y_extend;
      }
    }
    
  // Amount of space wasted for margins:
  coord_int x_margin_total = (w + 1) * x_margin; 
  coord_int y_margin_total = (h + 1) * y_margin;
        
  // Width and height of each cell:
  float cell_w = float(container->e_w() - x_margin_total) / w;
  float cell_h = float(container->e_h() - y_margin_total) / h;

  // Loop through all layout-info objects
  for (layout_info* loop = get_first_layinfo(); loop; loop = loop->get_next())
  {
    if (grid_info* gi = dynamic_cast<grid_info*>(loop)) // If its a grid-info object
    {
      int x_pos = gi->x_index+1; // Column position
      int y_pos = gi->y_index+1; // Row position
      int x_ext = gi->x_extend; // Number of columns
      int y_ext = gi->y_extend; // Number of rows

      // Stop it from going out-of-bounds
      if (x_pos < 0) x_pos = 0; else if (x_pos + x_ext > w + 1) x_pos = w + 1 - (x_pos + x_ext);
      if (y_pos < 0) y_pos = 0; else if (y_pos + y_ext > h + 1) y_pos = h + 1 - (y_pos + y_ext);
  
      // Calculate the actual x and y   
      float x = (x_pos * x_margin) + ((x_pos - 1) * cell_w);
      float y = (y_pos * y_margin) + ((y_pos - 1) * cell_h);
  
      // Calculate the width and height in pixels for this window
      float total_w = (x_ext * cell_w) + ((x_ext - 1) * x_margin);
      float total_h = (y_ext * cell_h) + ((y_ext - 1) * y_margin);
  
      gi->consume(zone(x,y,x+total_w,y+total_h)); // Assign it the space
    }
  }
}

void compass_layout::pack()
{
  coord_int max_w = container->e_w(), max_h = container->e_h(); 
  coord_int north_a = x_margin, north_b = max_w-x_margin, north_h = y_margin; 
  coord_int east_a = y_margin, east_b = max_h-y_margin, east_w = max_w-x_margin; 
  coord_int south_a = x_margin, south_b = max_w-x_margin, south_h = max_h-y_margin; 
  coord_int west_a = y_margin, west_b = max_h-y_margin, west_w = x_margin;

  for (layout_info* loop = get_first_layinfo(); loop; loop = loop->get_next())
  {
    if (compass_info* ci = dynamic_cast<compass_info*>(loop))
    {
      coord_int w = ci->get_w(), h = ci->get_h();     
      switch (ci->pos)
      {
        case c_north:
        {
          ci->consume(zone(north_a, north_h, north_b, north_h + h));
          north_h += h + 1 + x_margin;
          if (north_h > east_a) east_a = north_h;
          if (north_h > west_a) west_a = north_h;  
        } break;
        case c_north_east:
        {
          ci->consume(zone(east_w - w, north_h, east_w, north_h + h));
          if (north_h + h + y_margin >= east_a) east_a = north_h + h + 1 + y_margin;
          if (east_w - w - x_margin <= north_b) north_b = east_w - w - 1 - x_margin; 
        } break;
        case c_east:
        {
          ci->consume(zone(east_w - w, east_a, east_w, east_b));
          east_w -= w + 1 + x_margin;
          if (east_w < north_b) north_b = east_w;
          if (east_w < south_b) south_b = east_w;
        } break;
        case c_south_east:
        {
          ci->consume(zone(east_w - w, south_h - h, east_w, south_h));
          if (east_w - w - x_margin <= south_b) south_b = east_w - w - 1 - x_margin;
          if (south_h - h - y_margin <= east_b) east_b = south_h - h - 1 - y_margin;
        } break;
        case c_south:
        {
          ci->consume(zone(south_a, south_h - h, south_b, south_h));
          south_h -= h + 1 + y_margin;
          if (south_h < east_b) east_b = south_h;
          if (south_h < west_b) west_b = south_h;
        } break;
        case c_south_west:
        {
          ci->consume(zone(west_w, south_h - h, west_w + w, south_h));
          if (south_h - h - y_margin <= west_b) west_b = south_h - h - 1 - y_margin;
          if (west_w + w + x_margin >= south_a) south_a = west_w + w + 1 + x_margin;  
        } break;
        case c_west:
        {
          ci->consume(zone(west_w, west_a, west_w + w, east_b));
          west_w += w + 1 + x_margin;
          if (west_w > north_a) north_a = west_w;
          if (west_w > south_a) south_a = west_w;
        } break;
        case c_north_west:
        {
          ci->consume(zone(west_w, north_h, west_w + w, north_h + h));
          if (west_w + w + x_margin >= north_a) north_a = west_w + w + 1 + x_margin;
          if (north_h + h + y_margin >= west_a) west_a = north_h + h + 1 + y_margin;
        } break;
        case c_centre:
        {
          ci->consume(zone(west_w, north_h, east_w, south_h));
        } break;
      }
    }
  }
}

void gobbler_layout::pack()
{
  // This is the area that we will be 'gobbling' from
  zone area(x_margin, y_margin, container->e_w() - x_margin, container->e_h() - y_margin);
  
  // Loop through all the layout-info objects
  for (layout_info* loop = get_first_layinfo(); loop; loop = loop->get_next())
  {
    if (gobbler_info* gi = dynamic_cast<gobbler_info*>(loop))
    {
      float bias = gi->get_bias();
      coord_int bw, bh; 
      
      if (bias >= 0 && bias <= 1) // If it is using relative positioning
      {
        bw = area.w() * bias; // Calculate what portion of the space it would get
        bh = area.h() * bias;
      } else
      {
        bw = (coord_int)bias; // Otherwise, if it is absolute positioning, 
        bh = (coord_int)bias; // just cast the floating point to an integer
      }  
            
      switch (gi->get_dir()) // Check the direction the window is being added...
      {
        case d_north: 
          gi->consume(zone(area.ax, area.ay, area.bx, area.ay+bh)); // Chomp from the top of the area
          area.ay += bh + y_margin; // Reduce the space available from the top          
          break;

        case d_east:
          gi->consume(zone(area.bx-bw, area.ay, area.bx, area.by)); // Chomp from the right of the area
          area.bx -= bw + x_margin; // Reduce the space available from the right                                    
          break;

        case d_south:
          gi->consume(zone(area.ax, area.by-bh, area.bx, area.by)); // Chomp from the bottom of the area
          area.by -= bh + y_margin; // Reduce the space available from the bottom
          break;

        case d_west:
          gi->consume(zone(area.ax, area.ay, area.ax+bw, area.by)); // Chomp from the left of the area
          area.ax += bw + x_margin; // Reduce the space available from the left                   
      } 
      if (area.w() <= 0 || area.h() <= 0) return; // If we run out of space, stop
    }
  }
}

