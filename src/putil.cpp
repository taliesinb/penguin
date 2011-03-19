#include "putil.h"

compass_orientation get_dir(int w, int h)
{
  if (w > 0)
  {
    if (h > 0) return c_south_east; else 
    if (h < 0) return c_north_east; else
    return c_east;
  } else 
  
  if (w < 0) 
  {
    if (h > 0) return c_south_west; else 
    if (h < 0) return c_north_west; else
    return c_west;
  } else
  
  {
    if (h > 0) return c_south; else 
    if (h < 0) return c_north; else
    return c_centre;
  }
}

int get_x(compass_orientation c)
{ 
  switch (c)
  {
    case c_north:      return 0;
    case c_north_west: return -1;
    case c_west:       return -1;      
    case c_south_west: return -1;
    case c_south:      return 0;
    case c_south_east: return 1;
    case c_east:       return 1;
    case c_north_east: return 1;
    default:           return 0;
  };
}    
    
int get_y(compass_orientation c)
{ 
  switch (c)
  {
    case c_north:      return -1;
    case c_north_west: return -1;
    case c_west:       return 0;      
    case c_south_west: return 1;
    case c_south:      return 1;
    case c_south_east: return 1;
    case c_east:       return 0;
    case c_north_east: return -1;
    default:           return 0;
  };
}    

cursor_name compass_to_cursor(compass_orientation c)
{
  switch (c)
  {
    case c_north: return cursor_resize_v;
    case c_south: return cursor_resize_v;
    case c_east:  return cursor_resize_h;
    case c_west:  return cursor_resize_h;
    
    case c_north_east: return cursor_resize_r;
    case c_south_west: return cursor_resize_r;
    case c_north_west: return cursor_resize_l;
    case c_south_east: return cursor_resize_l; 
  
    case c_centre: return cursor_move;
    
    default: return cursor_normal;
  };
}


