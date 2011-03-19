#include "pwidgets.h"
#include "pgrx.h"
#include "allegro.h"

base_widget& widget_ei::source() const 
{ 
  return dynamic_cast<base_widget&>(*get_origin()); 
}

void window_block::draw(const graphics_context& grx)
{
  grx.rectfill(0,0,w(),h(),col);
}

window_block::window_block(int rr, int gg, int bb) 
: col((rr << 16) | (gg << 8) | bb  | 16777216)
{ }

void window_block::pre_load()
{
  if (col & 16777216)
    col = get_color((col >> 16) & 255, (col >> 8) & 255, col & 255);
}

void window_block::set_colour(int c)
{
  col = c; display();
}

void window_panel::draw(const graphics_context& grx)
{ 
  grx.rectfill(0,0,w(),h(),theme().frame);
}                                                                    

bool window_panel::pos_visible(coord_int x, coord_int y) const
{
  return false;
}

void window_frame::draw(const graphics_context& grx)
{
  grx.draw_frame(0, 0, w(), h(), frame);
  grx.rectfill(2, 2, w()-2, h()-2, theme().frame);
}
                                          
void window_lframe::draw(const graphics_context& grx)
{
  zone text_zone(10, 0, 20+ grx.font_width(text), grx.font_height()); 
  
  grx.render_line(c_west, text.c_str(), text_zone, theme().text, theme().frame);
  grx.rectfill(0,0,9,5,theme().frame);
  grx.rectfill(text_zone.bx+1,0,w(),5,theme().frame);
  
  zone* block = new zone(2, 7, w()-2, h()-2);
  occlude(block, &text_zone);
  
  while (block)
  { 
    grx.rectfill(block, theme().frame);    
    shift_kill(block);
  }
  
  /*  A A A   A A A A C
      A B B   B B B D C
      A B           D C
      A B           D C
      A B           D C
      A D D D D D D D C
      C C C C C C C C C
  */
  
  int a = theme().frame_low;
  int b = theme().frame_white;
  int c = theme().frame_white;
  int d = theme().frame_low;

  grx.hline(0, 5, text_zone.ax-1, a); grx.hline(text_zone.bx+1, 5, w()-1, a);
  grx.vline(0, 5, h()-1, a);
  grx.hline(1, 6, text_zone.ax-1, b); grx.hline(text_zone.bx+1, 6, w()-2, b);
  grx.vline(1, 6, h()-2, b);

  grx.hline(0, h(), w(), c);
  grx.vline(w(), 5, h(), c);
  grx.hline(1, h()-1, w()-1, d);
  grx.vline(w()-1, 6, h()-1, d);
}

void window_main::event_mouse_down(bt_int button)
{
  set_keyfocus();
  
  dragger.down(this);
  if (button & bt_left) z_shift(z_front);
}

void window_main::event_mouse_up(bt_int button)
{
  dragger.up(this);
}

void window_main::event_mouse_drag(coord_int x, coord_int y)
{
  dragger.drag(this, x, y);
}

void window_main::event_key_down(kb_event kd)
{
  switch (kd.get_scan())
  {
    case pk_up: relative_move(0,-5); break;
    case pk_down: relative_move(0,5); break;
    case pk_left: relative_move(-5,0); break;
    case pk_right: relative_move(5,0); break;
  }
}

void window_main::draw(const graphics_context& grx)
{
  grx.rectfill(2, 24, w()-2, h()-2, theme().frame);
  grx.rect(2, 2, w()-2, 23, theme().frame);
  grx.render_line(c_centre, title.c_str(), zone(3,3,w()-3,22), theme().white, theme().bar);
  
  grx.draw_frame(0,0,w(),h(), ft_bevel_out);
}

void base_button::draw(const graphics_context& grx)
{
  grx.draw_frame(0, 0, w(), h(), frame);
  
  grx.render_line(c_centre, text.c_str(), zone(2,2,w()-2,h()-2), theme().text, pressed ? theme().frame_high : theme().frame, -1, pressed ? 1 : 0, pressed ? 1 : 0);
}

void base_button::press()
{
  if (!pressed)
  {
    pressed = true;
    frame = invert_frame(frame);
    display();
  }
}

void base_button::unpress()
{
  if (pressed)
  {
    pressed = false;
    frame = invert_frame(frame);
    display();
  }
}

void window_button::event_mouse_down(bt_int button)
{
  set_keyfocus();
  if (button & bt_left) press();
}

void window_button::event_mouse_off()
{
  unpress();
}

void window_button::event_key_down(kb_event key)
{
  if (key.is_scan(pk_enter) || key.is_scan(pk_space)) press(); 
  else base_widget::event_key_down(key);
}

void window_button::event_key_up(kb_event kd)
{
  if (pressed && kd.is_scan(pk_enter) || kd.is_scan(pk_space))
  {
    click_count++;
    transmit(activate_ei());
    unpress();
  } 
}

void window_button::event_mouse_on()
{
  if (get_click_x() > 0 && get_button_state() & bt_left)
  {
    press();
  } 
}

