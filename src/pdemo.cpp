#include <ostream.h>
#include <fstream.h>
#include <string.h>
#include <sstream>

#include "allegro.h"
#include "penguin.h"

extern "C" BITMAP* load_jpg(char const*, RGB *);

FONT* cfont; // Font used by the console 
// Function called to set-up the graphics:
bool init_graphics();

// Event-info objects that 'file_chooser_class' emits:
struct dialogue_ei : public event_info
{ DEFINE_EI(dialogue_ei, event_info);
};    

struct dialogue_ok_ei : public dialogue_ei
{ DEFINE_EI(dialogue_ok_ei, dialogue_ei)
  
  std::string file;
  dialogue_ok_ei(const std::string& s) : file(s) { }
};

struct dialogue_cancel_ei : public dialogue_ei
{ DEFINE_EI(dialogue_cancel_ei, dialogue_ei)
};   

/* Class to create a window with space for a filename to be typed and an OK and
 * cancel button. It emits an event of type 'dialogue_ok_ei' when OK is pressed,
 * and stores the file entered in the event-info object. It emits an event of 
 * type 'dialogue_cancel_ei' when Cancel is pressed. This contains nothing. 
 * After emitting these events, it removes itself from the window tree 
 */
class file_chooser_class : public window_main
{
  private: 
  
    base_window& creator;        // Window which created us. To simulate 'modality', we disable it until we are finished
    
    window_textbox text;         // Textbox to enter filename into
    window_button ok_button;     // Ok button
    window_button cancel_button; // Cancel button
    
    void pre_load()
    {
      // Disable our creator, listen for the ok and cancel buttons being pressed,
      // set the focus to our textbox, and place ourselves in the centre of the
      // screen.
      
      creator.set_flag(evt_disabled, true);
      listen(ok_button, VOID_LISTENER(file_chooser_class::ok_press, activate_ei));
      listen(cancel_button, VOID_LISTENER(file_chooser_class::cancel_press, activate_ei));
      text.set_keyfocus();    
      place(c_centre);              
    }
    
    void pre_unload() // Re-enable our creator as we are removed.
    {
      creator.set_flag(evt_disabled, false);
    }
    
    // Self explanatory:
    void ok_press()
    {
      transmit(dialogue_ok_ei(text.get_text()));
      remove();       
    }
    
    void cancel_press()
    {
      transmit(dialogue_cancel_ei());
      remove();      
    }
    
    void event_key_down(kb_event key)
    {
      // If ENTER or ESC is pressed (we have snooping enabled, so even though
      // they would occur to the textbox we'll still get them), quit the dialogue.
      if (key.is_scan(pk_esc))
      {
        cancel_press();
      } else if (key.is_scan(pk_enter))
      {
        ok_press();
      }
    }
              
    
  public:
  
    file_chooser_class(base_window& c) 
    : window_main("Enter the name of the file"), creator(c),
      ok_button("OK"), cancel_button("Cancel")
    { 
      resize(200, 80); // Resize ourselves to the right dimensions
      
      set_layout(new compass_layout(2,2));      
      add_child(text, new compass_info(c_north));      
      add_child(ok_button, new compass_info(c_east));
      add_child(cancel_button, new compass_info(c_west));
      
      set_flag(evt_snoop_keys); // We want to snoop the keys (see above)
    }
};

/* Class that represents the painter window. */
class paint_class : public window_main
{
  private:   
   
    window_lframe controls; // 'Control' frame within all editting controls within it 
      window_listbox shape_list; // List of drawing operations user can use
      window_checkbox check_filled; // Checkbox for whether shapes drawn should be filled
      window_checkbox check_trans; // Checkbox for whether transparency should be used
      window_lframe radiogroup; // 'Colours' drawing operations will be performed in
        window_radiobutton radio_colour[6]; // 6 radiobuttons for 6 different colours
                   
