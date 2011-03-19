#ifndef PWIDGETS_H
#define PWIDGETS_H

#include <string>

#include "pbasewin.h"
#include "pmanager.h"
#include "pgrx.h"

/* Warning: This file is in rapid development and is thus in a constant state
 * of change. For this reason, it has not been commented or structured, as such
 * comments would become out-of-date very quickly and end up causing problems.
 * After I have implemented some more basic widgets, such as drop-down menus,
 * choice-boxes, tree- and table-views, I will document this module fully.
 */
class window_placer : public base_window
{
  public:
  
    window_placer(coord_int width =0, coord_int height =0)
    { hide(); resize(width, height); }
};

class window_block : public base_window
{
  private:
  
    void draw(const graphics_context& grx);    
    unsigned int col;
    
  public:
  
    using base_window::add_child;
  
    window_block(int rr =0, int gg =0, int bb =0);
        
    void pre_load();
    void set_colour(int c);
};  

class window_container : public base_window
{    
  public:
  
    using base_window::add_child;

    window_container()
    { }
};

class window_panel : public window_container
{
  private:
  
    bool pos_visible(coord_int x, coord_int y) const;
    void draw(const graphics_context& grx);
};

class window_frame : public window_container
{
  private:
  
    void draw(const graphics_context& grx);

  protected:

    frame_type frame;
    
  public:
    
    coord_int e_ax() const { return  2; } // Beginning of estate
    coord_int e_ay() const { return  2; }
    coord_int e_bx() const { return  w()-2; } // End of estate
    coord_int e_by() const { return  h()-2; }
  
    window_frame(frame_type f =ft_bevel_in)
    : frame(f)
    { }
};

class window_lframe : public window_container
{
  private:
  
    void draw(const graphics_context& grx);
  
  protected:
  
    std::string text;
    
  public:
  
    coord_int e_ax() const { return  3; }
    coord_int e_ay() const { return  15; }    
    coord_int e_bx() const { return  w()-3; } 
    coord_int e_by() const { return  h()-3; }
  
    window_lframe(const std::string& s) : text(s)
    { }    
    
    void set_text(const std::string& s) 
    {
      text = s;
      display();
    }
    
    std::string get_text()
    {
      return text;
    }
};          

class window_main : public window_frame
{
  protected:

    std::string title;
    
    drag_helper dragger;
    
    void event_mouse_down(bt_int button);
    void event_mouse_up(bt_int button);
    void event_key_down(kb_event kd);
    void event_mouse_drag(coord_int x, coord_int y);   

  public:

    window_main(const std::string& t ="", coord_int min_x =100, coord_int min_y =25) 
    : title(t), dragger(min_x, min_y)
    { }
  
    coord_int e_ax() const { return  3; } // Beginning of estate
    coord_int e_ay() const { return  24; }
    coord_int e_bx() const { return  w()-3; } // End of estate
    coord_int e_by() const { return  h()-3; }
  
    void draw(const graphics_context& grx);
};

class base_widget : public base_window
{
  private:
  
    // Should be set to give a description of the purpose of the widget. 
    std::string description; // Tool-tips, status-bars, etc, will make use of this.

  public:
  
    // Virtual functions to set the data of the widget.
    virtual void set_value(int value) { }       
    virtual void set_text(const std::string& value) { }      
                             
    // Virtual functions to return the data of the widget                               
    virtual int get_value() const   { return  0; }
    virtual std::string get_text() const { return  ""; }
    virtual void reset() { } // Set the widget back to its 'default' state
    
    // Change/retrieve the description of this widget's purpose 
    void set_description(const std::string& s) { description = s; }
    std::string get_description() { return description; }
    
    // Should be passed true by derived classes if widget can hold the keyboard focus
    base_widget(bool focusable) // Focus isn't properly implemented yet
    { } 
};

// Event-info classes that widget-derived classes emit
    
struct widget_ei : public event_info
{ DEFINE_EI(widget_ei, event_info)

  base_widget& source() const;
};

struct input_ei : public widget_ei
{ DEFINE_EI(input_ei, widget_ei)

};