void window_button::event_mouse_up(bt_int button)
{
  if (button & bt_left && pressed)
  {
    click_count++;
    transmit(activate_ei());
    unpress();
  }
}

void window_icon::draw(const graphics_context& grx)
{
  grx.draw_frame(0, 0, w(), h(), frame);
  
  coord_int width = icon ? icon->w-1 : 0, height = icon ? icon->h-1 : 0, x, y;
  grx.render_backdrop(x, y, zone(2,2,w()-2,h()-2), width, height, theme().frame, c_centre, pressed ? 1 : 0);
  
  if (icon) grx.blit(icon, 0, 0, x, y, icon->w, icon->h);
}

void window_toggle::event_mouse_down(bt_int button)
{
  set_keyfocus();
  if (button & bt_left)
  {
    click_count++;
    if (pressed)
    {  
      unpress();
      transmit(deactivate_ei());
    } else
    {
      press();
      transmit(activate_ei());
    }
  }
}    
           
void window_toggle::event_key_down(kb_event key)
{
  if (key.is_scan(pk_enter) || key.is_scan(pk_space))
  {
    click_count++;
    if (pressed)
    {  
      unpress();
      transmit(deactivate_ei());
    } else
    {
      press();
      transmit(activate_ei());
    }
  } else base_widget::event_key_down(key);
}
  
void window_label::draw(const graphics_context& grx)
{                                                       
  grx.draw_frame(0, 0, w(), h(), frame);
  
  if (multiline)
    grx.render_multiline(text.c_str(), zone(2,2,w()-2,h()-2), theme().frame);
  else
    grx.render_line(c_centre, text.c_str(), zone(2,2,w()-2,h()-2), theme().black, theme().frame);
}

void window_scrollbar::draw(const graphics_context& grx)
{    
  if (top_page) grx.set_mode_normal(); else grx.set_mode_dither();
  
  if (orientation == hv_vertical) grx.rectfill(0, 0, w(), local_ay(bar)-1, theme().black);
  else                            grx.rectfill(0, 0, local_ax(bar)-1, h(), theme().black);
     
  if (bottom_page) grx.set_mode_normal(); else grx.set_mode_dither();          
  
  if (orientation == hv_vertical) grx.rectfill(0, local_by(bar)+1, w(), h(), theme().black);
  else                            grx.rectfill(local_bx(bar)+1, 0, w(), h(), theme().black);
    
  grx.set_mode_normal();
}

void window_scrollbar::pre_load()
{
  switch (orientation)
  {
    case hv_vertical:
    {
      upbut.set_icon(theme().up_arrow);
      downbut.set_icon(theme().down_arrow);
      break;
    }
    case hv_horizontal:
    {
      upbut.set_icon(theme().left_arrow);
      downbut.set_icon(theme().right_arrow);
    }
  }
  update_to_bar();
  
  listen(upbut, LISTENER(window_scrollbar::press_up, mouse_hold_ei));
  listen(downbut, LISTENER(window_scrollbar::press_down, mouse_hold_ei));
}

void window_scrollbar::press_up(const mouse_hold_ei& ei)
{
  if (ei.hold_time == 0 || (ei.hold_time > 25 && ei.hold_time)) decrement();
}

void window_scrollbar::press_down(const mouse_hold_ei& ei)
{
  if (ei.hold_time == 0 || (ei.hold_time > 25 && ei.hold_time)) increment();
}

void window_scrollbar::position_children()
{
  if (orientation == hv_horizontal)
  {
    upbut.place(c_west, 16);
    downbut.place(c_east, 16);
  } else 
  {
    upbut.place(c_north, 16);
    downbut.place(c_south, 16);
  }
}  

void window_scrollbar::event_mouse_hold(int t)
{
  if (t == 0 || (t > 25 && t % 2))
  {
    switch (orientation)
    {
      case hv_horizontal:
      {
        if (get_mouse_x() <= bar.get_ax() && !bottom_page)
        {
          top_page = true; 
          decrement_page();
        } else if (!top_page)
        {
          bottom_page = true;
          increment_page();
        } 
        break;
      } 
      case hv_vertical:
      {
        if (get_mouse_y() <= bar.get_ay() && !bottom_page)
        {
          top_page = true;
          decrement_page();
        } else if (!top_page)
        {
          bottom_page = true;
          increment_page();
        }
        break;
      }
    }
    display();
  }
}

void window_scrollbar::event_mouse_up(bt_int button)
{
  top_page = false;
  bottom_page = false;
  display();
}

void window_scrollbar::move_resize_hook(const zone& old_pos, bool moved, bool resized)
{
  if (resized) update_to_bar();
}

void window_scrollbar::slider_bar::draw(const graphics_context& grx)
{
  grx.draw_frame(0, 0, w(), h(), ft_bevel_out);
  grx.rectfill(2, 2, w()-2, h()-2, theme().frame);
}

