#ifndef PDEFS_H
#define PDEFS_H

#include <iostream>

typedef short int coord_int; // Type that all co-ordinate variables should use
typedef unsigned short int flag_int; // Type that all low-level flags should use
typedef unsigned char bt_int; // Type for representing button-clicks
const coord_int coord_int_max = 32767; // Maximum value 'coord_int' can hold

enum cursor_name
{
  cursor_normal = 0, // Normal pointer
  cursor_resize_v,   // Vertical resizer (up/down)
  cursor_resize_h,   // Horizontal resizer (left/right)
  cursor_resize_l,   // Diagonal resizer (top-left/bottom-right)
  cursor_resize_r,   // Diagonal resizer (top-right/bottom_left)
  cursor_hotspot,    // 'Hand' pointer
  cursor_illegal,    // Crossed-circle symbol.
  cursor_move,       // Omnidirectional resizer (left/right/up/down)
  cursor_caret,      // Text-edit cursor.
  cursor_last        // MARKER. DON'T USE
};

enum hv_orientation
{
  hv_horizontal,
  hv_vertical
};

enum compass_orientation
{
  c_north,
  c_north_east,
  c_east,
  c_south_east,
  c_south,
  c_south_west,
  c_west,
  c_north_west,
  c_centre
};

enum compass_direction
{
  d_north,
  d_east,
  d_south,
  d_west
};

struct active_violation_exception { };
struct null_argument_exception { };
struct type_mismatch_exception { };
struct illegal_operation_exception { };
struct incomplete_state_exception { };
struct overflow_exception { };
struct underflow_exception { };
struct graphical_exception { };
          
#define Assert(a, b)    { if (!(a)) { std::cerr << "\"" #a "\" failed at " __FILE__ "(" << __LINE__ << "): " << b << std::endl; abort(); } }
#define AssertExp(a, b) { if (!(a)) { std::cerr << "\"" #a "\" failed at " __FILE__ "(" << __LINE__ << "): " << b << std::endl; throw b; } }

#include "pzone.h"

// These macros are shortcut for describing windows, zones, or bitmaps to an std::ostream
#define DOZONE(Z) "[" << Z->ax << "," << Z->ay << "-" << Z->bx << "," << Z->by << "]"
#define DOWIN(W) "<" << W->type_name() << ": " << W->get_win_id() << ">"
#define DOBMP(B) "{" << B->w << "," << B->h << ":" << B->cl << "," << B->ct << "," << B->cr << "," << B->cb << "}"

#define PRINT(a) cerr << #a << " = " << (a) << std::endl;

// These macros evaluate to A, but limited to B in the respective direction
#define PMIN(a,b) (((a) < (b)) ? (b) : (a))
#define PMAX(a,b) (((a) > (b)) ? (b) : (a))
#define PLIM(a,b,c) (((a) < (b)) ? PMAX(b, c) : PMAX(a, c))

// This macro yields a random colour
#define RANDOM_COLOR() makecol(rand() % 255, rand() % 255, rand() % 255)

#endif