    window_pane content_pane; // Pane to contain the image, make it scrollable
      window_canvas canvas; // Image that can be drawn onto
        masked_image marker; // 'Hand' pointer for 2-point shapes (lines,rects,circles)
    
    window_panel button_panel; // Blank panel along the button for save,load,clear
      window_button save_button; // Self explanatory:
      window_button exit_button;
      window_button clear_button;
      
    file_chooser_class chooser; // Chooser that'll come up if we click 'load' or 'save'
      
    enum draw_types
    {
      draw_line = 0,
      draw_circle,
      draw_box,
      draw_doodle,
      draw_fill,
      draw_paint
    };        
      
    int colour; // Colour to use in next drawing operation
    int trans; // Transparency to apply when using 'transparent' effect
   
    // Load hook: start listening to all events.
    void pre_load()
    {
      // Listen for when the user presses, releases and drags when over the canvas
      listen(canvas, VOID_LISTENER(paint_class::draw_down, mouse_down_ei));
      listen(canvas, VOID_LISTENER(paint_class::draw_up, mouse_up_ei));
      listen(canvas, LISTENER(paint_class::draw_drag, mouse_drag_ei));
      
      // Listen for when clear or save is clicked
      listen(clear_button, VOID_LISTENER(paint_class::clear_command, activate_ei));
      listen(save_button, VOID_LISTENER(paint_class::save_command, activate_ei));
      
      // Listen for when the 'save' file-getter dialogue returns with an 'OK'
      listen(chooser, LISTENER(paint_class::got_text, dialogue_ok_ei));
      
      // Listen for clicks from any of the colours.
      for (int i = 0; i < 6; i++)
        listen(radio_colour[i], VOID_LISTENER(paint_class::select_colour, activate_ei));
      
      // Load the little 'hand' image and hide it
      marker.load("mouse/hand.bmp");
      marker.hide();
    }     
    
    // Callback: Called whenever any of the colour-radiobuttons is pressed
    void select_colour()
    {
      // Check which of the radiobuttons is enabled, set the colour appropriately
           if (radio_colour[0].get_value()) colour = get_color(0,0,0);
      else if (radio_colour[1].get_value()) colour = get_color(128,32,196); 
      else if (radio_colour[2].get_value()) colour = get_color(240,0,10);
      else if (radio_colour[3].get_value()) colour = get_color(10,0,240);
      else if (radio_colour[4].get_value()) colour = get_color(10,240,0);
      else if (radio_colour[5].get_value()) colour = get_color(240,128,0);
    }
        
    // Callback: Called whenever the mouse is pressed on the canvas
    void draw_down()
    {      
      if (shape_list.get_value() == draw_paint) trans = 30; else trans = 127;
      
      switch (shape_list.get_value())
      {
        case draw_fill:
        {
          set_special_mode();
          canvas.context().fill(canvas.get_click_x(), canvas.get_click_y(), colour);
          solid_mode();                    
          canvas.display();          
          break;
        }         
        case draw_line: 
        case draw_circle: 
        case draw_box: 
        {
          marker.move(canvas.get_click_x()-4, canvas.get_click_y());
          marker.show();             
          break;
        }
      }
    }
    
    void draw_up()
    {
      marker.hide();
    
      coord_int x1 = canvas.get_click_x(), y1 = canvas.get_click_y();
      coord_int x2 = canvas.get_mouse_x(), y2 = canvas.get_mouse_y();
      bool filled = check_filled.get_value();
      
      set_special_mode();
      
      switch (shape_list.get_value())
      {
        case draw_line:
        {
          canvas.context().line(x1, y1, x2, y2, colour); 
          break;
        }
        case draw_circle:
        {
          int r = (int)hypot(x2-x1,y2-y1);          
          if (filled) canvas.context().circle_fill(x1, y1, r, colour);
          else canvas.context().circle(x1, y1, r, colour); 
          break;
        }
        case draw_box:
        {
          if (filled) canvas.context().rectfill(x1, y1, x2, y2, colour);
          else canvas.context().rect(x1, y1, x2, y2, colour);         
          break;
        } 
      };
      
      solid_mode();            
      canvas.display();
    }
    