void window_scrollbar::slider_bar::event_mouse_drag(coord_int x, coord_int y)
{
  switch (dynamic_cast<window_scrollbar*>(get_parent())->orientation)
  {
    case hv_vertical:
    {
      if (get_ay() + y < get_prev()->get_by()+1) y = (get_prev()->get_by()+1 - get_ay());
      if (get_by() + y > get_next()->get_ay()-1) y = (get_next()->get_ay()-1 - get_by());
      
      relative_move(0, y);
      
      dynamic_cast<window_scrollbar*>(get_parent())->update_from_bar();
      break;
    }
    case hv_horizontal:
    {      
      if (get_ax() + x < get_prev()->get_bx()+1) x = (get_prev()->get_bx()+1 - get_ax());
      if (get_bx() + x > get_next()->get_ax()-1) x = (get_next()->get_ax()-1 - get_bx());
      
      relative_move(x, 0);
      
      dynamic_cast<window_scrollbar*>(get_parent())->update_from_bar();
      break;
    }
  }  
}

void window_scrollbar::update_value(int new_value)
{
  int old_value = value;

  if (new_value > max) new_value = max;
  if (new_value < min) new_value = min;
  value = new_value;
  
  update_to_bar();
  
  if (value != old_value) transmit(scroll_ei(value, value-old_value));    
}

void window_scrollbar::update_to_bar()
{  
  if (autosize) bar_width = int((((orientation == hv_horizontal) ? w() : h()) - 34) / ((float(max - min) / page_step) + 1));   
 
  int max_length = ((orientation == hv_horizontal) ? w() : h()) - bar_width - 34; 
  float percent = float(value - min) / float(max - min);
  coord_int pos = int(17+(percent*max_length));
 
  if (bar_width < 5) bar_width = 5;
 
  if (orientation == hv_horizontal) bar.place(pos,0,pos+bar_width,h());
  else bar.place(0,pos,w(),pos+bar_width);
}

void window_scrollbar::update_from_bar()
{
  int old_value = value;
  
  int max_length = ((orientation == hv_horizontal) ? w() : h()) - bar_width - 34; 
  float percent = float((orientation == hv_horizontal) ? (bar.get_ax()-17) : (bar.get_ay()-17)) / float(max_length); 
  value = int(percent * (max - min) + min);
  
  transmit(scroll_ei(value, value-old_value));
}

void window_checkbox::draw(const graphics_context& grx) 
{                                  
  coord_int rx, ry;      
  grx.render_backdrop(rx, ry, zone(0,0,12,h()), 12, 12, theme().frame);
  
  grx.draw_frame(rx, ry, rx+12, ry+12, ft_bevel_in);
  grx.rectfill(rx+2, ry+2, rx+10, ry+10, theme().white);
  
  if (state)
  {
    int col = theme().black;
    
    grx.hline(rx+9,ry+3,rx+9,col);
    grx.hline(rx+8,ry+4,rx+9,col);
    grx.hline(rx+7,ry+5,rx+9,col); grx.hline(rx+3,ry+5,rx+3,col);
    grx.hline(rx+6,ry+6,rx+8,col); grx.hline(rx+3,ry+6,rx+4,col);
    grx.hline(rx+3,ry+7,rx+7,col);
    grx.hline(rx+4,ry+8,rx+6,col);
    grx.hline(rx+5,ry+9,rx+5,col);
  } 
  
  grx.render_line(c_west, text.c_str(), zone(13,0,w(),h()), theme().black, theme().frame);
}

void window_checkbox::event_key_down(kb_event key)
{
  if (key.is_scan(pk_enter) || key.is_scan(pk_space))
  {
    state = !state;
    display();
    if (state) transmit(activate_ei());
    else transmit(deactivate_ei());
  } else base_widget::event_key_down(key);
}

void window_checkbox::event_mouse_down(bt_int button)
{
  set_keyfocus();

  if (state)
  {
    state = false;
    display();
    transmit(deactivate_ei());
  } else 
  {
    state = true;
    display();
    transmit(activate_ei());
  }   
}

void window_checkbox::set_value(int val)
{
  if (val && !state)
  {
    state = true;
    display();
    transmit(activate_ei());
  } else if (!val && state)
  {
    state = false;
    display();
    transmit(deactivate_ei());
  }
}

void window_radiobutton::event_key_down(kb_event key)
{
  if (key.is_scan(pk_enter) || key.is_scan(pk_space))
  {
    set_value(1);
  } else base_widget::event_key_down(key);
}

void window_radiobutton::event_mouse_down(bt_int button)
{
  set_keyfocus();
  set_value(1);  
}  

void window_radiobutton::set_value(int val)
{
  if (val)
  {
    if (!state)
    {
      state = true;
      
      for (base_window* loop = most_prev(); loop; loop = loop->get_next())
        if (loop != this && dynamic_cast<window_radiobutton*>(loop))
          dynamic_cast<window_radiobutton*>(loop)->set_value(0);
      
      display();
      transmit(activate_ei());
    }
  } else 
  {
    if (state)
    {
      state = false;
      
      display();
      transmit(deactivate_ei());
    }
  }
}

