#include "pmanager.h"
#include "psublim.h"
#include "allegro.h"
#include "pconsole.h"

#define REPEAT_DELAY 200
#define REPEAT_RATE  40
#define EVENT_BUFFER_SIZE 64

void load_key_handler();
void load_mouse_handler();
void unload_key_handler();
void unload_mouse_handler();
void wm_keyboard_callback(int scan, int key);
void wm_keyboard_callback_end();
void wm_mouse_callback(int flags);
void wm_mouse_callback_end();
void key_repeater();
void key_repeater_end();
void add_key_event(int type, int scan, int key);
void add_key_event_end();

struct key_event
{
  int type;
  int scan;
  int key;
  int shifts;
  int other_keys;
} static volatile key_event_buffer[64];

volatile int last_mouse_event;
volatile int key_buffer_lock;
volatile int key_buffer_start;
volatile int key_buffer_end;
volatile int repeat_scan;
volatile int repeat_key;
volatile int last_frame;
volatile int wm_keys_down;
volatile bool close_gui_flag;

BITMAP* window_manager::get_cursor_bmp()
{
  return cursor->get_image();
}

void window_manager::set_cursor_bmp(BITMAP* image)
{
  cursor->set_image(image);
}
  
void window_manager::load_cursor(cursor_name cur, std::string file)
{
  PALETTE dummy_pal;
  BITMAP* image = load_bitmap(file.c_str(), dummy_pal);  
 
  if (flags[sys_active] && get_cursor_bmp() == cursor_library[cur]) 
    set_cursor_bmp(image);

  cursor_library[cur] = image;
}

void window_manager::set_cursor(BITMAP *image)
{
  if (get_cursor_bmp() != image) set_cursor_bmp(image);
}

void window_manager::set_cursor(cursor_name cur)
{
  if (get_cursor_bmp() != cursor_library[cur])
  {
    set_cursor_bmp(cursor_library[cur]);
  }
}

void window_manager::close_gui()
{
  close_gui_flag = true;
}

void close_gui()
{
  close_gui_flag = true;
}

void window_manager::do_gui(base_window& iow, bool (*func)(window_manager&))
{
  io_win = &iow;
  close_gui_flag = false;
  
  if (!cursor_library[cursor_normal])    load_cursor(cursor_normal, "mouse/cursor.bmp");
  if (!cursor_library[cursor_resize_h])  load_cursor(cursor_resize_h, "mouse/resize_h.bmp");
  if (!cursor_library[cursor_resize_v])  load_cursor(cursor_resize_v, "mouse/resize_v.bmp");
  if (!cursor_library[cursor_resize_l])  load_cursor(cursor_resize_l, "mouse/resize_l.bmp");
  if (!cursor_library[cursor_resize_r])  load_cursor(cursor_resize_r, "mouse/resize_r.bmp");
  if (!cursor_library[cursor_hotspot])   load_cursor(cursor_hotspot, "mouse/hand.bmp");
  if (!cursor_library[cursor_illegal])   load_cursor(cursor_illegal, "mouse/nogo.bmp");
  if (!cursor_library[cursor_caret])     load_cursor(cursor_caret, "mouse/caret.bmp");
  if (!cursor_library[cursor_move])      load_cursor(cursor_move, "mouse/move.bmp");
  
  add_child(io_win, 0, false);

  cursor->set_image(cursor_library[cursor_normal]);   
  position_mouse(0,0);
  
  load_key_handler();
  load_mouse_handler();
  load();
  
  draw();
  
  while (!close_gui_flag)
  { 
    while (last_frame == retrace_count);
    last_frame = retrace_count;
   
    poll();                            
    if (func) 
      if (func(*this)) break;  
  }

  unload();    
  unload_key_handler();
  unload_mouse_handler();  
  
  io_win->remove();
}

void window_manager::purge(base_window* win)
{
  if (gloop == win) gloop = 0;
  if (o_target == win) o_target = 0;
  if (keyfocus == win) keyfocus = 0;
  if (target == win) target = 0;
  if (drag_target == win) drag_target = 0;
  
  for (base_window* loop = win->get_child(); loop; loop = loop->next)
    purge(loop);
}

void window_manager::clear_cursor_library()
{
  for (int i = 0; i < cursor_last; i++)
    cursor_library[i] = 0;
}

void window_manager::draw()
{
  display_all();
}

void window_manager::poll()
{
  if (poll_in_action) return;
  poll_in_action = true;

  acquire_bitmap(get_buffer());

  process_keyboard();
  process_mouse();
  
  transmit(poll_ei(++frame));
  if (++caret_blink_count > 60)
  {
    caret_blink = !caret_blink;
    if (keyfocus) keyfocus->event_key_blink();
    caret_blink_count = 0;
  }
                 
  release_bitmap(get_buffer());
  
  if (key[KEY_TILDE] && key_shifts & KB_ALT_FLAG)
  {
    suspend();
    do_console(this, this, console_font ? console_font : ::font);
    resume();
  }
    
  poll_in_action = false;
}