struct activate_ei : public input_ei
{ DEFINE_EI(activate_ei, input_ei)

};

struct deactivate_ei : public input_ei
{ DEFINE_EI(deactivate_ei, input_ei)

};

struct int_input_ei : public input_ei
{ DEFINE_EI(int_input_ei, input_ei);

  const int value;
  
  int_input_ei(int v) : value(v)
  { }  
};

struct string_input_ei : public input_ei
{ DEFINE_EI(string_input_ei, input_ei)
  
  const std::string text;
  
  string_input_ei(const std::string& t) : text(t)
  { }
};


class base_button : public base_widget
{
  private:
  
    void draw(const graphics_context& grx);
 
  protected:

    std::string text;
    bool pressed;
    unsigned int click_count;
    frame_type frame;

  public:
  
    base_button(const std::string& t ="", frame_type f =ft_button_out)
    : base_widget(true), text(t), pressed(false), click_count(0), frame(f)
    { }
   
    void set_text(const std::string& s)
    {
      text = s;
      display();
    }
  
    std::string get_text() const { return text; }
  
    void set_pressed(bool p)
    {
      pressed = p;
    }
  
    unsigned int get_click_count() const { return click_count; }
  
    bool is_pressed() const { return pressed; }
  
    void press();
    void unpress(); 
    
    coord_int normal_w() const { return 75; }
    coord_int normal_h() const { return 23; }
};

class window_button : public base_button
{
  protected:

    void event_key_down(kb_event kd);
    void event_key_up(kb_event kd);
    void event_mouse_down(bt_int button);
    void event_mouse_up(bt_int button);
    void event_mouse_off();
    void event_mouse_on();

  public:
  
    window_button(const std::string& t ="")
    : base_button(t)
    { }
};

class window_icon : public window_button
{
  private:
  
    void draw(const graphics_context& grx);

  protected:
  
    BITMAP* icon;
    
  public:
  
    window_icon(BITMAP* i =0) : icon(i)
    { }
    
    void set_icon(BITMAP* i) { icon = i; display(); }
    BITMAP* get_icon() const { return icon; }
};    
  

class window_toggle : public base_button
{
  protected:
  
    void event_key_down(kb_event kd);
    void event_mouse_down(bt_int button);
                                           
  public:
  
    window_toggle(const std::string& t ="")
    : base_button(t, ft_bevel_out)
    { }
        
    int get_value() const { return is_pressed() ? 1 : 0; } 
    
    coord_int normal_h() const { return 30; }
};    

class window_label : public base_widget
{          
  private:

    void draw(const graphics_context& grx);

  protected:

    std::string text;
    frame_type frame;
    bool multiline;
  
  public:
  
    window_label(const std::string& t = "", bool multi =false, frame_type f =ft_shallow_in)
    : base_widget(false), text(t), frame(f), multiline(multi)
    { }
  
    void set_text(const std::string& s)
    {
      text = s;
      display();
    }
    
    std::string get_text() { return text; }
    coord_int normal_w() const { return 100; }
    coord_int normal_h() const { return 15; } 
};

class window_scrollbar : public base_widget
{
  private:
  
    void draw(const graphics_context& grx);
  
  protected:
  
    void position_children();
  
    int value;
    int min;
    int max;  
    int bar_width;
    int unit_step;
    int page_step;
  
    hv_orientation orientation;
    
    bool top_page;
    bool bottom_page;
    
    bool autosize;
 
    void event_mouse_hold(int t);
    void event_mouse_up(bt_int button);
  
    class slider_bar : public base_window
    {   
      private:

        void draw(const graphics_context& grx);
        void event_mouse_drag(coord_int x, coord_int y);    
        
    } bar;
    friend class slider_bar;    
    
    window_icon upbut;
    window_icon downbut;         
    
    void update_to_bar();
    void update_from_bar(); 
    void update_value(int new_value);       
    void place_buttons();
    
    void press_up(const mouse_hold_ei& ei);
    void press_down(const mouse_hold_ei& ei);

  public:

    coord_int normal_w() const { return (orientation==hv_vertical) ? 16 : 0; }
    coord_int normal_h() const { return (orientation==hv_horizontal) ? 16 : 0; }
   