void window_radiobutton::draw(const graphics_context& grx) 
{    
  /* white:
     2: 4-7, 10
     3: 3-8, 10
     4: 2-9, 11
     5: 2-9, 11
     6: 2-9, 11
     7: 2-9, 11
     8: 3-8, 10
     9: 4-7, 10
     10: 2-3, 8-9
     11: 4-7 
     
     dark:
     0: 4-7
     1: 2-3, 8-9
     2: 1
     3: 1
     4: 0
     5: 0
     6: 0
     7: 0
     8: 1
     9: 1
     
     black:
     1: 4-7
     2: 2-3, 8-9
     3: 2
     4: 1
     5: 1
     6: 1
     7: 1 
     8: 2
  
     grey:
     0: 0-3,8-11
     1: 0-1,10-11
     2: 0,11
     3: 0,9,11
     4: 10
     5: 10
     6: 10
     7 :10
     8: 0,9,11
     9: 0,3-4,8-9,11
     10:0-1,4-7,10-11
     11:0-3,8-11
   */       
                                                  
  coord_int rx, ry;      
  grx.render_backdrop(rx, ry, zone(0,0,0+12,h()), 11, 11, theme().frame);

  int col = theme().frame;
  grx.hline(rx+0,ry+0,rx+3,col); grx.hline(rx+8,ry+0,rx+11,col);
  grx.hline(rx+0,ry+1,rx+1,col); grx.hline(rx+10,ry+1,rx+11,col);
  grx.putpixel(rx+0,ry+2,col); grx.putpixel(rx+11,ry+2,col);
  grx.putpixel(rx+0,ry+3,col); grx.putpixel(rx+9,ry+3,col); grx.putpixel(rx+11,ry+3,col); 
  grx.putpixel(rx+10,ry+4,col);
  grx.putpixel(rx+10,ry+5,col);
  grx.putpixel(rx+10,ry+6,col);
  grx.putpixel(rx+10,ry+7,col);
  grx.putpixel(rx+0,ry+8,col); grx.putpixel(rx+9,ry+8,col); grx.putpixel(rx+11,ry+8,col);
  grx.putpixel(rx+0,ry+9,col); grx.putpixel(rx+11,ry+9,col); 
  grx.hline(rx+2,ry+9,rx+3,col); grx.hline(rx+8,ry+9,rx+9,col); 
  grx.hline(rx+0,ry+10,rx+1,col); grx.hline(rx+4,ry+10,rx+7,col);grx.hline(rx+10,ry+10,rx+11,col);
  grx.hline(rx+0,ry+11,rx+3,col); grx.hline(rx+8,ry+11,rx+11,col);  
  
  col = theme().frame_white;
  grx.hline(rx+4,ry+2,rx+7,col); grx.putpixel(rx+10,ry+2,col);
  grx.hline(rx+3,ry+3,rx+8,col); grx.putpixel(rx+10,ry+3,col);
  grx.hline(rx+2,ry+4,rx+9,col); grx.putpixel(rx+11,ry+4,col);
  grx.hline(rx+2,ry+5,rx+9,col); grx.putpixel(rx+11,ry+5,col);
  grx.hline(rx+2,ry+6,rx+9,col); grx.putpixel(rx+11,ry+6,col);
  grx.hline(rx+2,ry+7,rx+9,col); grx.putpixel(rx+11,ry+7,col);
  grx.hline(rx+3,ry+8,rx+8,col); grx.putpixel(rx+10,ry+8,col);
  grx.hline(rx+4,ry+9,rx+7,col); grx.putpixel(rx+10,ry+9,col);
  grx.hline(rx+2,ry+10,rx+3,col); grx.hline(rx+8,ry+10,rx+9,col);
  grx.hline(rx+4,ry+11,rx+7,col); 
  
  col = theme().frame_low;
  grx.hline(rx+4,ry+0,rx+7,col);
  grx.hline(rx+2,ry+1,rx+3,col); grx.hline(rx+8,ry+1,rx+9,col);
  grx.putpixel(rx+1,ry+2,col);
  grx.putpixel(rx+1,ry+3,col);
  grx.putpixel(rx+0,ry+4,col);
  grx.putpixel(rx+0,ry+5,col);
  grx.putpixel(rx+0,ry+6,col);
  grx.putpixel(rx+0,ry+7,col);
  grx.putpixel(rx+1,ry+8,col);
  grx.putpixel(rx+1,ry+9,col);
  
  col = theme().frame_black;
  grx.hline(rx+4,ry+1,rx+7,col);
  grx.hline(rx+2,ry+2,rx+3,col); grx.hline(rx+8,ry+2,rx+9,col);
  grx.putpixel(rx+2,ry+3,col);
  grx.putpixel(rx+1,ry+4,col);
  grx.putpixel(rx+1,ry+5,col);
  grx.putpixel(rx+1,ry+6,col);
  grx.putpixel(rx+1,ry+7,col);
  grx.putpixel(rx+2,ry+8,col);
   
/*   DOT:
    
     5: 5-6
     6: 4-7
     7: 4-7
     8: 5-6
*/  
  
  if (state)
  {
    grx.hline(rx+5,ry+4,rx+6,col);
    grx.hline(rx+4,ry+5,rx+7,col);
    grx.hline(rx+4,ry+6,rx+7,col);
    grx.hline(rx+5,ry+7,rx+6,col);
  } 
  
  grx.render_line(c_west, text.c_str(), zone(13, 0, w(), h()), theme().black, theme().frame);
}

