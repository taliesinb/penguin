#include "psublim.h"

#include <stdlib.h>
#include <math.h>

#include "pgrx.h"
#include "allegro.h"
#include "pmaster.h"

/* This simple function is called whenever a subliminal window's sub-buffer is
 * changed at all. The 'sub_changed_hook' function is called, and then the 
 * window is displayed. If linear, only the portion that was changed is displayed,
 * otherwise, the entire window is redrawn.
 */
void window_sub::sub_buffer_updated(zone* list)
{
  sub_changed_hook(list); // Call the hook

  if (flag(grx_sensitive) && list) 
  {
    // If this subliminal window is a linear, just display the effected areas by
    // calling 'draw_arb_zones' with no recursion flags
    if (linear) 
    {
      list->offset_list(get_cx(), get_cy());
      draw_arb_zones(list, DAZ_F_SPYSUB);
    } else display(); // If it isn't linear, display the entire window.
  }
}

/* This function rerenders the subliminal window's entire sub_buffer. It loops
 * through all inferior windows, calling their 'draw_to_sub' function to get them
 * to draw onto its own sub_buffer. After doing this, it alerts any derived
 * classes's hooks.
 */
void window_sub::update_sub()
{
  // If we are not capable of having our sub-buffer updated, return
  if (!sub_buffer || !visible() || !flag(sys_active)) return;

  // If we are at all clipped, clear our sub-buffer so the clipped portions don't
  // get filled with junk. 
  if (flag(vis_positive_clip) || flag(vis_negative_clip)) clear_to_color(sub_buffer, 0);
  
  // Loop through all inferior windows that we are visible to us
  for (base_window* loop = get_parent(); loop && loop != this; loop = loop->superior())
  {
    if (loop->visible() && loop->flag(sys_active))
    {
      // If they are visible and active, draw themselves to us
      loop->draw_to_sub(this, &loop->clipped(), false);
    }
  }

  // Call the hook to inform it our entire surface has been drawn to!
  sub_changed_hook(&zone(0, 0, w(), h()));
}

void window_sub::post_load()
{
  sub_buffer = 0;
  resize_sub_buffer(w(), h());
}

void window_sub::pre_unload()
{
  if (sub_buffer) 
  {
    destroy_bitmap(sub_buffer);
    sub_buffer = 0;
  }
}

void window_sub::resize_sub_buffer(coord_int w, coord_int h)
{
  if (sub_buffer) destroy_bitmap(sub_buffer);

  sub_buffer = create_bitmap_ex(bitmap_color_depth(get_master()->get_buffer()), w+1, h+1);     
  Assert(sub_buffer, "Error allocating new sub-buffer for window " << this);

  update_sub();
}

/* Potential optimisation here: on a move, offset the original sub-buffer by the
   amount moved, and only re-calculate the portions that are now null. Also, on
   a resize, only recalculate the 'extra' space created, if any. */

void window_sub::move_resize_hook(const zone& old_pos, bool was_moved, bool was_resized)
{
  if (was_resized) resize_sub_buffer(w(), h());
  else update_sub();
}

void distort_blit(BITMAP* src, BITMAP* dst, int sx, int sy, int dx, int dy, int w, int h)
{
  float k = h / 6.2831 / 2;

  for (int y = 0; y < h; y++)
  {
    float v = (cos(y / k) + 1) * (w / 5);
    stretch_blit(src, dst, int(sx + v/2), sy + y, int(w - v), 1, dx, dy + y, w, 1);
  }
}

masked_image::~masked_image()
{
  if (image) destroy_bitmap(image);
}

coord_int masked_image::normal_w() const
{
  return image ? image->w - 1 : 0;
}

coord_int masked_image::normal_h() const
{
  return image ? image->h - 1 : 0;
}

masked_image::masked_image(std::string file)
: image(0)
{
  set_flag(sys_always_resize);

  PALETTE pal;
  image = load_bitmap(file.c_str(), pal);
}

masked_image::masked_image(BITMAP* bmp)
: image(0)
{
  set_flag(sys_always_resize);

  if (bmp)
  {
    image = create_bitmap_ex(bitmap_color_depth(bmp), bmp->w, bmp->h);
    if (image) blit(bmp, image, 0, 0, 0, 0, bmp->w, bmp->h);
  }
}

masked_image::masked_image()
: image(0)
{
  set_flag(sys_always_resize);
}

void masked_image::set_image(BITMAP *_image)
{
  if (image = _image) resize(image->w-1, image->h-1);
}

bool masked_image::pos_visible(coord_int x, coord_int y) const
{
  int p = getpixel(image,x-get_cx(),y-get_cy());
  if (p == -1 || p == bitmap_mask_color(image)) return false;

  return true;
}

void masked_image::load(BITMAP* bmp)
{
  if (image) destroy_bitmap(image);
  
  if (bmp)
  {
    image = create_bitmap_ex(bitmap_color_depth(bmp), bmp->w, bmp->h);
    if (image) blit(bmp, image, 0, 0, 0, 0, bmp->w, bmp->h);
  }  
}

void masked_image::load(std::string file)
{
  if (image) destroy_bitmap(image);
  
  PALETTE pal;
  image = load_bitmap(file.c_str(), pal);
  
  if (flag(sys_loaded)) set_image(image);
}