    window_scrollbar(hv_orientation o =hv_horizontal)   
    : base_widget(true), value(0), min(0), max(50), bar_width(30), unit_step(1), 
      page_step(10), orientation(o), top_page(false), bottom_page(false), autosize(true)
    {    
      add_child(upbut);
      add_child(bar);
      add_child(downbut);
    }
   
    void set_min(int n) { min = n; update_value(value); }    
    void set_max(int n) { max = n; update_value(value); }
    void set_bar(int n=0) { autosize = (n == 0); bar_width = n; update_to_bar(); }
    void set_unit_step(int n) { unit_step = n; update_to_bar(); }
    void set_page_step(int n) { page_step = n; update_to_bar(); }
    
    int get_min() const { return min; }
    int get_max() const { return max; }
    int get_bar() const { return bar_width; }
    int get_unit_step() const { return unit_step; }
    int get_page_step() const { return page_step; }
    
    float get_fraction() const { return float(max - min) / float(value - min); }
    
    void increment() { update_value(value + unit_step); }
    void decrement() { update_value(value - unit_step); }
    void increment_page() { update_value(value + page_step); }
    void decrement_page() { update_value(value - page_step); }
    
    void set_value(int n) { update_value(n); }
    int get_value() const { return value; }    
   
    void pre_load();  
    void move_resize_hook(const zone& old_pos, bool moved, bool resized);
};  

struct scroll_ei : public int_input_ei
{ DEFINE_EI(scroll_ei, int_input_ei)

  const int change;
  
  scroll_ei(int v, int c)
  : int_input_ei(v), change(c)
  { }                                                  
};    

class window_image : public base_window
{
  public: // TODO: Add arbitrary alignment
  
    enum display_method
    {
      stretch, tile, centre
    };
    
    window_image(std::string file, display_method m =centre);
    window_image(BITMAP* bmp, display_method m =centre);
    window_image();
    ~window_image();
        
    void load(BITMAP* bmp, display_method m =centre);
    void load(std::string file, display_method m =centre);
    void set_method(display_method m =centre);
    
    BITMAP* get_image() { return image; } 
    void set_image(BITMAP* new_image);
    
  protected:
  
    BITMAP* image;
    display_method method;
    
    void pre_load();
    
  private:
  
    void draw(const graphics_context& grx);
    
    coord_int normal_w() const;
    coord_int normal_h() const;
};

class window_canvas : public base_window
{
  public:
  
    const graphics_context context();    
    void clear();
    BITMAP* get_buffer() { return buffer; }
    
    using base_window::add_child;
        
  protected:
  
    BITMAP* buffer;
        
    void post_load();
    void post_unload();
    
  private:
  
    void draw(const graphics_context& grx);
};
  
  
class window_checkbox : public window_label
{
  private:
  
    bool state;
    void draw(const graphics_context& grx);
    
  protected:
  
    void event_key_down(kb_event key);
    void event_mouse_down(bt_int button); 

  public:
  
    window_checkbox(const std::string& t ="", bool s =false)
    : window_label(t, ft_none), state(s)
    { }
    
    int get_value() const { return state ? 1 : 0; }
    void set_value(int v);
    void reset() { set_value(0); }
    
    coord_int normal_w() const { return 60; }
    coord_int normal_h() const { return 15; } 
};    

class window_radiobutton : public window_label
{
  private:
    
    bool state;
    void draw(const graphics_context& grx);
    
  protected:
  
    void event_mouse_down(bt_int button);
    void event_key_down(kb_event key);
  
  public:
  
    window_radiobutton(const std::string& t ="")
    : window_label(t, ft_none), state(false)
    { }
    
    int get_value() const { return state ? 1 : 0; }
    void set_value(int v);
    void reset() { set_value(0); }
    
    coord_int normal_w() const { return 60; }
    coord_int normal_h() const { return 15; } 
};


class window_textbox : public base_widget
{
  private:
  
    void draw(const graphics_context& grx);
    
  protected:
   