void window_textbox::draw(const graphics_context& grx)
{                                                       
  grx.draw_frame(0, 0, w(), h(), ft_bevel_in);
  
  if (multiline)
  {
    grx.render_multiline(text, zone(2,2,w()-2-(vscroll.flag(vis_visible)?16:0),h()-2), theme().pane, (flag(evt_keyfocus) && get_manager()->show_caret()) ? edit_pos : std::string::npos, line, wordwrap);
  } else
  {
    grx.render_line(c_west, text.c_str(), zone(2,2,w()-2,h()-2), theme().text, theme().pane, (flag(evt_keyfocus) && get_manager()->show_caret()) ? edit_pos : std::string::npos);
  }
}

void window_textbox::event_key_down(kb_event kb)
{
  char c = kb.get_char();
  zone text_zone(2,2,w()-2,h()-2);

  if (c == 8)
  {
    if (edit_pos > 0)
    {
      text.erase(edit_pos-1, 1);
      edit_pos--;
    }
    
  } else if (c == 13)
  {
    if (multiline)
    {
      text.insert(edit_pos, 1, '\n');
      edit_pos++;
    }    
  } else
  {
    switch (kb.get_scan())
    {
      case pk_del:
      {
        if (edit_pos < text.length()) text.erase(edit_pos, 1);\
        
      } break;
        
      case pk_left:
      {
        if (edit_pos > 0) edit_pos--;        
        
      } break;
        
      case pk_right:
      {
        if (edit_pos < text.length()) edit_pos++;
        
      } break;
       
      case pk_home:
      {
        if (multiline)
        {
          int line = char_to_line(theme().font, text, text_zone, edit_pos);
          if (line > 1) edit_pos = line_to_char(theme().font, text, text_zone, line);
          else edit_pos = 0;
          
        } else edit_pos = 0;                  
        
      } break;
      
      case pk_pgup:
      {
        if (multiline && vscroll.visible())
        {
          int cur_line = char_to_line(theme().font, text, text_zone, edit_pos);
          cur_line -= vscroll.get_page_step();
          if (cur_line < 1) cur_line = 1;
          edit_pos = line_to_char(theme().font, text, text_zone, cur_line);
        }
      } break;
      
      case pk_pgdn:
      {
        if (multiline && vscroll.visible())
        {
          int cur_line = char_to_line(theme().font, text, text_zone, edit_pos);
          cur_line += vscroll.get_page_step();
          if (cur_line > lines) cur_line = lines;
          edit_pos = line_to_char(theme().font, text, text_zone, cur_line);
        }
      } break;
       
      case pk_up:
      {
        if (multiline)
        {
          int line = char_to_line(theme().font, text, text_zone, edit_pos);
          
          if (line > 1)
          {
            int start = line_to_char(theme().font, text, text_zone, line);           
            int column = edit_pos - start;
            int last_line = line_to_char(theme().font, text, text_zone, line-1); 
            edit_pos = last_line + column;
            if (edit_pos >= start) edit_pos = start-1;
          }        
        }  
      
      } break;
      
      case pk_tab:
      {
        int pos = edit_pos;
        if (multiline) pos -= first_char_in_line(theme().font, text, text_zone, edit_pos);
        
        do
        {
          text.insert(edit_pos, 1, ' ');
          edit_pos++, pos++;
        } while (pos % 4);
        
      } break;
      
      case pk_down:
      {
        if (multiline)
        {
          int line = char_to_line(theme().font, text, text_zone, edit_pos);
          
          if (line < lines)
          {
            int start = line_to_char(theme().font, text, text_zone, line);           
            int column = edit_pos - start;
            int next_line = line_to_char(theme().font, text, text_zone, line+1); 
            int chars = chars_in_line(theme().font, text, text_zone, line+1);
            edit_pos = next_line + column;
            if (edit_pos > next_line + chars) edit_pos = next_line + chars;
          }        
        }  
      
      } break;
      
      case pk_end:
      {
        if (multiline)
        {
          int line = char_to_line(theme().font, text, text_zone, edit_pos);
          if (line < lines) edit_pos = line_to_char(theme().font, text, text_zone, line+1)-1;
          else edit_pos = text.length();
          
        } else edit_pos = text.length();        
        
      } break;
      
      default:
      {
        if (c)
        {
          text.insert(edit_pos, 1,  c);
          edit_pos++;
        }      
      }
    }    
  } 
  
  if (edit_pos > text.length()) edit_pos = text.length(); 
  if (edit_pos < 0) edit_pos = 0;
 
  delegate_displays();
          
  get_manager()->set_caret(true);
  change_text();
  display();
  
  undelegate_displays();
  
  transmit(string_input_ei(text));  
}