    void draw_drag(const mouse_drag_ei& ei)
    {
      switch (shape_list.get_value())
      {
        case draw_doodle:
        {
          coord_int mx = canvas.get_mouse_x(), my = canvas.get_mouse_y();
          set_special_mode();
          canvas.context().line(mx-ei.x_move, my-ei.y_move, mx, my, colour);        
          solid_mode();
          canvas.display();
          
          break;
        } 
        case draw_paint:
        {
          coord_int mx = canvas.get_mouse_x(), my = canvas.get_mouse_y();
          set_special_mode();
          canvas.context().do_line(mx-ei.x_move, my-ei.y_move, mx, my, colour, paint_line);
          solid_mode();
          canvas.display();        
          
          break;
        }
      };
    }
    
    void set_special_mode()
    {
      if (check_trans.get_value())
      {
        drawing_mode(DRAW_MODE_TRANS, 0, 0, 0);
        set_trans_blender(0, 0, 0, trans);
      }
    }
    
    void clear_command()
    {
      canvas.clear();
    }
    
    void save_command()
    {
      set_flag(evt_disabled, true);
      add_sibling(chooser);
    }
    
    // Callback: called when file-getter for 'save' returns
    void got_text(const dialogue_ok_ei& ei)
    {
      if (!ei.file.empty()) // Save the file, if possible
      {      
        PALETTE pal;
        save_bitmap(ei.file.c_str(), canvas.get_buffer(), pal);
      }
      set_flag(evt_disabled, false);
    } 
     
    // Used to draw a think line. Called as a callback by 'do_line' which calls
    // this for every point on the 'paint' line drawn    
    static void paint_line(BITMAP* bmp, int x, int y, int d)
    { 
      circlefill(bmp, x, y, 3, d);
    }

  public:

    paint_class()
    : window_main("Painter"), colour(0), 
      controls("Controls"),      
      radiogroup("Colour"),
      check_filled("Fill shape"),
      check_trans("Transparent"),
      chooser(*this),
      content_pane(canvas, window_pane::conceal, window_pane::conceal),      
      save_button("Save"),
      clear_button("Clear")
    {               
      set_layout(new gobbler_layout(3,3));
      add_child(controls, new gobbler_info(d_west, .4, li_expand_both));
      add_child(button_panel, new gobbler_info(d_south, 40, li_expand_both));       
      add_child(content_pane, new gobbler_info(d_east, 1, li_expand_none));
                  
      button_panel.set_layout(new grid_layout(3,1));
      button_panel.add_child(save_button, new grid_info(1,1)); 
      button_panel.add_child(clear_button, new grid_info(2,1,2));
      
      radiogroup.set_layout(new flow_layout(2, 2));
      for (int i = 0; i < 6;i++)
        radiogroup.add_child(radio_colour[i], new layout_info);
        
      radiogroup.get_layout()->set_conserve(true);
             
      controls.set_layout(new compass_layout(3,3));
      controls.add_child(check_filled, new compass_info(c_south));
      controls.add_child(check_trans, new compass_info(c_south));
      controls.add_child(radiogroup, new compass_info(c_north));      
      controls.add_child(shape_list, new compass_info(c_centre));
                                                                              
      canvas.add_child(marker);
       
      content_pane.resize(204,204);
      canvas.resize(200,200);
      radiogroup.resize(100,120);
      shape_list.resize(100,60);
      check_filled.resize(100,15);
      check_trans.resize(100,15);
      
      check_filled.set_value(true);
                  
      shape_list.add_item("Line");
      shape_list.add_item("Circle");
      shape_list.add_item("Box");
      shape_list.add_item("Doodle");
      shape_list.add_item("Fill");
      shape_list.add_item("Paint"); 
      shape_list.set_value(draw_line);
      
      radio_colour[0].set_value(1);      
      radio_colour[0].set_text("Black");
      radio_colour[1].set_text("Purple");
      radio_colour[2].set_text("Red");
      radio_colour[3].set_text("Blue");
      radio_colour[4].set_text("Green");                                         
      radio_colour[5].set_text("Orange");
    } 
};