    std::string text;
    int lines;
    int line;
    unsigned int edit_pos;
    bool multiline;
    bool wordwrap;
        
    window_scrollbar vscroll;
        
    void event_mouse_down(bt_int button);
    void event_mouse_on() { set_cursor(cursor_caret); }
    void event_mouse_off() { set_cursor(cursor_normal); }
    void event_key_down(kb_event kb);
    void event_key_blink() { display(); } 
    void event_key_unfocus() { display(); }
    void event_key_focus() { display(); }                 
    
    void position_children();
    
    void change_text();
    void pre_load();
    void post_load();
        
    void scrolled(const scroll_ei& ei)
    {        
      line = ei.value;
      display();
    } 
    
  public:
  
    window_textbox(const std::string& initial = "", bool ml =false, bool ww =true) 
    : base_widget(true), text(initial), lines(0), line(0), edit_pos(0), multiline(ml), wordwrap(ww), vscroll(hv_vertical)
    { 
      if (multiline) add_child(vscroll); 
      set_flag(evt_snoop_clicks); 
    }
    
    int get_value() const 
    {
      return edit_pos;
    }
    
    void set_text(const std::string& s)
    {
      text = s;
      change_text();
      display();
    }
    
    std::string get_text() const
    {
      return text;
    }
    
    void reset()
    { 
      text = "";
      change_text();
      display();
    }
    
    coord_int e_ax() const { return 2; }
    coord_int e_ay() const { return 2; }
    coord_int e_bx() const { return w()-2; }
    coord_int e_by() const { return h()-2; }
    
    coord_int normal_w() const { return 150; }
    coord_int normal_h() const { return 23; }
};           

class window_listbox : public base_widget
{
  private:
  
    void draw(const graphics_context& grx);
    
  protected:  
  
    int cur_sel;
    std::vector<std::string> list;
    window_scrollbar vscroll;
    int line;
    
    void event_mouse_down(bt_int button);
    void event_key_down(kb_event kb);
    
    coord_int e_ax() const { return 2; }
    coord_int e_ay() const { return 2; }
    coord_int e_bx() const { return w()-2; }
    coord_int e_by() const { return h()-2; }
    
    void position_children();
                       
    void change_list();
    
    void pre_load();    
    void post_load();
    
    void scrolled(const scroll_ei& ei)
    {        
      line = ei.value;
      display();
    }

  public:
  
    std::string get_item(int n);
    void select(int n);
    
    void add_item(const std::string& s);
    void remove_item(const std::string& s);
    void remove_item(int n);
    void clear_list();   
        
    int get_value() const { return cur_sel; }
    std::string get_text() const { return (cur_sel > -1) ? list[cur_sel] : ""; }
    
    void set_value(int v);
    void set_text(const std::string& str);
    
    window_listbox()
    : base_widget(true), cur_sel(-1), vscroll(hv_vertical), line(0)
    { add_child(vscroll); set_flag(evt_snoop_clicks); }    
};      

struct selection_change_ei : public string_input_ei
{ DEFINE_EI(selection_change_ei, string_input_ei)

  const int index;
  
  selection_change_ei(const std::string& s, int i)
  : string_input_ei(s), index(i)
  { }
};

class window_pane : public base_window
{
  public:
  
    coord_int e_ax() const { return 2; }
    coord_int e_ay() const { return 2; }
    coord_int e_bx() const { return w()-2; }
    coord_int e_by() const { return h()-2; }

    bool hvis;
    bool vvis;

    enum scrollstate
    {
      none,
      fixed,
      conceal,
      visible,
    };      
  
    window_pane(base_window& c, scrollstate h =visible, scrollstate v=visible);

  private:
    
    void draw(const graphics_context& grx);
    
    base_window& content;
    window_scrollbar hscroll;
    window_scrollbar vscroll;
    window_block corner_block;
   
    scrollstate hstate, vstate;            

    void post_load();
    void pre_load();
    void position_children();
    
    void content_resize(const move_resize_ei& ei);    
    void update_hscroll(const scroll_ei& ei);
    void update_vscroll(const scroll_ei& ei);
    void update_content();    
};

#endif