void window_textbox::event_mouse_down(bt_int button)
{
  set_keyfocus();
  
  zone text_zone(2, 2, w()-2-(vscroll.flag(vis_visible)?16:0), h()-2);
  
  if (multiline) 
  {
    edit_pos = find_multiline_index(theme().font, text, text_zone, get_mouse_x(), get_mouse_y(), line, wordwrap);
  } else
  {
    edit_pos = find_line_index(theme().font, c_west, std::string(text).c_str(), text_zone, get_mouse_x());
  }
  
  get_manager()->set_caret(true);
  
  display();
}

void window_textbox::change_text()
{
  if (flag(sys_active) && multiline)
  {
    int max_lines = (h()-7) / text_height(theme().font);
    lines = lines_in_multiline(theme().font, text, zone(2,2,w()-2-(vscroll.flag(vis_visible)?16:0),h()-2), wordwrap);
  
    if (lines > max_lines)              
    {
      vscroll.show();
      
      vscroll.set_max(lines - max_lines);
      vscroll.set_page_step(max_lines);
      
      int cline = char_to_line(theme().font, text, zone(2,2,w()-2-(vscroll.flag(vis_visible)?16:0),h()-2), edit_pos); 
      if (cline <= line) vscroll.set_value(cline-1);
      else if (cline > line + max_lines) vscroll.set_value(cline);   
      else vscroll.display();
      
    } else if (lines < max_lines) 
    {
      line = 0;
      vscroll.hide();
    }
  }
}

void window_textbox::pre_load()
{
  if (multiline) 
  {
    listen(vscroll, LISTENER(window_textbox::scrolled, scroll_ei));
    vscroll.hide();
  }
}

void window_textbox::post_load()
{
  change_text();
}

void window_textbox::position_children()
{  
  if (multiline)
  {                                      
    vscroll.place(c_east, normal_size);
    change_text();
  }
}  

void window_listbox::draw(const graphics_context& grx)
{ 
  grx.draw_frame(0, 0, w(), h(), ft_bevel_in);
  
  clipper clip(grx, 2, 2, w()-2, h()-2);
  
  coord_int ypos = 2;
  int txt_h = text_height(theme().font);        
  for (int n = line; n < list.size() && ypos < h()-2; n++)
  {
    if (n == cur_sel) grx.render_line(c_west, list[n].c_str(), zone(2,ypos,w()-2,ypos+txt_h), theme().pane, theme().bar);
    else              grx.render_line(c_west, list[n].c_str(), zone(2,ypos,w()-2,ypos+txt_h), theme().text, theme().pane);
      
    ypos += txt_h + 1;
  } 
  
  if (ypos <= h()-2) grx.rectfill(2,ypos,w()-2,h()-2,theme().pane);
}  
    
void window_listbox::add_item(const std::string& s)
{
  list.push_back(s);
  change_list();
} 

void window_listbox::remove_item(const std::string& s)
{
  for (std::vector<std::string>::iterator r = list.begin(); r != list.end(); r++)
  {
    if (*r == s) 
    {
      list.erase(r);
      
      if (cur_sel == r-list.begin()) 
      {
        cur_sel = -1;
        transmit(selection_change_ei("", -1));
      }
      change_list();      
      break;
    }
  }
}

void window_listbox::remove_item(int n)
{
  if (n >= 0 && n < list.size())
  {
    list.erase(list.begin() + n);
    if (n == cur_sel) 
    {
      cur_sel = -1;
      transmit(selection_change_ei("", -1));
    }
    change_list();
  }
}

void window_listbox::clear_list()
{
  list.clear();
  cur_sel = -1;
  transmit(selection_change_ei("", -1));
  change_list();
}

void window_listbox::change_list()
{
  if (!flag(sys_active)) return;

  int max_lines = (h()-4) / (text_height(theme().font)+1);
  
  if (!vscroll.flag(vis_visible) && list.size() > max_lines)              
  {
    vscroll.set_max(list.size() - max_lines);
    vscroll.set_page_step(max_lines);
    vscroll.show();
    line = 0;
    
    
  } else if (list.size() < max_lines)
  {
    vscroll.hide();  
  }
  
      
  display();
}

void window_listbox::pre_load()
{
  vscroll.hide();
  listen(vscroll, LISTENER(window_listbox::scrolled, scroll_ei));
}

void window_listbox::post_load()
{ 
  if (list.size()) change_list();
}

void window_listbox::select(int n)
{
  if (n >= 0 && n < list.size())
  {
    cur_sel = n;
    display();
    
    transmit(selection_change_ei(list[cur_sel], cur_sel));
  } else
  {
    cur_sel = -1;
    display();
    
    transmit(selection_change_ei("", -1));
  }
}

void window_listbox::set_value(int v)
{
  select(v);
}