class note_class : public window_main
{
  private:
  
    enum { save, load } get_mode;
  
    window_textbox text;
    window_panel button_panel; 
      window_button save_button;
      window_button load_button;
      window_button clear_button;
  
    file_chooser_class chooser;
  
    void save_press()
    {
      get_mode = save;
      add_sibling(chooser);   
    }
    
    void load_press()
    { 
      get_mode = load;
      add_sibling(chooser);
    }      
      
    void got_text(const dialogue_ok_ei& ei)
    {
      if (!ei.file.empty())
      {
        if (get_mode == save)
        {    
          ofstream out(ei.file.c_str());
          if (out)
          {
            out << text.get_text();
          }            
        } else if (get_mode == load)  
        {     
          ifstream in(ei.file.c_str());     
          if (in)
          { 
            std::stringstream temp;
            temp << in.rdbuf();
            text.set_text(temp.str());
          }
        }
      }
    }
  
    void pre_load()
    {
      listen(save_button, VOID_LISTENER(note_class::save_press, activate_ei));
      listen(load_button, VOID_LISTENER(note_class::load_press, activate_ei));
      text.listen(clear_button, VOID_LISTENER(base_widget::reset, activate_ei));
      listen(chooser, LISTENER(note_class::got_text, dialogue_ok_ei));
    }
  
  public:
  
    note_class() 
    : window_main("Note"),
      save_button("Save"),
      load_button("Load"),
      clear_button("Clear"),
      text("", true),
      chooser(*this)
    {
      button_panel.set_h(27);
      
      set_layout(new compass_layout);
      add_child(button_panel, new compass_info(c_south));
      add_child(text, new compass_info(c_centre));
      
      button_panel.set_layout(new flow_layout);
      button_panel.add_child(save_button, new layout_info);
      button_panel.add_child(load_button, new layout_info);
      button_panel.add_child(clear_button, new layout_info);
    }
};

class widget_class : public window_main
{
  private:
  
    std::string instruction_text;
    window_label instruct;      
                               
    window_button button;
    window_toggle toggle;
    window_block block;
    window_label label;     
    window_textbox textbox;
    window_image image;
    window_frame frame;
      window_label frame_text;
    window_lframe lframe;
      window_checkbox checkbox;
      window_radiobutton radio1;
      window_radiobutton radio2;
    window_listbox list;
    window_scrollbar scroll;
    window_pane pane;
      window_image pane_image;
    window_textbox multitext;      
    window_label multilabel;
        
    void show_description(const mouse_on_ei& ei)
    {
      base_widget* source = dynamic_cast<base_widget*>(&ei.source());
      
      if (source) instruct.set_text(source->get_description());        
      
      else if (&ei.source() == &image)      instruct.set_text("\"window_image\"\nWidget that can display a graphical image loaded from a file or memory. It will replace any masked areas of the image with the generic window-background colour. It can display the image as 1) tiled, 2) centred, or 3) stretched to fit.");
      else if (&ei.source() == &lframe)     instruct.set_text("\"window_lframe\"\nNon-widget. Name is 'labelled frame': similar to 'window_frame', but allows a piece of text to be associated with the frame. This is displayed at the top and can be changed and retrieved through 'get_text' and 'set_text'."); 
      else if (&ei.source() == &pane_image) instruct.set_text("\"window_pane\"\nUseful container that associates another window as its child. This window will be managed automatically and the window-pane will provide scrollbars to scroll its area when necessary - in this case, scrolling an image that is too large for the pane.");
      else if (&ei.source() == &block)      instruct.set_text("\"window_block\"\nUsed to provide a solid rectangle of colour, specified in RGB before loading.");
    }
    