void masked_image::sub_changed_hook(const zone* list)
{
  if (image)
  {
    for (; list; list = list->next)
    {
      masked_blit(image, get_sub_buffer(), list->ax, list->ay, list->ax, list->ay, list->w()+1, list->h()+1);
    }
  }
}

void masked_image::draw(const graphics_context& grx)
{
  grx.blit(get_sub_buffer(), 0, 0, 0, 0, w()+1, h()+1);
}

void masked_image::pre_load()
{
  set_image(image);
}                    

void shadowed_masked_image::sub_changed_hook(const zone* list)
{
  if (image)
  {
    set_multiply_blender(0,0,0,255);
    
    for (; list; list = list->next)
    {
      masked_blit(image, get_sub_buffer(), list->ax, list->ay, list->ax, list->ay, list->w() + 1, list->h() + 1);
  
      if (shadow_mask)
      {
        bmp_clip(get_sub_buffer(), list->ax, list->ay, list->bx, list->by);
        draw_trans_sprite(get_sub_buffer(), shadow_mask, x_offset, y_offset);
        bmp_clip(get_sub_buffer(), 0, 0, get_sub_buffer()->w, get_sub_buffer()->h);
      }
    }
  }
}

shadowed_masked_image::shadowed_masked_image(unsigned char sl, unsigned char x_off, unsigned char y_off)
: masked_image(0), shadow_mask(0), shadow_level(sl), x_offset(x_off), y_offset(y_off)
{ }

void shadowed_masked_image::set_image(BITMAP *_image)
{
  if (image = _image)
  {  
    if (bitmap_color_depth(image) != 8)
    {
      if (shadow_mask) destroy_bitmap(shadow_mask);
      shadow_mask = create_bitmap_ex(bitmap_color_depth(image), image->w, image->h);
  
      clear_to_color(shadow_mask, bitmap_mask_color(shadow_mask));
  
      set_trans_blender(shadow_level, shadow_level, shadow_level, 0);
      draw_lit_sprite(shadow_mask, image, 0, 0, 255);
      set_trans_blender(255, 0, 255, 0);
      draw_lit_sprite(shadow_mask, image, -x_offset, -y_offset, 255);
    } else
    {
      if (shadow_mask) destroy_bitmap(shadow_mask);
      shadow_mask = 0;
    }
  
    resize(image->w - 1 + x_offset, image->h - 1 + y_offset);
  } 
}                                                       
    
void rle_masked_image::set_image(BITMAP* _image)
{
  if (_image)
  {  
    if (rle_image) destroy_rle_sprite(rle_image);
    if (image) rle_image = get_rle_sprite(image); else rle_image = 0;
  
    resize(image->w - 1, image->h - 1);
  } else
  {
    if (rle_image) destroy_rle_sprite(rle_image);
    rle_image = 0;
  }
}

void rle_masked_image::sub_changed_hook(const zone* list)
{
  if (rle_image) draw_rle_sprite(get_sub_buffer(), rle_image, 0, 0);
}

rle_masked_image::~rle_masked_image()
{ 
  if (rle_image) destroy_rle_sprite(rle_image); 
}


void distort_blit(BITMAP* src, BITMAP* dst, int sx, int sy, int dx, int dy, int w, int h);
void reversed_blit(BITMAP* src, BITMAP* dest, int sx, int sy, int dx, int dy, int w, int h);

void window_magnifier::draw(const graphics_context& grx)
{
  switch (mode)
  {
    case 0: // Zoom mode
         
      grx.draw_frame(0,0,w(),h(), ft_bevel_out);
      grx.stretch_blit(get_sub_buffer(), (w()-3) / 4, (h()-3) / 4, (w()-3) / 2, (h()-3) / 2, 0+2, 0+2, w()-3, h()-3);
      break;
     

    case 1: // Reverse mode
     
      grx.draw_frame(0,0,w(),h(), ft_bevel_out);    
      reversed_blit(get_sub_buffer(), grx, 2, 2, 2+grx.get_ox(), 2+grx.get_oy(), w()-3, h()-3);    
      break;
     

    case 2: // Distort mode
     
      grx.draw_frame(0,0,w(),h(), ft_bevel_out);
      distort_blit(get_sub_buffer(), grx, 2, 2, 2+grx.get_ox(), 2+grx.get_oy(), w()-3, h()-3);
      break;
     
 
    case 3: // Plain mode
     
      grx.rect(0, 0, w(), h(), theme().black);    
      grx.blit(get_sub_buffer(), 1, 1, 1, 1, w()-1, h()-1);
      break;
     
    
    case 4: // Debug mode

      grx.rectfill(0, 0, w(), h(), RANDOM_COLOR());
      break;
  }
}

void window_magnifier::event_key_down(kb_event key)
{
  switch (key.get_char())
  {
    case '1': set_mode(0); break;
    case '2': set_mode(1); break;
    case '3': set_mode(2); break;
    case '4': set_mode(3); break;
    case '5': set_mode(4); break;
  }
}

void window_magnifier::event_mouse_down(bt_int button)
{
  set_keyfocus();
  dragger.down(this);
  if (button & bt_left) z_shift(z_front);
}

void window_magnifier::event_mouse_up(bt_int button)
{
  dragger.up(this);
}

void window_magnifier::event_mouse_drag(coord_int x, coord_int y)
{
  dragger.drag(this, x, y);
}   