void window_listbox::set_text(const std::string& str)
{
  for (int n = 0; n < list.size(); n++)
  {
    if (list[n] == str)
    {
      select(n);
      return;
    }
  }
  
  cur_sel = list.size();
  list.push_back(str);    
  change_list();  
  
  transmit(selection_change_ei(list[cur_sel], cur_sel));
}

std::string window_listbox::get_item(int n)
{ 
  return (n >= 0 && n < list.size()) ? list[n] : ""; 
}

void window_listbox::position_children()
{                                        
  vscroll.place(c_east, normal_size);
  change_list();
}  

void window_listbox::event_mouse_down(bt_int button)
{
  set_keyfocus();
  
  if (!(button & bt_snoop))
  {   
    int index = (get_click_y()-2) / (text_height(theme().font)+1) + line;
    
    if (index >= 0 && index < list.size() && index != cur_sel)
    {
      cur_sel = index;
      display();  
      
      transmit(selection_change_ei(list[cur_sel], cur_sel));
    }
  }
}  

void window_listbox::event_key_down(kb_event key)
{
  if (key.is_scan(pk_up))
  {
    if (cur_sel > 0) 
    {
      cur_sel--;
      
      if (cur_sel >= line+vscroll.get_page_step()) vscroll.set_value(cur_sel - vscroll.get_page_step() + 1);
      else if (cur_sel < line) vscroll.set_value(cur_sel);
      else display();
      
      transmit(selection_change_ei(list[cur_sel], cur_sel));
    }    
  } else if (key.is_scan(pk_down))
  {
    if (cur_sel < list.size()-1) 
    {
      cur_sel++;
      
      if (cur_sel >= line+vscroll.get_page_step()) vscroll.set_value(cur_sel - vscroll.get_page_step() + 1);
      else if (cur_sel < line) vscroll.set_value(cur_sel);
      else display();
      
      transmit(selection_change_ei(list[cur_sel], cur_sel));
    }
  } else if (key.is_scan(pk_enter))
  {
    transmit(selection_change_ei(list[cur_sel], cur_sel));
  
  } else if (char c = key.get_char())
  {
    for (int n = 0; n < list.size(); n++)
    {
      if (list[n][0] == c)
      { 
        cur_sel = n;
        display();
        
        transmit(selection_change_ei(list[cur_sel], cur_sel));
        break;
      }
    }
  } else base_widget::event_key_down(key);
}

window_image::window_image(std::string file, display_method m)
: image(0), method(m)
{
  PALETTE pal;
  image = load_bmp(file.c_str(), pal);
    ASSERT(image);
}

window_image::window_image(BITMAP* bmp, display_method m)
: image(0), method(m)
{
  if (bmp)
  {
    image = create_bitmap_ex(bitmap_color_depth(bmp), bmp->w, bmp->h);
    if (image) blit(bmp, image, 0, 0, 0, 0, bmp->w, bmp->h);
  }
}

window_image::window_image()
: image(0), method(centre)
{
}

window_image::~window_image()
{
  if (image) destroy_bitmap(image);
}

void window_image::pre_load()
{
  if (BITMAP* temp = image)
  {
    image = 0;
    set_image(temp);
    destroy_bitmap(temp);
  }
}

void window_image::set_image(BITMAP* new_image)
{
  if (flag(sys_loading) || flag(sys_loaded))
  {
    if (new_image)
    {
      if (image) destroy_bitmap(image); 
      image = create_bitmap_ex(get_depth(), new_image->w, new_image->h);
      clear_to_color(image, theme().frame);
      masked_blit(new_image, image, 0, 0, 0, 0, new_image->w, new_image->h);   
      
    } else
    {
      if (image) destroy_bitmap(image);
      image = 0;
    }
  } else
  {
    if (image) destroy_bitmap(image);
    image = new_image;
  }
}

void window_image::draw(const graphics_context& grx)
{
  if (image)
  {     
    switch (method)
    {
      case centre:
      {
        coord_int x, y;
        grx.render_backdrop(x, y, zone(0,0,w(),h()), image->w-1, image->h-1, theme().frame);
        grx.blit(image, x, y);
        break;
      } 
      
      case tile:
      {
        for (int y = 0; y < h(); y += image->h)
          for (int x = 0; x < w(); x += image->w)
            grx.blit(image, x, y);            
        break;
      } 
      
      case stretch:
      {
        grx.stretch_blit(image, 0, 0, image->w, image->h, 0, 0, w()+1, h()+1);
        break;
      }
    }      
  } else
  {
    grx.rectfill(0,0,w(),h(), theme().frame);
  }
}

coord_int window_image::normal_w() const 
{ 
  return image ? image->w-1 : 0; 
}

coord_int window_image::normal_h() const
{ 
  return image ? image->h-1 : 0; 
}

void window_image::load(BITMAP* bmp, display_method m)
{
  if (bmp && !flag(sys_loaded))
  {
    if (image) destroy_bitmap(image); 
    image = create_bitmap_ex(bitmap_color_depth(bmp), bmp->w, bmp->h);
    blit(bmp, image, 0, 0, 0, 0, bmp->w, bmp->h);
  
  } else
  {
    set_image(bmp);
  }
  
  method = m;
  display();
}