    void show_instructions()
    {
      instruct.set_text(instruction_text);
    }
                  
  public:
  
    widget_class()
    : window_main("Widget Demo", 330, 400),
      instruction_text("Move the cursor over the widgets to receive a description."),
      instruct("", true),
      button("Button"),
      toggle("Toggle"),
      block(200,20,100),
      label("Label"),
      textbox("Textbox..."),
      image("images/winamp.bmp"),
      frame_text("Frame", false, ft_none),
      lframe("Label-frame"),
      checkbox("Checkbox"),
      radio1("Radiobutton A"),
      radio2("Radiobutton B"),
      multilabel("Label with:\n- Multiple Lines\n- Word wrapping", true),
      scroll(hv_horizontal),
      pane_image("images/farms.jpg"),
      pane(pane_image),
      multitext("Multiple lines\nWord-wrapping\n.\n.\n.\n.\n.\n.\n.\n.\n.\nScrollable Textbox", true)
      
    {
      resize(390, 532);
    
      button.set_description("\"window_button\"\nBasic button type that can be clicked (and will emit 'activate_ei' events when clicked).");
      toggle.set_description("\"window_toggle\"\nButton that can exist in one of two states: pressed or released. It emits 'activate_ei' and 'deactivate_ei' to represent any change of state. 'get_value' will return 1 if it is currently pressed, 0 if released.");
      label.set_description("\"window_label\"\nWidget that can contain a line of text to be displayed along its width. Any frame-style can be set, and the text can be accessed and changed by 'get_text' and 'set_text'.");
      textbox.set_description("\"window_textbox\"\nWidget that can accept a single line of text, typed through the keyboard. This can be set and retrieved by 'get_text' and 'set_text'.");
      frame_text.set_description("\"window_frame\"\nNon-widget that can be used to contain other windows. Although the default frame-style is 'ft_bevel_in', any other style can be used.");
      checkbox.set_description("\"window_checkbox\"\nWidget that contains a line of text in addition to an on/off 'checkbox'. Checking the box emits 'activate_ei', unchecking it emits 'deactivate_ei'.");
      radio1.set_description("\"window_radiobutton\"\nWidget that is intended to co-exist with a group of other 'radiobuttons' under the same parent. It is identical to the 'window_checkbox' widget, but is mutually exclusive with other 'radiobuttons' in the same parent (enabling one will disable all others).");
      multilabel.set_description("\"window_label\"\nWidget that is the same as the single-line label, but can display multiple lines of text. Multi-line capability is switched on when the constructor is called with a flag set.");
      scroll.set_description("\"window_scrollbar\"\nComplex widget that represents a value in a range. Used automatically by most other widgets/containers, such as 'window_pane'.");
      multitext.set_description("\"window_textbox\"\nIdentical to the single-line textbox, but turns on multiple-line functionality in the constructor. Allows for automatic scrolling, page-up, page-down, etc.");
      list.set_description("\"window_listbox\"\nDisplays a list of possible text-items that can be selected. Will scroll where necessary. The index of the currently selected item is found by 'get_value', and 'get_text' returns the actual entry.");
    
      instruct.set_text(instruction_text); 
      instruct.resize(300, 100);
      list.add_item("Listbox:");
      list.add_item("Option A");
      list.add_item("Option B");
      list.add_item("Option C");      
      
      block.resize(30,30);
      frame.resize(60,40);
      lframe.resize(120,80);
      checkbox.set_w(100);
      radio1.set_w(100);
      radio2.set_w(100);        
      multilabel.resize(120,80);      
      list.resize(80,80);
      scroll.set_w(300);
      pane.resize(160,160);
      multitext.resize(160,160);
          
      set_layout(new flow_layout(10,10, true, true));    
      add_child(instruct, new layout_info(li_expand_w));    
      add_child(button, new layout_info);
      add_child(toggle, new layout_info);
      add_child(label, new layout_info(li_expand_w));
      add_child(textbox, new layout_info);
      add_child(block, new layout_info);      
      add_child(image, new layout_info);
      add_child(frame, new layout_info(li_expand_w));
      add_child(lframe, new layout_info);
      add_child(multilabel, new layout_info);
      add_child(list, new layout_info(li_expand_w));
      add_child(scroll, new layout_info(li_expand_w));
      add_child(pane, new layout_info);
      add_child(multitext, new layout_info);
      
      frame.set_layout(new compass_layout);
      frame.add_child(frame_text, new compass_info(c_centre));

      lframe.set_layout(new flow_layout);
      lframe.add_child(checkbox, new layout_info(li_expand_w));
      lframe.add_child(radio1, new layout_info(li_expand_w));
      lframe.add_child(radio2, new layout_info(li_expand_w));       
    }  
    
