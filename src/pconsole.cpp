#include "pconsole.h"

#include "pmanager.h"
#include "allegro.h"
#include "pdefs.h"
#include "pzone.h"
#include "pbasewin.h"
#include "psublim.h"
#include "pwidgets.h"

#include <typeinfo>
#include <iostream.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strstream>
#include <math.h>

#define com_is(s) (!strcmp(console_line, s))
#define com_arg(s) temp = strlen(s), (!strncmp(console_line, s, temp) && console_line[temp])
#define is_arg(s) strstr(console_line + temp, s)
#define num_arg(s) atoi(strstr(console_line + temp, s) + strlen(s))
#define con_out(s...) sprintf(next_console_line(), s)
#define con_put(s) strcpy(next_console_line(), s)
#define arg_str() (console_line + temp)

#define CONSOLE_LINES 256

extern FONT* cfont;

char console_line[128];
char console_out[CONSOLE_LINES][128];
char console_command_history[16][128];
int console_history_line = -1;
int console_history_cur = 0;
int console_char = 0;
int console_mode = 0;
int console_cur_line = -1;
int console_out_max = 0;
int console_prompt_y = 0;
int console_line_h = 0;
base_window* console_gui = 0;
bool console_updated = false;
window_manager* console_man = 0;
FONT* console_font;

void five_sec_handler();
void five_sec_handler_end();
volatile int five_sec_tick = 0;

void display_console_prompt();
void switch_console_mode(int mode);
char* next_console_line();
base_window* console_get_win(char* s, int& num);
bool console_command();

int draw_to_next_sub(base_window* win);
int recalc_viszones(base_window* win);

void console_init()
{
  for (int i = 0; i < CONSOLE_LINES; i++)
    console_out[i][0] = 0;

  for (int i = 0; i < 16; i++)
    console_command_history[i][0] = 0;

  LOCK_VARIABLE(five_sec_tick);
  LOCK_FUNCTION((void*)five_sec_handler);
}

void do_console(window_manager* man, base_window* gui, FONT* _font)
{
  set_clip(screen, 0, 0, screen->w, screen->h);

  console_font = _font;

  rectfill(screen,0,SCREEN_H - text_height(console_font) - 6,SCREEN_W,SCREEN_H,0);

  console_gui = gui;
  console_line[0] = 0;
  console_char = 0;
  console_mode = 1;
  console_prompt_y = SCREEN_H - text_height(console_font) - 5;
  console_man = man;
  console_line_h = text_height(console_font);

  switch_console_mode(1);
  
  // 0 = GUI screen, invisible input.
  // 1 = Clipped one-line prompt.
  // 2 = Entire prompt screen.

  clear_keybuf();

  display_console_prompt();

  while(!key[KEY_ESC])
  {
    char k = readkey() & 0xff;

    if (key[KEY_HOME])
    {
      if (console_history_line == -1) console_history_line = console_history_cur;
      else if (console_history_line == console_history_cur) console_history_line = (console_history_line + 1) % 16;
      console_history_line = (console_history_line + 15) % 16;

      strcpy(console_line, console_command_history[console_history_line]);
      console_char = strlen(console_line);

    } else if (key[KEY_END])
    {
      if ((console_history_line + 1) % 16 == console_history_cur) console_history_line = -1;
      
      if (console_history_line >= 0)
      {
        console_history_line = (console_history_line + 1) % 16;
        
        strcpy(console_line, console_command_history[console_history_line]);
        console_char = strlen(console_line);
      } else console_line[console_char = 0] = 0;
    }
      
    else if (k > 32 && k < 123) { console_line[console_char++] = k; console_line[console_char] = 0; }
    else if (k == 32)      { console_line[console_char++] = ' '; console_line[console_char] = 0; }
    else if (k == 8)         console_line[--console_char] = 0;
    else if (k == 13) if (console_command()) break;

    if (console_char < 0) console_char = 0;
    else if (console_char > 63) console_char = 63;

    display_console_prompt();
  }

  while (key[KEY_ESC]);

  set_clip(screen, 0, 0, screen->w, screen->h);
}

char* next_console_line()
{
  console_updated = true;

  if (console_out_max >= CONSOLE_LINES)
  {
    for (int z = 0; z < CONSOLE_LINES-1; z++)
      strcpy(console_out[z], console_out[z+1]);
      
    return console_out[CONSOLE_LINES-1];
  }
  else return console_out[console_out_max++];
}