void window_image::load(std::string file, display_method m)
{
  PALETTE pal;
  BITMAP* bmp = load_bitmap(file.c_str(), pal);

  if (bmp && !flag(sys_loaded))
  {
    if (image) destroy_bitmap(image); 
    image = bmp;
  
  } else
  {
    set_image(bmp);
  }
  
  method = m;
  display();
}

void window_image::set_method(display_method m)
{
  method = m;
  display();
}

void window_canvas::post_load()
{
  buffer = create_bitmap_ex(get_depth(), w()-3, h()-3);
  clear_to_color(buffer, theme().pane);
}

void window_canvas::post_unload()
{                        
  destroy_bitmap(buffer);
}

void window_canvas::clear()
{
  clear_to_color(buffer, theme().pane);
  display();
}

void window_canvas::draw(const graphics_context& grx)
{
  coord_int x, y;
  grx.render_backdrop(x, y, zone(0,0,w(),h()), buffer->w-1, buffer->h-1, theme().pane, c_north_west);
  grx.blit(buffer, x, y);

}

const graphics_context window_canvas::context()
{
  return graphics_context(buffer, 0, 0, theme());
} 

void window_pane::draw(const graphics_context& grx)
{
  grx.draw_frame(0,0,w(),h(),ft_bevel_in);
  grx.rectfill(2,2,w()-2,h()-2,theme().white);
}

void window_pane::content_resize(const move_resize_ei& ei)
{
  if (ei.resized) update_content();
}

void window_pane::pre_load()
{
  corner_block.set_colour(theme().frame);
  corner_block.hide();

  add_child(content, 0, false);
  listen(hscroll, LISTENER(window_pane::update_hscroll, scroll_ei));
  listen(vscroll, LISTENER(window_pane::update_vscroll, scroll_ei));
  listen(content, LISTENER(window_pane::content_resize, move_resize_ei));
  listen(*this, LISTENER(window_pane::content_resize, move_resize_ei));
}

void window_pane::post_load()
{
  update_content();
  position_children();
}

void window_pane::update_hscroll(const scroll_ei& ei)
{
  content.move(-ei.value, content.get_ay());
}

void window_pane::update_vscroll(const scroll_ei& ei)
{
  content.move(content.get_ax(), -ei.value);
}

void window_pane::position_children()
{ 
  vscroll.place(e_w()-vscroll.normal_w(), 0, e_w(), e_h()- hvis * hscroll.h());
  hscroll.place(0, e_h()-hscroll.normal_h(), e_w()- vvis * vscroll.w(), e_h());
  corner_block.place(e_w()-vscroll.normal_w()+1, e_h()-hscroll.normal_h()+1, e_w(), e_h());
  
  forget(content, LISTENER(window_pane::content_resize, move_resize_ei));
  if (hstate == fixed) content.set_w(e_w() - vvis * (vscroll.w()+1));
  if (vstate == fixed) content.set_h(e_h() - hvis * (hscroll.h()+1));
  listen(content, LISTENER(window_pane::content_resize, move_resize_ei));
}

void window_pane::update_content()
{
  coord_int max_w = content.w();
  coord_int max_h = content.h();

  bool h_show = max_w > e_w();
  bool v_show = max_h > e_h();

  if (h_show) v_show = max_h > e_h() - hscroll.h();
  if (v_show) h_show = max_w > e_w() - vscroll.w();
  
  if (hstate == none || hstate == fixed) h_show = false;
  if (vstate == none || vstate == fixed) v_show = false; 
  
  coord_int scr_w = e_w() - v_show * (vscroll.w()+1);
  coord_int scr_h = e_h() - h_show * (hscroll.h()+1); 
   
  if (h_show)
  {
    hscroll.set_page_step(scr_w);
    hscroll.set_max(max_w-scr_w); 
    hscroll.show();  
    hvis = true;
  } else 
  {
    if (content.get_ax()) content.move(0, content.get_ay());
    if (hstate != visible) 
    {
      hscroll.hide();
      hvis = false;
    } else hscroll.set_max(0);      
  }
  
  if (v_show) 
  {
    vscroll.set_page_step(scr_h);
    vscroll.set_max(max_h-scr_h); 
    vscroll.show();
    vvis = true;
  } else
  {
    if (content.get_ay()) content.move(content.get_ax(), 0);
    if (vstate != visible) 
    { 
      vscroll.hide(); 
      vvis = false; 
    }
    else vscroll.set_max(0);  
    
  }
  
  if (h_show && v_show) corner_block.show(); else corner_block.hide(); 
}

window_pane::window_pane(base_window& c, scrollstate h, scrollstate v)
: content(c), hscroll(hv_horizontal), vscroll(hv_vertical), hstate(h), vstate(v), hvis(true), vvis(true)
{
  add_child(hscroll);
  add_child(vscroll);
  add_child(corner_block);
}
 