    void pre_load()
    {
      listen(instruct, VOID_LISTENER(widget_class::show_instructions, mouse_on_ei));
      listen(button, LISTENER(widget_class::show_description, mouse_on_ei));
      listen(toggle, LISTENER(widget_class::show_description, mouse_on_ei));
      listen(block, LISTENER(widget_class::show_description, mouse_on_ei));
      listen(label, LISTENER(widget_class::show_description, mouse_on_ei));
      listen(textbox, LISTENER(widget_class::show_description, mouse_on_ei));
      listen(image, LISTENER(widget_class::show_description, mouse_on_ei));
      listen(frame_text, LISTENER(widget_class::show_description, mouse_on_ei));
      listen(lframe, LISTENER(widget_class::show_description, mouse_on_ei));
      listen(checkbox, LISTENER(widget_class::show_description, mouse_on_ei));
      listen(radio1, LISTENER(widget_class::show_description, mouse_on_ei));
      listen(radio2, LISTENER(widget_class::show_description, mouse_on_ei));
      listen(list, LISTENER(widget_class::show_description, mouse_on_ei));
      listen(scroll, LISTENER(widget_class::show_description, mouse_on_ei));
      listen(pane_image, LISTENER(widget_class::show_description, mouse_on_ei));
      listen(multilabel, LISTENER(widget_class::show_description, mouse_on_ei));
      listen(multitext, LISTENER(widget_class::show_description, mouse_on_ei));
    }

};

class desktop_class : public window_block
{
  private:
    
    window_image wallpaper;

    paint_class paint_window;
    window_magnifier mag_window;
    note_class note_window;
    masked_image masked_window;
    widget_class widget_window;
    
    window_frame button_frame;
      window_toggle paint_button;
      window_toggle mag_button;
      window_toggle note_button;
      window_toggle widget_button;
      window_toggle masked_button;
      window_button about_button;
      window_button exit_button;     
      
    struct about_class : public window_frame
    {      
      window_button exit_button;
      window_label info;
      
      about_class() : info("", true)
      {
        exit_button.set_text("Exit");
        info.set_text("Penguin is a GUI-library built using the Allegro graphics "
                      "library. It is a platform-independant library that can "
                      "run under DOS, Windows, Linux, BeOS, and any other system "
                      "which supports Allegro. It has taken certain ideas from "
                      "Java's AWT, such as the general layout policies.\n\n"
                      "Sebastian Beynon can be found at \"beynons@worldonline.co.za\""
                      ", or on 011-726-2600.");
      
        set_layout(new compass_layout);
        add_child(exit_button, new compass_info(c_south, li_expand_none));
        add_child(info, new compass_info(c_centre));
        set_flag(evt_dialogue);
      }
      
    } about_window;
           