void display_console_prompt()
{
  if (console_mode == 2)
  {
    int height = (console_out_max - console_cur_line) * console_line_h;
    if (console_updated && height > (console_prompt_y - text_height(console_font)))
      console_cur_line += (int)ceil(height - (console_prompt_y - text_height(console_font))) / console_line_h;
  }
  
  if (key[KEY_UP])
  {
    if (console_mode < 2 && console_cur_line == -1) console_cur_line = console_out_max - 1;
    else console_cur_line = PMIN(console_cur_line - (key[KEY_LSHIFT] ? 20 : 1), 0);
    console_updated = true;
  }
  if (key[KEY_DOWN])
  {
    if (console_cur_line >= 0) console_cur_line += (key[KEY_LSHIFT] ? 20 : 1);
    if (console_cur_line >= console_out_max) console_cur_line = (console_mode == 2) ? console_out_max - 1 : -1;
    console_updated = true;
  }

  text_mode(0);
  switch (console_mode)
  {
    case 1:

      rectfill(screen,0,SCREEN_H - text_height(console_font) - 6,SCREEN_W,SCREEN_H,0);
      if (console_cur_line == -1)
        textprintf(screen, console_font, 10, console_prompt_y, makecol(32,192,32), ">%s_", console_line);
      else
        textprintf(screen, console_font, 10, console_prompt_y, makecol(32,192,32), "%d] %s", console_cur_line, console_out[console_cur_line]);
      break;
      
    case 2:

      if (console_updated)
        for (int z = PMIN(console_cur_line,0), a = 0; a < console_prompt_y; z++, a += console_line_h)
        {
          if (console_out && z < console_out_max)
          {
            textprintf(screen, console_font, 10, a, makecol(32,192,32), "%s", console_out[z]);
            rectfill(screen,10 + text_length(console_font, console_out[z]), a, SCREEN_W, a + text_height(console_font), 0);
          } else rectfill(screen,10,a,SCREEN_W,a+text_height(console_font),0);
        }

      rectfill(screen,0,SCREEN_H - text_height(console_font) - 6,SCREEN_W,SCREEN_H,0);
      textprintf(screen, console_font, 10, console_prompt_y, makecol(32,192,32), ">%s_", console_line);
      break;
  }
  text_mode(-1);
  
  console_updated = false;
  set_clip(screen, 0, 0, SCREEN_W, SCREEN_H); 
}

void switch_console_mode(int mode)
{
  switch (mode)
  {
    case 0:
      set_clip(screen, 0, 0, SCREEN_W, SCREEN_H);
      break;
      
    case 1:
      set_clip(screen, 0, 0, SCREEN_W, SCREEN_H);
      
      rectfill(screen,0,SCREEN_H - text_height(console_font) - 6,SCREEN_W,SCREEN_H,0);
      set_clip(screen, 0, 0, SCREEN_W, console_prompt_y);

      console_cur_line = -1;
      console_gui->display_all();

      break;
      
    case 2:
      set_clip(screen, 0, 0, SCREEN_W, SCREEN_H);

      console_cur_line = 0;
      clear(screen);
      
      break;
  }
  console_mode = mode;
}