void window_manager::set_keyfocus(base_window* new_win)
{
  base_window* old_win = keyfocus;
  keyfocus = new_win;
  
  if (new_win != old_win)
  {
    if (old_win)
    {
      old_win->set_flag(evt_keyfocus, false);
      old_win->event_key_unfocus(); 
      old_win->transmit(key_unfocus_ei());      
    }
    if (new_win)
    { 
      caret_blink = true;
      caret_blink_count = 0;  
      
      new_win->set_flag(evt_keyfocus);
      new_win->event_key_focus();
      new_win->transmit(key_focus_ei());        
    }
  }
}

window_manager::window_manager()
: io_win(0), keyfocus(0), target(0), drag_target(0), frame(0), hold_t(0), 
  cursor(0), poll_in_action(false), tree_altered(false), 
  caret_blink(false), caret_blink_count(0)
{
  resize(coord_int_max, coord_int_max);

  cursor = new shadowed_masked_image();
  add_child(cursor);

  clear_cursor_library();
}

window_manager::~window_manager()
{ }

void window_manager::suspend()
{
  unload_key_handler();
  unload_mouse_handler();
}

void window_manager::resume()
{
  load_key_handler();
  load_mouse_handler();

  draw();
}

void window_manager::process_keyboard()
{
  int start = key_buffer_start;
  int end = key_buffer_end;

  key_buffer_lock++;
  key_buffer_start = key_buffer_end;
  key_buffer_lock--;

  if (start != end)
  {
    for (int i = start; i != end; i = (i + 1) % EVENT_BUFFER_SIZE)
    {
      switch (key_event_buffer[i].type)
      {
        case 1:
          up_key(scancode_to_ascii(key_event_buffer[i].scan),key_event_buffer[i].scan,key_event_buffer[i].shifts, key_event_buffer[i].other_keys);
          break;
          
        case 0:
          down_key(key_event_buffer[i].key,key_event_buffer[i].scan,key_event_buffer[i].shifts, key_event_buffer[i].other_keys);
          break;

        case 2:
          press_key(key_event_buffer[i].key,key_event_buffer[i].scan,key_event_buffer[i].shifts, key_event_buffer[i].other_keys);
          break;
      }
    }
  }
}