    void pre_load()
    {
      paint_window.listen(paint_button, VOID_LISTENER(base_window::show, activate_ei)); 
      paint_window.listen(paint_button, VOID_LISTENER(base_window::hide, deactivate_ei));
      mag_window.listen(mag_button, VOID_LISTENER(base_window::show, activate_ei)); 
      mag_window.listen(mag_button, VOID_LISTENER(base_window::hide, deactivate_ei));
      note_window.listen(note_button, VOID_LISTENER(base_window::show, activate_ei)); 
      note_window.listen(note_button, VOID_LISTENER(base_window::hide, deactivate_ei));
      masked_window.listen(masked_button, VOID_LISTENER(base_window::show, activate_ei)); 
      masked_window.listen(masked_button, VOID_LISTENER(base_window::hide, deactivate_ei));     
      widget_window.listen(widget_button, VOID_LISTENER(base_window::show, activate_ei)); 
      widget_window.listen(widget_button, VOID_LISTENER(base_window::hide, deactivate_ei));     
      
      listen(about_button, VOID_LISTENER(desktop_class::show_about, activate_ei));
      listen(about_window.exit_button, VOID_LISTENER(desktop_class::hide_about, activate_ei));
      
      get_manager()->listen(exit_button, VOID_LISTENER(window_manager::close_gui, activate_ei));   
      listen(masked_window, LISTENER(desktop_class::drag_mask, mouse_drag_ei));     
      
      window_block::pre_load();
    }
    
    void show_about()
    {
      set_flag(evt_disabled, true);
      about_window.show();
    }
    
    void hide_about()
    {
      set_flag(evt_disabled, false);
      about_window.hide();
    }
    
    void drag_mask(const mouse_drag_ei& ei)
    {
      masked_window.relative_move(ei.x_move, ei.y_move);
    }
     
  public:
    
    desktop_class() 
    : window_block(100,100,128),
      wallpaper("images/penguin.bmp"),
      masked_window("images/girl.bmp"),
      
      paint_button("Paint Window"),
      mag_button("Magnifier"),
      note_button("Note Window"),
      widget_button("Widget Demos"),
      masked_button("Masked Image"),      

      about_button("About"),
      exit_button("Exit")
    { 
      add_child(wallpaper);    
      add_child(button_frame);
      
      add_child(paint_window); // Add all the demo windows
      add_child(note_window); 
      add_child(widget_window);
      add_child(mag_window);      
      add_child(masked_window); 
      add_child(about_window);
      
      paint_window.hide(); // Hide all the demo windows
      note_window.hide();
      mag_window.hide();
      widget_window.show();
      masked_window.show(); masked_window.set_flag(sys_z_fixed);
      about_window.hide(); about_window.set_flag(sys_z_fixed);
    
      resize(SCREEN_W, SCREEN_H);
    
      wallpaper.resize(normal_size, normal_size);
      wallpaper.place(c_centre);
      button_frame.move(15,15);     

      // Give the demo windows reasonable sizes
      button_frame.resize(150, 300);
      paint_window.resize(400, 300);
      mag_window.resize(200, 200);
      note_window.resize(300, 300);
      about_window.resize(250, 200);
      masked_window.resize();            
        
      // Place all the windows in reasonable starting positions  
      paint_window.place(c_centre);
      note_window.place(c_centre);
      mag_window.place(c_centre);
      masked_window.place(c_centre);
      widget_window.place(c_centre);
      about_window.place(c_centre);
            
      button_frame.set_layout(new compass_layout(5,5));
      button_frame.add_child(paint_button, new compass_info(c_north));
      button_frame.add_child(note_button, new compass_info(c_north));      
      button_frame.add_child(widget_button, new compass_info(c_north));
      button_frame.add_child(mag_button, new compass_info(c_north));
      button_frame.add_child(masked_button, new compass_info(c_north)); 
      
      button_frame.add_child(exit_button, new compass_info(c_south));
      button_frame.add_child(about_button, new compass_info(c_south));        
    }    
};

