#ifndef PUSER_H
#define PUSER_H

enum pk_modifiers
{
  pk_shift_flag = 0x0001,
  pk_ctrl_flag = 0x0002,
  pk_alt_flag = 0x0004,
  pk_lwin_flag = 0x0008,
  pk_rwin_flag = 0x0010,
  pk_menu_flag = 0x0020,
  pk_scrolock_flag = 0x0100,
  pk_numlock_flag = 0x0200,
  pk_capslock_flag = 0x0400,
  pk_inaltseq_flag = 0x0800,
  pk_accent1_flag = 0x1000,
  pk_accent2_flag = 0x2000,
  pk_accent3_flag = 0x4000,
  pk_accent4_flag = 0x8000
};
         
enum pk_keys 
{
  pk_a = 1,
  pk_b,
  pk_c,
  pk_d,
  pk_e,
  pk_f,
  pk_g,
  pk_h,
  pk_i,
  pk_j,
  pk_k,
  pk_l,
  pk_m,
  pk_n,
  pk_o,
  pk_p,
  pk_q,
  pk_r,
  pk_s,
  pk_t,
  pk_u,
  pk_v,
  pk_w,
  pk_x,
  pk_y,
  pk_z,
  pk_0,
  pk_1,
  pk_2,
  pk_3,
  pk_4,
  pk_5,
  pk_6,
  pk_7,
  pk_8,
  pk_9,
  pk_0_pad,
  pk_1_pad,
  pk_2_pad,
  pk_3_pad,
  pk_4_pad,
  pk_5_pad,
  pk_6_pad,
  pk_7_pad,
  pk_8_pad,
  pk_9_pad,
  pk_F1,
  pk_F2,
  pk_F3,
  pk_F4,
  pk_F5,
  pk_F6,
  pk_F7,
  pk_F8,
  pk_F9,
  pk_F10,
  pk_F11,
  pk_F12,
  pk_esc,
  pk_tilde,
  pk_minus,
  pk_equals,
  pk_backspace,
  pk_tab,
  pk_openbrace,
  pk_closebrace,
  pk_enter,
  pk_colon,
  pk_quote,
  pk_backslash,
  pk_backslash2,
  pk_comma,
  pk_stop,
  pk_slash,
  pk_space,
  pk_insert,
  pk_del,
  pk_home,
  pk_end,
  pk_pgup,
  pk_pgdn,
  pk_left,
  pk_right,
  pk_up,
  pk_down,
  pk_slash_pad,
  pk_asterisk,
  pk_minus_pad,
  pk_plus_pad,
  pk_del_pad,
  pk_enter_pad,
  pk_prtscr,
  pk_pause,
  pk_abnt_c1,
  pk_yen,
  pk_kana,
  pk_convert,
  pk_noconvert,
  pk_at,
  pk_circumflex,
  pk_colon2,
  pk_kanji,
  pk_lshift, 
  pk_rshift,
  pk_lcontrol,
  pk_rcontrol,
  pk_alt,
  pk_altgr,
  pk_lwin,
  pk_rwin,
  pk_menu,
  pk_scrlock,
  pk_numlock,
  pk_capslock,
  pk_max
};

/* Object for representing a key-press/release. Holds character of key affected,
 * scan-code of the key, whether it was a repeat or not, what shift-type keys
 * were held down when it was pressed, whether it is has been 'snooped' from
 * a contained window, and how many other keys were held down when it happened. */ 
 
struct kb_event
{
  private:

    int okeys; // Number of other keys held down
    long unsigned int kd; // Contains shift-flags, character, scancode, repeat..

  public:
 
    kb_event(unsigned int key, unsigned int scan, unsigned int shift, bool discard, bool repeat, int ok)
    {
     kd = (key & 127) | ((scan & 127) << 8);// | (shift << 16);    
     okeys = ok;
     if (discard) kd |= 32768;
     if (repeat) kd |= 128;
    }
  
    // Has been snooped?
    bool is_snoop() { return (kd & 32768) ? true : false; } 
    
    // Is a repeat and not a first key-press?
    bool is_repeat() { return (kd & 128) ? true : false; }
    
    // Were any other keys held down?
    bool is_only() { return !other_keys(); }
    
     // Returns number of other keys pressed
    int other_keys() { return okeys; }
    
        // Scan-code of key that was pressed/released
    int get_scan() { return ((kd >> 8) & 127); }
    
    // Returns the character portion of the event
    char get_char() { return char(kd & 127); }
    
    // Returns shift-state of the key event
    int get_shift() { return ((kd) >> 16) & 65535; }
    
    // Returns true if this represents a printable character
    bool is_char() { return get_char(); }
    
    // Returns true if this is a 'virgin' keypress
    bool is_first() { return !is_snoop() && !is_repeat(); }
          
    // Returns true if shift modifier matches flags
    int is_shift(pk_modifiers m) { return (get_shift() & m); }
    
    // Returns true if this matches the given scan-code
    bool is_scan(pk_keys k) { return get_scan() == k; }
    
    // Returns true if the corresponding shift-key was held down during the event.
    bool alt() { return is_shift(pk_alt_flag); }               
    bool shift() { return is_shift(pk_shift_flag); } 
    bool ctrl() { return is_shift(pk_ctrl_flag); }
    
    void set_snoop() { kd |= 32768; } // Used by window-manager
};

// Masks used with 'bt_int' type that describe what kind of mouse-click it was
const int bt_left = 1;  
const int bt_right = 2;
const int bt_middle = 4;
const int bt_snoop = 8;

#endif