bool console_command()
{
  int temp = 0;

   if (com_arg("load "))
  {
    if (FILE* f = fopen(arg_str(), "r"))
    {
      while (fgets(console_line,128,f))
      {
        console_line[strlen(console_line)-1]=0;
        console_command();
      }
      console_line[0] = 0;
      
    } else con_out("Load failed");
    
  } else if (com_arg("mode "))
  {
    int _mode = atoi(arg_str());
    if (_mode >= 0 && _mode <= 2)
    {
      switch_console_mode(_mode);
      con_out("Set console mode to %d", _mode);
    }
    
  } else if (com_is("cls"))
  {
    console_cur_line = -1;
    console_out_max = 0;
    for (int i = 0; i < CONSOLE_LINES; i++)
      console_out[i][0] = 0;
    console_updated = true;
    
  } else if (com_is("close"))
  {
    return true;
    
  } else if (com_is("halt"))
  {
    console_man->close_gui();
    return true;
    
  } else if (com_arg("flush "))
  {
    if (!strncmp(arg_str(), "cerr", 4))
    {
      for (int i = 0; i < console_out_max; i++)
      {
        for (char* z = console_out[i]; *z; z++) if (*z < 32) cerr << char(32); else cerr << *z;
        cerr << "\n";
      }
      con_out("Flushed history to cerr");
    } else if (FILE* f = fopen(arg_str(), "w"))
    {
      for (int i = 0; i < console_out_max; i++) fprintf(f, "%s\n", console_out[i]);
      con_out("Flushed history to \"%s\"", arg_str());
    } else con_out("Flush failed");
    
  } else if (!strncmp(console_line, "tree", 4))
  {
    base_window* win = console_gui;
  
    if (console_line[4] == ' ' && console_line[5])
    {
      int num;
      win = console_get_win(console_line + 5, num);
      
      if (win)
        con_out("Printing tree from window #%d", num);
      else {
        con_out("Cannot find window #%d", num);
        goto skip;
      }
    } else con_out("Printing tree from GUI:");
      
    std::ostrstream treebuf;
    win->print(treebuf);
    treebuf << char(0);
    
    char* loop = treebuf.str();
    char* start = loop;
    int len = 0;

    while(*loop)
    {
      loop++;

      if (len && (*loop == '\n' || *loop == 0))
      {
        char* line = next_console_line();
        strncpy(line, start, len);
        line[len] = 0;
        start = loop + 1;
        len = 0;
      } else len++;
    }
    next_console_line();
    
  } else if (com_arg("echo "))
  {
    strcpy(next_console_line(), arg_str());

  } else if (com_is("info"))
  {
    next_console_line();
    con_out("Driver name: %s", gfx_driver->name);
    con_out("Description: %s", gfx_driver->desc);
    con_out("Refresh rate: %d", get_refresh_rate());

  } else if (com_is("count"))
  {
    con_out("Zone count      - %d", zone::count);
    con_out("Win count       - %d", base_window::count);
  } else if (com_arg("display "))
  {
    int num;
    base_window* win = console_get_win(arg_str(), num);

    if (win)
    {
      if (is_arg("-vis")) base_window::debug |= W_DEBUG_SHOW_VIS_ZONES;
      if (is_arg("-drawz")) base_window::debug |= W_DEBUG_DRAW_Z_COUNT;
      if (is_arg("-drawid")) base_window::debug |= W_DEBUG_DRAW_WIN_ID;
      
      if (is_arg("-all")) { con_out("Displaying sub-windows"); win->display_all(); }
      else win->display();

      if (is_arg("-tosub"))
      {
        if (is_arg("-all")) win->for_all(draw_to_next_sub);
        else draw_to_next_sub(win);
      }

      if (is_arg("-drawid")) base_window::debug ^= W_DEBUG_DRAW_WIN_ID;
      if (is_arg("-drawz")) base_window::debug ^= W_DEBUG_DRAW_Z_COUNT;
      if (is_arg("-vis")) base_window::debug ^= W_DEBUG_SHOW_VIS_ZONES;
      
      con_out("Drawing window #%d", num);
    } else con_out("Cannot find window #%d", num);

  } else if (com_arg("move "))
  {
    int num;
        
    base_window* win = console_get_win(arg_str(), num);

    if (win)
    {
      if (is_arg("-vis")) base_window::debug |= W_DEBUG_SHOW_VIS_ZONES;


      int x = num_arg("-x");
      int y = num_arg("-y");

      if (is_arg("-abs"))
      {
        x -= win->get_cx();
        y -= win->get_cy();
      }

      if (is_arg("-mouse"))
      {
        coord_int old_x = win->get_ax(), old_y = win->get_ay();
        get_mouse_mickeys(&x,&y);
        poll_mouse();
        while (!mouse_b)
        {
          poll_mouse();
          get_mouse_mickeys(&x, &y);
          if (x || y) win->move(x + win->get_ax(), y + win->get_ay());
        }
        x = win->get_ax() - old_x;
        y = win->get_ay() - old_y;

      } else if (is_arg("-keyboard"))
      {

        coord_int old_x = win->get_ax(), old_y = win->get_ay();
        do
        {
          readkey();
          
          int step = key[KEY_LSHIFT] ? 10 : 1;
          if (key[KEY_8_PAD]) win->relative_move(0, -step);
          else if (key[KEY_6_PAD]) win->relative_move(step, 0);
          else if (key[KEY_4_PAD]) win->relative_move(-step, 0);
          else if (key[KEY_2_PAD]) win->relative_move(0, step);
          else if (key[KEY_7_PAD]) win->relative_move(-step, -step);
          else if (key[KEY_9_PAD]) win->relative_move(step, -step);
          else if (key[KEY_3_PAD]) win->relative_move(step, step);
          else if (key[KEY_1_PAD]) win->relative_move(-step, step);
        } while (!key[KEY_ENTER]);
        
        x = win->get_ax() - old_x;
        y = win->get_ay() - old_y;
        
      } else
      {
        win->move(win->get_ax()+x,win->get_ay()+y);
      }
      
      if (is_arg("-vis")) base_window::debug ^= W_DEBUG_SHOW_VIS_ZONES;

      con_out("Moving window #%d by %d, %d", num, x, y);
    } else con_out("Cannot find window #%d", num);

  } else if (com_arg("find "))
  {
    con_out("Finding windows of type \"%s\":", arg_str());
    for (base_window* loop = console_man; loop; loop = loop->superior())
    {
      if (strcmp(arg_str(), loop->type_name()) == 0)
      {
        if (base_widget* widget = dynamic_cast<base_widget*>(loop))
        {
          con_out("%s(%d) [%d, %d][%d, %d]: \"%s\", %d", loop->type_name(), loop->win_id, 
          loop->get_cx(), loop->get_cy(), loop->w(), loop->h(), widget->get_text().c_str(), widget->get_value());
        } else 
        {
          con_out("%s(%d) [%d, %d][%d, %d]", loop->type_name(), loop->win_id, 
          loop->get_cx(), loop->get_cy(), loop->w(), loop->h());
        }
      }
    }

  } else if (com_arg("resize "))
  {
    int num;
    base_window* win = console_get_win(arg_str(), num);

    if (win)
    {
      if (is_arg("-vis")) base_window::debug |= W_DEBUG_SHOW_VIS_ZONES;
      int x = num_arg("-x");
      int y = num_arg("-y");

      if (is_arg("-relative")) win->resize(win->get_ax()+x, win->get_ax()+y);
      else win->resize(x, y);

      if (is_arg("-vis")) base_window::debug ^= W_DEBUG_SHOW_VIS_ZONES;

      con_out("Resizing window #%d by %d, %d", num, x, y);
    } else con_out("Cannot find window #%d", num);

  } else if (com_arg("z_shift "))
  {
    int num;
    base_window* win = console_get_win(arg_str(), num);

    if (win)
    {
      int a = num_arg("by");
      
      win->z_shift(a);

      con_out("Z-moving window #%d", num);
    } else con_out("Cannot find window #%d", num);

  } else if (com_arg("show "))
  {
    int num;
    base_window* win = console_get_win(arg_str(), num);
    
    if (win)
    {
      win->show();
      con_out("Showing window #%d", num);
    } else con_out("Cannot find window #%d", num);  
  } else if (com_arg("hide "))
  {
    int num;
    base_window* win = console_get_win(arg_str(), num);
    
    if (win)
    {
      win->hide();
      con_out("Hiding window #%d", num);
    } else con_out("Cannot find window #%d", num);  
  } else if (com_arg("list "))
  {
    int num;
    base_window* win = console_get_win(arg_str(), num);

    if (win)
    {
      if (is_arg("-update"))
      {
        if (is_arg("-all")) win->for_all(recalc_viszones);
        else recalc_viszones(win);
      }

      const zone* loop = win->get_vis_list();
      bool step = is_arg("-step") ? true : false;
      bool draw = is_arg("-draw") ? true : false;
      int a = 0;

      con_out("Listing vis_list of window #%d:", num);
      
      while (loop)
      {
        con_out("%d] %d,%d-%d,%d", a++, loop->ax, loop->ay, loop->bx, loop->by);

        if (step)
        {
          set_clip(screen, 0, 0, SCREEN_W, SCREEN_H);
          
          console_cur_line = console_out_max - 1;
          display_console_prompt();

          int win_id = win->get_win_id();
          int c = makecol(((win_id + 2) * 4949) % 256,((win_id + 4) * 27334) % 256,((win_id + 2) * 344834) % 256);
          rect(screen, loop->ax, loop->ay, loop->bx, loop->by, c);
          rect(screen, loop->ax + 2, loop->ay + 2, loop->bx - 2, loop->by - 2, c);

          readkey();
        }
        if (draw)
        {
          set_clip(screen, 0, 0, SCREEN_W, SCREEN_H);

          {
            graphics_context context(win->get_master()->get_buffer(), win->get_cx(), win->get_cy(), win->get_master()->get_theme());
            context.clip(loop);
            win->draw(context);
          }

          readkey();
        }

        loop = loop->next;
      }
      if (step && console_mode == 1) console_cur_line = -1;
    } else con_out("Cannot find window #%d", num);

  } else if (com_is("clip"))
  {
    con_out("Clipping space of screen is %d,%d-%d,%d", screen->cl, screen->ct, screen->cr-1, screen->cb-1);
    char* temp = 0;
    if ((temp = strstr(console_line, "-ax"))) screen->cl = atoi(temp+4);
    if ((temp = strstr(console_line, "-ay"))) screen->ct = atoi(temp+4);
    if ((temp = strstr(console_line, "-bx"))) screen->cr = atoi(temp+4)+1;
    if ((temp = strstr(console_line, "-by"))) screen->cb = atoi(temp+4)+1;
    if (temp) con_out("...Now set to %d,%d-%d,%d", screen->cl, screen->ct, screen->cr-1, screen->cb-1);
    
  } else if (com_arg("clear "))
  {
    int num;
    if (!strcmp(arg_str(), "screen"))
    {
      clear(screen);
      con_out("Cleared screen");
    } else if (base_window* win = console_get_win(arg_str(), num))
    {
      if (win)
      {
        if (is_arg("-sub"))
        {
          if (dynamic_cast<window_sub*>(win)) { con_out("Cleared sub_buffer of window #%d", num); clear(dynamic_cast<window_sub*>(win)->get_sub_buffer()); }
          else { con_out("Window #%d does not have base class sub_window", num); goto skip; }
        } else { con_out("Cleared surface of window #%d", num); clear(win->get_master()->get_buffer()); }
      } else { con_out("Could not find window #%d", num); goto skip; }
    }
  } else if (com_arg("bmp "))
  {
    int num;
    if (!strcmp(arg_str(), "screen"))
    {
      con_out("Screen pointer is %p", screen);
      con_out("Screen size is %d, %d", SCREEN_W, SCREEN_H);
      con_out("Screen virtual size is %d, %d", VIRTUAL_W, VIRTUAL_H);
      con_out("Screen clipping space is %d,%d-%d,%d", screen->cl, screen->ct, screen->cr-1, screen->cb-1);
      con_out("Screen color depth is %d", bitmap_color_depth(screen));
      if (is_linear_bitmap(screen)) con_out("Screen is a linear bitmap");
      if (is_planar_bitmap(screen)) con_out("Screen is a planar bitmap");

    } else if (base_window* win = console_get_win(arg_str(), num))
    {
      BITMAP* bmp;
      if (win)
      {
        if (is_arg("-sub"))
        {
          if (dynamic_cast<window_sub*>(win)) { con_out("Accessing sub_buffer of window #%d", num); bmp = dynamic_cast<window_sub*>(win)->get_sub_buffer(); }
          else { con_out("Window #%d does not have base class sub_window", num); goto skip; }
        } else if(is_arg("-master"))
        {
          if (dynamic_cast<window_master*>(win)) { con_out("Accessing master-buffer of window #%d", num); bmp = dynamic_cast<window_master*>(win)->get_buffer(); }
          else { con_out("Window #%d does not have base class window_master", num); goto skip; }
        } else { con_out("Accessing surface of window #%d", num); bmp = win->get_master()->get_buffer(); }
      }
      
      con_out("Bitmap pointer is %p", bmp);
      con_out("Bitmap size is %d, %d", bmp->w, bmp->h);
      con_out("Bitmap clipping space is %d,%d-%d,%d", bmp->cl, bmp->ct, bmp->cr-1, bmp->cb-1);
      con_out("Bitmap color depth is %d", bitmap_color_depth(bmp));

      if (is_arg("-draw"))
      {
        con_out("Drawing bitmap...");
        int sx = (SCREEN_W / 2) - (bmp->w / 2);
        int sy = (SCREEN_H / 2) - (bmp->h / 2);

        clear(screen);
        set_clip(screen, 0, 0, SCREEN_W, SCREEN_H);
//        blit(bmp, screen, 0, 0, 0, 0, bmp->w, bmp->h);
        blit(bmp, screen, 0, 0, sx, sy, bmp->w, bmp->h);
        rect(screen, sx-2,sy-2,sx+bmp->w+2,sy+bmp->h+2, makecol(255,255,255));
        readkey();
        clear(screen);
      }
        
    } else con_out("Could not find window #%d", num);
  } else if (com_arg("debugon "))
  {
    if (is_arg("all")) base_window::debug = ~0;
    else if (*arg_str() == '?')
    {
      con_out("Debugging flags:");
      con_out("draw_arb            - %c", (base_window::debug & W_DEBUG_DRAW_ARB_ZONES ? 25 : 26));
      con_out("draw_vis            - %c", (base_window::debug & W_DEBUG_SHOW_VIS_ZONES ? 25 : 26));
      con_out("no_delegate_display - %c", (base_window::debug & W_DEBUG_NO_DELEGATE_DISPLAY ? 25 : 26));
      con_out("no_update_sub       - %c", (base_window::debug & W_DEBUG_NO_UPDATE_SUB ? 25 : 26));
      con_out("step_display        - %c", (base_window::debug & W_DEBUG_STEP_DISPLAY ? 25 : 26));
      con_out("log_display         - %c", (base_window::debug & W_DEBUG_LOG_DISPLAY ? 25 : 26));
      con_out("draw_z_count        - %c", (base_window::debug & W_DEBUG_DRAW_Z_COUNT ? 25 : 26));
      con_out("draw_win_id         - %c", (base_window::debug & W_DEBUG_DRAW_WIN_ID ? 25 : 26));
      con_out("draw_d_count        - %c", (base_window::debug & W_DEBUG_DRAW_D_COUNT ? 25 : 26));
      con_out("no_ysort            - %c", (base_window::debug & W_DEBUG_NO_YSORT ? 25 : 26));
    } else {
      if (is_arg("draw_arb")) { base_window::debug |= W_DEBUG_DRAW_ARB_ZONES; con_out("Set debugging flag: draw_arb_zones"); }
      if (is_arg("draw_vis")) { base_window::debug |= W_DEBUG_SHOW_VIS_ZONES; con_out("Set debugging flag: show_vis_zones"); }
      if (is_arg("no_delegate_display")) { base_window::debug |= W_DEBUG_NO_DELEGATE_DISPLAY; con_out("Set debugging flag: no_delegate_display"); }
      if (is_arg("no_update_sub")) { base_window::debug |= W_DEBUG_NO_UPDATE_SUB; con_out("Set debugging flags: no_update_sub"); }
      if (is_arg("step_display"))   { base_window::debug |= W_DEBUG_STEP_DISPLAY; con_out("Set debugging flag: step_display"); }
      if (is_arg("log_display"))    { base_window::debug |= W_DEBUG_LOG_DISPLAY; con_out("Set debugging flag: log_display"); }
      if (is_arg("draw_z_count"))    { base_window::debug |= W_DEBUG_DRAW_Z_COUNT; con_out("Set debugging flag: draw_z_count"); }
      if (is_arg("draw_win_id"))    { base_window::debug |= W_DEBUG_DRAW_WIN_ID; con_out("Set debugging flag: draw_win"); }
      if (is_arg("draw_d_count"))    { base_window::debug |= W_DEBUG_DRAW_D_COUNT; con_out("Set debugging flag: draw_d_count"); }
      if (is_arg("no_ysort"))    { base_window::debug |= W_DEBUG_NO_YSORT; con_out("Set debugging flag: no_ysort"); }
    }
  } else if (com_arg("debugoff "))
  {
    if (is_arg("all")) base_window::debug = 0;
    else
    {
      if (is_arg("draw_arb")) { base_window::debug &= ~W_DEBUG_DRAW_ARB_ZONES; con_out("Unset debugging flag: draw_arb_zones"); }
      if (is_arg("no_delegate_display")) { base_window::debug &= ~W_DEBUG_NO_DELEGATE_DISPLAY; con_out("Unset debugging flag: no_delegate_display"); }
      if (is_arg("no_update_sub")) { base_window::debug &= ~W_DEBUG_NO_UPDATE_SUB; con_out("Unset debugging flags: no_update_sub"); }
      if (is_arg("draw_vis")) { base_window::debug &= ~W_DEBUG_SHOW_VIS_ZONES; con_out("Unset debugging flag: show_vis_zones"); }
      if (is_arg("step_display"))   { base_window::debug &= ~W_DEBUG_STEP_DISPLAY; con_out("Unset debugging flag: step_display"); }
      if (is_arg("log_display"))    { base_window::debug &= ~W_DEBUG_LOG_DISPLAY; con_out("Unset debugging flag: log_display"); }
      if (is_arg("draw_z_count"))    { base_window::debug &= ~W_DEBUG_DRAW_Z_COUNT; con_out("Unset debugging flag: draw_z_count"); }
      if (is_arg("draw_win_id"))    { base_window::debug &= ~W_DEBUG_DRAW_WIN_ID; con_out("Unset debugging flag: draw_win_id"); }
      if (is_arg("draw_d_count"))    { base_window::debug &= ~W_DEBUG_DRAW_D_COUNT; con_out("Unset debugging flag: draw_d_count"); }
      if (is_arg("no_ysort"))    { base_window::debug &= ~W_DEBUG_NO_YSORT; con_out("Unset debugging flag: no_ysort"); }
    }
  } else if (com_arg("remove "))
  {
    int num;
    if (base_window* win = console_get_win(arg_str(), num))
    {
      con_out("Removed window #%d", num);
      win->remove();
    } else con_out("Could not find window #%d", num);
    
  } else if (com_arg("window "))
  {
    int num;
    base_window* win = console_get_win(arg_str(), num);

    if (win)
    {
      int viszones = 0;
      const zone* loop = win->get_vis_list();
      while (loop) { viszones++; loop = loop->next; }
    
      con_out("Window #%d:", num);
      con_out("logical  - %d,%d,%d,%d", win->get_ax(), win->get_ay(), win->get_bx(), win->get_by());
      con_out("physical - %d,%d,%d,%d [%d,%d]", win->get_cx(), win->get_cy(), win->get_dx(), win->get_dy(), win->w(), win->h());
      con_out("clipped  - %d,%d,%d,%d [%d,%d]", win->get_ccx(), win->get_ccy(), win->get_cdx(), win->get_cdy(), win->get_cdx() - win->get_ccx(), win->get_cdy() - win->get_ccy());
      con_out("type     - %s", win->type_name());
      con_out("viszones - %d", viszones);

      if (win->flag(base_window::vis_negative_clip)) con_out("<negative_clip>");
      if (win->flag(base_window::vis_positive_clip)) con_out("<positive_clip>");
      if (win->flag(base_window::vis_complete_clip)) con_out("<complete_clip>");
      if (!win->flag(base_window::vis_visible)) con_out("<invisible>");

      if (win->get_parent())     con_out("parent   - %d", win->get_parent()->get_win_id());
      if (win->get_child())      con_out("child    - %d", win->get_child()->get_win_id());
      if (win->get_next())       con_out("next     - %d", win->get_next()->get_win_id());
      if (win->get_prev())       con_out("prev     - %d", win->get_prev()->get_win_id());
      if (win->get_next_sub())   con_out("sub      - %d", win->get_next_sub()->get_win_id());
      if (win->get_manager())    con_out("manager  - %d", win->get_manager()->get_win_id());
      if (win->get_master())     con_out("master   - %d", win->get_master()->get_win_id());
      if (win->get_master()->get_buffer())        con_out("surface  - %p", win->get_master()->get_buffer());
      next_console_line();
      
    } else con_out("Cannot find window #%d", num);

  } else if (com_arg("update_sub "))
  {
    int num;
    base_window* win = console_get_win(arg_str(), num);

    if (win && dynamic_cast<window_sub*>(win))
    {    
      dynamic_cast<window_sub*>(win)->update_sub();
      con_out("Updated window #%d's sub_buffer", num);
      
    } else con_out("Cannot find subparent window #%d", num);

  } else if (com_arg("time "))
  {
    int num;
    base_window* win = console_get_win(arg_str(), num);

    if (is_arg("-zone"))
    {
      con_out("Timing:");

      if (is_arg("-alloc"))
      {
        zone* head = new zone(0,1,2,3);
        zone* loop = head;
        
        int count = 0;
        five_sec_tick = 0;
        install_int(five_sec_handler, 1000);

        while (!five_sec_tick)
        {
          loop->next = new zone(800,671,642,243);
          loop = loop->next;
          count++;
        }
        remove_int(five_sec_handler);

        for (zone* z = head, *next_z; z; z = next_z)
        {
          next_z = z->next;
          delete z;
        }
      
        con_out("Zones allocated per second: %d", count);
        
      } else if (is_arg("-occ"))
      {
        int count = 0;
        five_sec_tick = 0;
        install_int(five_sec_handler, 1000);

        while (!five_sec_tick)
        {
          zone* a = new zone(rand() % 1023, rand() % 1023, rand() % 1023, rand() & 1023);
          zone b(rand() % 1023, rand() % 1023, rand() % 1023, rand() & 1023);
          occlude(a, &b);
          delete_zonelist(a);
          
          count++;
        }
        remove_int(five_sec_handler);

        con_out("Zones occluded per second: %d", count);
      }
      
    } else if (win)
    {      
      con_out("Timing window #%d:", num);
      
      acquire_bitmap(screen);
      
      if (is_arg("-pure")) { base_window::debug |= W_DEBUG_NO_DISPLAY; con_out("- pure"); }
      if (is_arg("-novis")) { base_window::debug |= W_DEBUG_IGNORE_VISZONES; con_out("- ignore viszones"); }
      if (is_arg("-nodaz")) { base_window::debug |= W_DEBUG_NO_DAZ_OPT; con_out("- no daz optimization"); }
      
      if (is_arg("-move"))
      {
        int count = 0;
        
        int x = num_arg("-x");
        int y = num_arg("-y");

        if (!x && !y) x = -5;

        five_sec_tick = 0;
        install_int(five_sec_handler, 5000);

        while (!five_sec_tick)
        {
          win->move(win->get_ax()+x, win->get_ay()+y);
          win->move(win->get_ax()-x, win->get_ay()-y);
          count += 2;
        }
        remove_int(five_sec_handler);

        con_out("Moves per second: %d", int(count / 5));
      } else if (is_arg("-slide"))
      {
        int count = 0;
        five_sec_tick = 0;
        install_int(five_sec_handler, 5000);

        while (!five_sec_tick)
        {
          win->move( win->get_ax()+1, win->get_ay() );
          count += 1;
        }
        remove_int(five_sec_handler);

        con_out("Moves per second: %d", int(count / 5));
      } else if (is_arg("-draw"))
      {
        int count = 0;
        
        five_sec_tick = 0;
        install_int(five_sec_handler, 5000);

        while (!five_sec_tick)
        {
          win->display_all();
          count++;
        }
        remove_int(five_sec_handler);

        con_out("Draws per second: %d", int(count / 5));
      } else if (is_arg("-random"))
      {
        int count = 0;

        five_sec_tick = 0;
        install_int(five_sec_handler, 5000);

        int max_w = win->get_parent()->w() - win->w();
        int max_h = win->get_parent()->h() - win->h();

        srand(12345);

        while (!five_sec_tick)
        {
          win->move(rand() % max_w, rand() % max_h);
          count++;
        }
        remove_int(five_sec_handler);

        con_out("Placements per second: %d", int(count / 5));
      } else if (is_arg("-clock"))
      {
        int count = 0;

        five_sec_tick = 0;
        install_int(five_sec_handler, 5000);

        int half_w = win->w() / 2;
        int half_h = win->h() / 2;
        int max_w = win->get_parent()->w();
        int max_h = win->get_parent()->h();
        int centre_x = max_w / 2;
        int centre_y = max_h / 2;
        
        int radius = centre_x - half_w;
        if (radius > centre_y - half_h) radius = centre_y - half_h ;

        srand(12345);

        while (!five_sec_tick)
        {
          win->move(centre_x + (sin(count / 50.0) * radius) - half_w,
                    centre_y + (cos(count / 50.0) * radius) - half_h);
          count++;
        }
        remove_int(five_sec_handler);

        con_out("Placements per second: %d", int(count / 5));
      } else if (is_arg("-vis"))
      {
        int count = 0;
        five_sec_tick = 0;
        install_int(five_sec_handler, 5000);

        while (!five_sec_tick)
        {
          win->update_family_vislist();
          count++;
        }
        remove_int(five_sec_handler);

        con_out("Viszone calculations per second: %d", count);
      } else if (is_arg("-pack"))
      {
        int count = 0;
        five_sec_tick = 0;
        install_int(five_sec_handler, 5000);

        while (!five_sec_tick)
        {
          win->pack();
          count++;
        }
        remove_int(five_sec_handler);

        con_out("Packs per second: %d", count);
      }
      
      if (is_arg("-pure")) base_window::debug ^= W_DEBUG_NO_DISPLAY;
      if (is_arg("-novis")) base_window::debug ^= W_DEBUG_IGNORE_VISZONES;
      if (is_arg("-nodaz")) base_window::debug ^= W_DEBUG_NO_DAZ_OPT;
      
      release_bitmap(screen);
      
    } else con_out("Cannot find window #%d", num);
    
  } else if (*console_line == 0)
  {
    *next_console_line() = 0;
  } else {
    con_out("Unknown command \"%s\"", console_line);
  }

  skip:

  strcpy(console_command_history[console_history_cur], console_line);
  console_history_cur = (console_history_cur + 1) % 16;

  console_line[0] = 0;
  console_char = 0;
  
  return false;
}