void window_manager::process_mouse()
{
  int o_mouse_x = ::mouse_x;
  int o_mouse_y = ::mouse_y;
  int o_mouse_b = mouse_b;
  
  o_target = target;

  poll_mouse();
  
  bool mouse_moved = (o_mouse_x != ::mouse_x || o_mouse_y != ::mouse_y);
  bool has_moved = mouse_moved;

  if (has_moved) 
  { 
    hold_t = 0;
    cursor->move(::mouse_x, ::mouse_y);
    
  } else has_moved = tree_altered;

  target = io_win->find_window_under(::mouse_x, ::mouse_y);
  tree_altered = false;
    
  if (target)
  {
    target->button_state = mouse_b;
    target->mouse_x = ::mouse_x - target->get_cx();
    target->mouse_y = ::mouse_y - target->get_cy();
  }                
	
	if (o_mouse_x != ::mouse_x) { std::cout << "mouse_x" << ::mouse_x << std::endl; };

  if (mouse_moved) 
  {
    if (!mouse_b)
    {
      if (target && !target->disabled()) 
      {
        if(target) target->event_mouse_move(::mouse_x-o_mouse_x,::mouse_y-o_mouse_y);
        if(target) target->transmit(mouse_move_ei(::mouse_x, ::mouse_y, ::mouse_x-o_mouse_x,::mouse_y-o_mouse_y));
      }
    } else 
    {
      if (drag_target && !drag_target->disabled())
      {
        drag_target->mouse_x = ::mouse_x - drag_target->get_cx();
        drag_target->mouse_y = ::mouse_y - drag_target->get_cy();
        
        if(drag_target) drag_target->event_mouse_drag(::mouse_x-o_mouse_x,::mouse_y-o_mouse_y);
        if(drag_target) drag_target->transmit(mouse_drag_ei(::mouse_x, ::mouse_y, ::mouse_b, ::mouse_x-o_mouse_x,::mouse_y-o_mouse_y));
        if(drag_target) drag_target->set_flag(evt_dragged);
      }
    }
    
    if (tree_altered) target = io_win->find_window_under(::mouse_x, ::mouse_y);
    
  } else
  {
    if (target && mouse_b && mouse_b == o_mouse_b && drag_target == target && !target->disabled())
    {
      if(target) target->event_mouse_hold(hold_t);
      if(target) target->transmit(mouse_hold_ei(::mouse_x, ::mouse_y, ::mouse_b, hold_t));
      
      hold_t++;
      
    } else hold_t = 0;
  }

  if (target != o_target)
  {
    if (o_target && !o_target->disabled())
    {
      if(o_target) o_target->set_flag(evt_mouse_over, false);
      if(o_target) o_target->event_mouse_off();
      if(o_target) o_target->transmit(mouse_off_ei());      
      if(o_target) o_target->mouse_x = -1;
      if(o_target) o_target->mouse_y = -1;
    }

    if (target && !target->disabled())
    {
      if(target) target->set_flag(evt_mouse_over, false);
      if(target) target->event_mouse_on();
      if(target) target->transmit(mouse_on_ei());
    }
  }

  if (int but = (mouse_b & ~o_mouse_b))
  {
    if (target && !target->disabled())
    {
      coord_int mx = ::mouse_x - target->get_cx();
      coord_int my = ::mouse_y - target->get_cy();
      
      target->click_x = mx;
      target->click_y = my;      
      if(target) target->event_mouse_down(but);
      if(target) target->transmit(mouse_down_ei(mx, my, but));
      
      but |= bt_snoop;
      for (gloop = target ? target->get_parent() : 0; gloop; gloop = gloop->get_parent())
      {
        mx += gloop->get_child()->get_ax() + gloop->e_ax();
        my += gloop->get_child()->get_ay() + gloop->e_ay();
        if (gloop->flag(evt_snoop_clicks)) 
        {
          if(gloop) gloop->event_mouse_down(but);
          if(gloop) gloop->transmit(mouse_down_ei(mx, my, but));
        }
      } 
    }

    drag_target = target;
    
  } else if (int but = (o_mouse_b & ~mouse_b))
  {
    if (drag_target && !drag_target->disabled())
    {
      coord_int mx = ::mouse_x - drag_target->get_cx();
      coord_int my = ::mouse_y - drag_target->get_cy();
      coord_int clx = drag_target->click_x;
      coord_int cly = drag_target->click_y;
    
      if(drag_target) drag_target->event_mouse_up(but);
      if(drag_target) drag_target->transmit(mouse_up_ei(mx, my, but, clx, cly));
      if(drag_target) drag_target->set_flag(evt_dragged, false);
      if(drag_target) drag_target->click_x = -1;
      if(drag_target) drag_target->click_y = -1;
      
      but |= bt_snoop;
      for (base_window* gloop = drag_target ? drag_target->get_parent() : 0; gloop; gloop = gloop->get_parent())
      {
        mx += gloop->get_child()->get_ax() + gloop->e_ax();
        clx += gloop->get_child()->get_ax() + gloop->e_ax();
        my += gloop->get_child()->get_ay() + gloop->e_ay();
        cly += gloop->get_child()->get_ay() + gloop->e_ay();
        if (gloop->flag(evt_snoop_clicks)) 
        {
          if(gloop) gloop->event_mouse_up(but);
          if(gloop) gloop->transmit(mouse_up_ei(mx, my, but, clx, cly));
        }
      } 
    }
    drag_target = 0;
  }

  if (has_moved && mouse_b)
  {
    target = io_win->find_window_under(::mouse_x, ::mouse_y);
    tree_altered = false;
  } 
}

void window_manager::down_key(int key, int scan, int shift, int other)
{
  if (keyfocus && !keyfocus->disabled())
  {
    kb_event kb(key, scan, shift, false, false, other);
    if(keyfocus) keyfocus->event_key_down(kb);
    if(keyfocus) keyfocus->transmit(key_down_ei(kb));    

    kb.set_snoop();
    for (gloop = keyfocus ? keyfocus->get_parent() : 0; gloop; gloop = gloop ? gloop->get_parent() : 0)
    {
      if (gloop->flag(evt_snoop_keys)) 
      {
        if(gloop) gloop->event_key_down(kb);
        if(gloop) gloop->transmit(key_down_ei(kb));
      }
    }
  }
  
  if (scan == KEY_F5 && target) target->z_shift(base_window::z_back);
  if (scan == KEY_F6 && target) target->z_shift(base_window::z_lower);
  if (scan == KEY_F7 && target) target->z_shift(base_window::z_raise);
  if (scan == KEY_F8 && target) target->z_shift(base_window::z_front);
}

void window_manager::up_key(int key, int scan, int shift, int other)
{
  if (keyfocus && !keyfocus->disabled())
  {
    kb_event kb(key, scan, shift, false, false, other);
    if(keyfocus) keyfocus->event_key_up(kb);
    if(keyfocus) keyfocus->transmit(key_up_ei(kb));

    kb.set_snoop();
    for (gloop = keyfocus ? keyfocus->get_parent() : 0; gloop; gloop = gloop ? gloop->get_parent() : 0)
    {
      if (gloop->flag(evt_snoop_keys)) 
      {
        if(gloop) gloop->event_key_up(kb);
        if(gloop) gloop->transmit(key_up_ei(kb));
      }
    }
  }
}