/*
class desktop_class : public window_block // Inherit from a blank window with a 'block' of colour
{
  private:

    window_frame frame1;
    window_frame frame2;
    window_button button[8]; // There will now be 8 contained buttons.

  public:
    
    desktop_class() // The constructor will set-up the windows: 
    : window_block(128,0,0) // Give our window a red colour (Red,Green,Blue)
    { 
      // Array, containing the names of each of the buttons:
      char numbers[8][6] = {"One","Two","Three","Four","Five","Six","Seven","Eight"};
    
      frame1.resize(150,150); // Set the initial sizes for the frames
      frame2.resize(150,150);
    
      set_layout(new flow_layout(5,5)); // Use a flow-layout for the frames, 5x5 gap
      add_child(frame1, new layout_info); 
      add_child(frame2, new layout_info); 
                 
      // The frames will use grid-layout, with auto-cell counting, and a 10 by 10
      // pixel gap between elements
      frame1.set_layout(new grid_layout(0,0,10,10));
      frame2.set_layout(new grid_layout(0,0,10,10));
            
      for (int i = 0; i < 4; i++) // For each button...
      {                                                     
        // Calculate the x and y positions for each button by the dividend and remaineder
        frame1.add_child(button[i],   new grid_info(i%2,i>>1)); // Add 1-4 to frame1
        frame2.add_child(button[i+4], new grid_info(i%2,i>>1)); // Add 5-8 to frame2
        button[i].set_text(numbers[i]);
        button[i+4].set_text(numbers[i+4]); 
      }
    }
};*/

int main(int argc, char* argv[])
{  
	if (init_graphics())
	{
	  return 1; // If we can't set the graphics mode, quit
	}

	desktop_class desktop; // Declare the desktop class,
	desktop.resize(SCREEN_W, SCREEN_H); // And resize to fit the screen

	// The GUI must have a 'window manager' to run the program. Here, we declare 
	// one and use the desktop window as the 'first' or 'background' window.
	window_manager manager;   
	manager.set_console_font(cfont);
	manager.do_gui(desktop, 0); // Execute the GUI, until 'close_gui' is called.
}
END_OF_MAIN();

bool init_graphics()
{
  allegro_init();  // Install and initialize Allegro
  install_timer();
  install_keyboard();
  install_mouse();

  // Try to set the graphics mode for the selection dialogue
  if (set_gfx_mode(GFX_AUTODETECT_WINDOWED, 320, 200, 0, 0) &&
      set_gfx_mode(GFX_AUTODETECT, 320, 200, 0, 0)) return false;
                               
  set_palette(desktop_palette);
  
  // Hold the values given after the dialogue returns:
  int card = -1, w = 1024, h = 768, depth = 32;
  do
  { // Loop until the CANCEL is pressed or sensible values are chosen by the dialogue.
    if (!gfx_mode_select_ex(&card, &w, &h, &depth)) return false;    
  } while (depth <= 8 || w < 640 || h < 480);
  
  set_mouse_speed(2, 2);
  
  set_color_depth(depth); // Attempt to set the requested graphics mode.    
  request_refresh_rate(75);
  if (set_gfx_mode(card, w, h, 0, 0))
  {
    request_refresh_rate(0);
    if (set_gfx_mode(card, w, h, 0, 0))
    {
      std::stringstream message; // Print out that we couldn't do it!
      message << "Could not set graphics mode " << w << " by " << h << " @ " << depth << " bpp\n";
      allegro_message(message.str().c_str());

      return false;
    }
  }
  
  // Allow JPEGs to be loaded
  register_bitmap_file_type("jpg", load_jpg, NULL);
  
  set_window_title("Penguin GUI"); // Set the title of the Program to 'Penguin GUI'

  // Load the two fonts in the font.dat datafile, return false if impossible
  if (DATAFILE* datafile = load_datafile("fonts/font.dat"))
  {
    cfont = (FONT *)datafile[2].dat; // Console font,
    font = (FONT *)datafile[0].dat; // GUI Font
    return true;
    
  } else
  {
    return false;
  }
}



