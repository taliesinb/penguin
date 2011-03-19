#include "pmaster.h"
#include "allegro.h"

window_master::window_master(int depth)
: display_delegation_depth(0), buffer_depth(depth)
{
  set_flag(grx_master);
}

void window_master::pre_load()
{ 
  if (buffer_depth == 0) 
  {
    buffer = screen;
  } else
  {
    buffer = create_bitmap_ex(buffer_depth, w()+1, h()+1);
    clear(buffer);
  }
  theme.load();
}      

void window_master::post_unload()
{
  if (buffer_depth) destroy_bitmap(buffer);
  
  theme.unload();
}

void window_master::delegate(base_window* b)
{
  display_delegation_list.push_back(b);
  b->set_flag(sys_delegated);
}

void window_master::begin_display_delegation()
{
  display_delegation_depth++;
}

void window_master::end_display_delegation()
{
  if (--display_delegation_depth == 0)
  {
    flush_display_delegation();
  }
}

void window_master::flush_display_delegation()
{
  for(std::list<base_window*>::iterator i = display_delegation_list.begin(); i != display_delegation_list.end(); i++)
  {
    if ((*i)->flag(sys_delegated)) 
    {
      (*i)->_display();
      (*i)->set_flag(sys_delegated, false);
    }
  }
  
  display_delegation_list.clear();
}