void window_manager::press_key(int key, int scan, int shift, int other)
{
  if (keyfocus && !keyfocus->disabled())
  {
    kb_event kb(key, scan, shift, false, true, other);
    if(keyfocus) keyfocus->event_key_down(kb);
    if(keyfocus) keyfocus->transmit(key_down_ei(kb));

    kb.set_snoop();
    for (gloop = keyfocus ? keyfocus->get_parent() : 0; gloop; gloop = gloop ? gloop->get_parent() : 0)
    {
      if (gloop->flag(evt_snoop_keys)) 
      {
        if(gloop) gloop->event_key_down(kb);
        if(gloop) gloop->transmit(key_down_ei(kb));
      }
    }
  }
}

void wm_keyboard_callback(int scan)
{
  int key = scancode_to_ascii(scan);
  if (scan & 0x80) // If this key has been released
  {
    scan ^= 0x80;
  
    wm_keys_down--;
    
    if (!wm_keys_down || ((key && key == repeat_key) || (scan && scan == repeat_scan)))
    {
      remove_int(key_repeater);
      repeat_scan = -1;
      repeat_key = -1;
    }

    add_key_event(1, scan, key);
    
  } else {
  
    add_key_event(0, scan, key);

    if (scan > 0 && scan != repeat_scan)
    {
      if (wm_keys_down) remove_int(key_repeater);
      
      repeat_scan = scan;
      repeat_key = key;
      install_int(key_repeater, REPEAT_DELAY);
    }
    
    wm_keys_down++;
  }
}
END_OF_FUNCTION(wm_keyboard_callback);

void wm_mouse_callback(int flags)
{
  last_mouse_event = flags;
}
END_OF_FUNCTION(wm_mouse_callback);

void load_key_handler()
{
  for (int i = 0; i < EVENT_BUFFER_SIZE; i++)
  {
    key_event_buffer[i].type = 0;
    key_event_buffer[i].scan = 0;
    key_event_buffer[i].key = 0;
  }
  key_buffer_lock = 0;
  key_buffer_start = 0;
  key_buffer_end = 0;
  repeat_scan = 0;
  repeat_key = 0;
  wm_keys_down = 0;

  LOCK_VARIABLE(key_buffer_lock);
  LOCK_VARIABLE(key_buffer_start);
  LOCK_VARIABLE(key_buffer_end);
  LOCK_VARIABLE(repeat_scan);
  LOCK_VARIABLE(repeat_key);
  LOCK_VARIABLE(key_event_buffer);
  LOCK_VARIABLE(wm_keys_down);

  LOCK_FUNCTION(add_key_event);
  LOCK_FUNCTION(wm_keyboard_callback);
  LOCK_FUNCTION(key_repeater);

  set_keyboard_rate(0,0);

  /* hacking allegro */
  keyboard_lowlevel_callback = wm_keyboard_callback;
}

void unload_key_handler()
{
  remove_int(key_repeater);

  keyboard_lowlevel_callback = 0;

  set_keyboard_rate(250, 33);

  remove_keyboard();
  install_keyboard();
}

void load_mouse_handler()
{
  LOCK_FUNCTION(wm_mouse_callback);

  poll_mouse();

  mouse_callback = wm_mouse_callback;
  set_window_close_hook(::close_gui);
}

void unload_mouse_handler()
{
  mouse_callback = 0;
  
  // Error - crashes allegro, for some reason!
  // remove_mouse();
  install_mouse();
  
  set_window_close_hook(0);
}

/* repeat_timer:
 *  Timer callback for doing automatic key repeats.
 */
void key_repeater()
{
  add_key_event(2, repeat_scan, repeat_key);

  install_int(key_repeater, REPEAT_RATE);
}
END_OF_FUNCTION(key_repeater);

void add_key_event(int type, int scan, int key)
{
  key_buffer_lock++;
  if (key_buffer_lock != 1)
  {
    key_buffer_lock--;
    return;
  }

  int c = (key_buffer_end + 1) % EVENT_BUFFER_SIZE;

  if (c != key_buffer_start)
  {
    key_event_buffer[key_buffer_end].type = type;
    key_event_buffer[key_buffer_end].scan = scan;
    key_event_buffer[key_buffer_end].key = key > 0 ? key : 0;
    key_event_buffer[key_buffer_end].shifts = key_shifts;
    key_event_buffer[key_buffer_end].other_keys = wm_keys_down;
    key_buffer_end = c;
  }

  key_buffer_lock--;
}
END_OF_FUNCTION(add_key_event);

