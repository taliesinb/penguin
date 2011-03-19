#ifndef PSUBLIM_H
#define PSUBLIM_H

class window_sub;
class masked_image;
struct BITMAP;

#include "pbasewin.h"
#include "pevent.h"
#include "math.h"

/* This class acts as a base for all windows which wish to implement transparency
 * in some way, whether this be a 'masked image' where the background shows through,
 * or a magnifier, an irregularly-shaped window, or a semi-transparent box.
 */
class window_sub : public base_window
{
  private:
                      
    BITMAP *sub_buffer; // The bitmap containing the image of what is beneath us
  
    // Helper function to resize the sub-buffer to a new width and height. 
    // Automatically recalculates the sub-buffer if necessary.
    void resize_sub_buffer(coord_int _x, coord_int _y);    
    
  protected:

    // Flag indicating whether a change in the sub-buffer requires the entire
    // window to be redisplayed, or merely the affected portion. It is the 
    // responsibility of the derived classes to set this to true, it is false by
    // default.
    bool linear; 

    // Hook for inherited classes - called whenever the sub-buffer is altered
    virtual void sub_changed_hook(const zone* changed_list) { }

    // Overridden hook to re-allocate the sub-buffer when the our size changes,
    // and recalculate the sub-buffer when we are moved.
    void move_resize_hook(const zone& old_pos, bool was_moved, bool was_resized);
    
    void post_load();
    void pre_unload();

    window_sub() 
    : sub_buffer(0), linear(false)
    { set_flag(grx_subliminal); }
        
  public:
  
    void update_sub(); // Recalculates the entire sub-buffer
    
    // Helper function - called whenever another window draws to our sub-buffer
    void sub_buffer_updated(zone* list); 
    
    bool is_linear() { return linear; }        
    BITMAP* get_sub_buffer() { return sub_buffer; }
};

/* This is a portable magnifier to enlarge a portion of the screen under it. */
class window_magnifier : public window_sub
{
  protected:
  
    void event_mouse_down(bt_int button); 
    void event_mouse_up(bt_int button);
    void event_mouse_drag(coord_int x, coord_int y);
    void event_key_down(kb_event key);

    void draw(const graphics_context& grx);
    
    drag_helper dragger; 
    int mode; // Represents the particular mode we are in

  public:

    window_magnifier() : mode(0) { }
  
    void set_mode(int m)
    {
      mode = m;
      display();
    }
};

/* There are 3 different types of masked images, each using different approaches
 * to render the image to the screen. Here is a description of the 3 types:
 *  
 * MASKED_IMAGE:
 *
 * Whenver the sub_buffer of this window is changed or updated, the masked
 * image will be blitted onto the sub_buffer. This way, when the sub_buffer
 * is blitted to the screen, it will automatically contain both the masked image
 * and the underlying background. This has advantages and disadvantages, of course.
 * On the plus side, it is very fast, on a par with a normal image blitting for
 * straight display; but on the minus side, its sub_buffer must be updated every
 * time the masked image is changed, and furthermore the sub_buffer will no longer
 * just represent the regions underlying this window, ie, the sub_buffer is no
 * longer "pure".
 *
 * RLE_MASKED_IMAGE:
 *
 * This will create an RLE copy of the provided bitmap and store it internally.
 * It is for all intents and purposes the same as a locked image, but is marginally
 * faster on redisplays and sub-buffer re-renders, as well as having a smaller
 * memory footprint (depending on the source image). However, this only counts if
 * the original source image is then freed, because unlike all the other masked_image
 * windows, this window creates its own RLE bitmap which will obviously add to the
 * memory required - the RLE plus the source is more than just the source.
 *  
 * SHADOWED_MASKED_IMAGE
 *
 * This masked image maintains a 'shadow buffer', a bitmap containing shadow information
 * about the current image. Solid parts of the image will 'cast a shadow' into the
 * shadow bitmap, using the given x and y offsets and shadow strength. The shadow 
 * bitmap is then used to darken parts of the final image prior to display.
 */
class masked_image : public window_sub
{
  private:
  
    void draw(const graphics_context& grx); 
 
  protected:

    BITMAP *image; // Our current image

    void pre_load();
    bool pos_visible(coord_int x, coord_int y) const; // Checks for opaque-ness
    void sub_changed_hook(const zone* changed_list); // Blits image over sub-buffer

  public:

    coord_int normal_w() const;
    coord_int normal_h() const;

    masked_image(BITMAP* i); // Load image from existing image (copies)
    masked_image(std::string file); // Load image from file
    masked_image(); // No image
    ~masked_image(); // Delete associated image

    void load(BITMAP* bmp); // Load image from existing image (copies)
    void load(std::string file); // Load image from file
    virtual void set_image(BITMAP *_image); // Load image from existing image (points)

    BITMAP* get_image() { return image; } // Returns current image
};

class shadowed_masked_image : public masked_image
{
  private:

    BITMAP* shadow_mask; // Bitmap representing the shadow to be applied
    unsigned char shadow_level; // 'Darkness' of shadow. 255 = none, 0 = black
    unsigned char x_offset; // Point at which the shadow appears, horizontally
    unsigned char y_offset; // Point at which the shadow appears, vertically

  protected:
  
    void sub_changed_hook(const zone* changed_list);
    
  public:
  
    // Constructor: takes depth of shadow, x-offset and y-offset
    shadowed_masked_image(unsigned char sl=200, unsigned char x_off =4, unsigned char y_off =1);

    // When a new image is set, recalculates the shape of the shadow
    void set_image(BITMAP *_image); 
};

struct RLE_SPRITE;
class rle_masked_image : public masked_image
{
  private:

    RLE_SPRITE* rle_image;

  protected:
  
    void sub_changed_hook(const zone* changed_list);

  public:

    ~rle_masked_image();

    rle_masked_image(BITMAP* _image) 
    : masked_image(_image), rle_image(0)
    { }
    
    rle_masked_image(std::string file)
    : masked_image(file), rle_image(0)
    { }

    void set_image(BITMAP *_image);
};

#endif