base_window* console_get_win(char* s, int& num)
{
  num = -1;
  if (*s == '*')
  {
    base_window* t = console_man->get_target();
    if (t)
    {
      num = t->get_win_id();
      return console_man->get_target();
    } else {
      return 0;
    }
  } else if (*s == '!')
  {
    num = console_man->get_keyfocus()->get_win_id();
    return console_man->get_keyfocus();

  } else {
    num = atoi(s);
    return console_gui->find(num);
  }
  return 0;
}

int draw_to_next_sub(base_window* win)
{
  if (window_sub* temp = win->get_next_sub())
  {
    BITMAP* bmp = screen;

    int x = (temp->get_cx() + temp->get_dx()) / 2;
    int y = (temp->get_cy() + temp->get_dy()) / 2;

    int xd1 = win->get_cx() - x;
    int xd2 = win->get_dx() - x;
    int yd1 = win->get_cy() - y;
    int yd2 = win->get_dy() - y;

    int x2 = (abs(xd1) > abs(xd2)) ? win->get_dx() : win->get_cx();
    int y2 = (abs(yd1) > abs(yd2)) ? win->get_dy() : win->get_cy();

    xor_mode(1);
    rect(bmp, win->get_cx(), win->get_cy(), win->get_dx(), win->get_dy(), makecol(255,255,255));
    line(bmp, x2, y2, x, y, makecol(255,255,255));
    circle(bmp, x, y, 5, makecol(255,255,255));
    solid_mode();
  }

  return 0;
}

int recalc_viszones(base_window* win)
{
 // win->calculate_viszones();
  con_out("Recalculating viszones of window #%d", win->get_win_id());
  
  return 0;
}

void five_sec_handler()
{
  five_sec_tick++;
}
END_OF_FUNCTION(five_sec_handler);
