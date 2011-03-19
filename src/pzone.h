#ifndef PZONE_H
#define PZONE_H

#include "pdefs.h" 

class base_window;

// The zone class is a group of 4 co-ordinates representing a rectangular area. A "next"
// pointer is also built into the class to allow for easily daisy-chained "zones".
class zone
{
  public:

    coord_int ax; // Top left x, y co-ordinates.
    coord_int ay;
    coord_int bx; // Bottom right x, y co-ordinates. Should never be below ax, ay!
    coord_int by;

    zone* next;   // Next zone in list (if any).

    /*
       "Clips" is set after execution of the "intersect" function, and will be:
       -1 - if the zones did not overlap,
        0 - if the passed zone was within the called zone,
        x - otherwise, the number of sides that had to be clipped,

       "Num" is a simple zone count; it is incremented in every initializer, and
       decremented in every destructor. A memory leak will yield a greater-than-zero
       zone count after all zones have gone out of scope.
    */
    static int clips;
    static int count;

    zone() 
    : ax(0), ay(0), bx(0), by(0), next(0)
    { count++; }

    // Copy constructor
    zone(const zone& other)
    : ax(other.ax), ay(other.ay), bx(other.bx), by(other.by), next(0)  
    { count++; }

    // Deep-copy constructor: copies off the entire list pointed to by "other".
    zone(const zone* other)
    : ax(other->ax), ay(other->ay), bx(other->bx), by(other->by),
      next(other->next ? other->next->duplicate() : 0)
    { count++; }

    // Window constructor: Initialized with the physical coordinates of the window.
    zone(const base_window* win);

    // Normal constructor, takes 4 variables for all its co-ordinates
    zone(coord_int _ax, coord_int _ay, coord_int _bx, coord_int _by)
    : ax(_ax), ay(_ay), bx(_bx), by(_by), next(0)
    { count++; }

    // Zone destructor: Decrements zone count
    ~zone()
    { count--; }

    coord_int w() const { return (bx - ax); } // Calculates, returns the width
    coord_int h() const { return (by - ay); } // Calculates, returns the height
  
    void shift_zone(zone* n); // Adds (n) to the beginning of (this) list.
    void push_zone(zone* n);  // Adds (n) to the end of (this) linked list.
    zone* duplicate() const; // Returns a pointer to a deep copy of (this) list

    // Cross-references (this) with (other). See "clips" for return types.
    int check_intersect(const zone* other) const; 
    
    // Clips (this) to (other) like "intersect", but returns number of collisions.
    int clip(const zone* other);         
    
    // Returns a new zone with the co-ords of the intersection between (this) 
    // and (other). Returns NULL if none.
    zone* intersect(const zone* other) const;  
    
    // Returns TRUE if point x,y lies within this zone.
    bool intersect(coord_int x, coord_int y) const; 

    // Erases a specific zone from list, returns beginning of new list. Deprecated.
    zone* remove(zone* victim);    
    
    // Returns the zone that is just before (other) in (this) linked list.
    zone* find(const zone* other); 

    // Adds these co-ordinates to the x and y variables of all zones in this list
    void offset_list(coord_int x, coord_int y); 
    
    // Adds these co-ordinates to the x and y variables of this zone only
    void offset(coord_int x, coord_int y) { ax += x; bx += x; ay += y; by += y; }

    // Just for the sake of it. Ostream operator to print zone.
    friend std::ostream& operator<<(std::ostream&, const zone&);
};

// Sub-divides all zones in (this) list given a pointer to a list of masks.
void occlude(zone*& list, const zone* mask);     

// This function attempts to delete the zone (z) or every zone in the zone-list (z).
inline void delete_zonelist(zone* z)
{
  for (zone* loop = z, *next_loop; loop; loop = next_loop)
  {
    next_loop = loop->next;
    delete loop;
  }
}

// Returns true if the point x,y lies within the rect defined by this zone.
inline bool zone::intersect(coord_int x, coord_int y) const
{
  if (x >= ax && x <= bx && y >= ay && y <= by) return true;
  else return false;
}

// This function will attempt to push the zone (b) onto the zone-list (a). If (a) is NULL,
// it will be initialized with the pointer (b). Note - (a) is passed as a reference, and
// should be treated specially by the calling code (ie, not discarded). B CAN be NULL.
inline void push_back (zone*& a, zone* b)
{
  if (a)
  {
    zone* step = a;
    while (step->next) step = step->next;
    step->next = b;
  } else {
    a = b;
  }
}

// This function will attempt to insert b onto the beginning of a. b MUST be a pointer
// to a single (un-linked) zone.
inline void push_front(zone*& a, zone* b)
{
  b->next = a;
  a = b;
}

// This function will insert b in the zonelist a, trying to insert b in order
// of its ay variable.
inline void push_sort(zone*& a, zone* b)
{
  if (a)
  {
    zone* loop = a, *old = 0;
    while (loop && loop->ay < b->ay)
    {
      old = loop;
      loop = loop->next;
    }

    b->next = loop;

    if (old) old->next = b;
    else a = b;
  } else a = b;
}

// Adds (_next) to the end of (this) list.
inline void zone::push_zone(zone* _next)
{                                                      
  if (!_next) return;

  zone* step = this;
  while (step->next) step = step->next;

  step->next = _next;
}

// Adds (_next) right after (this) in the list.
inline void zone::shift_zone(zone* _next)
{
  _next->next = next;
  next = _next;
}

// This increments the given zone pointer, and deletes whats been left behind
inline void shift_kill(zone *& z)
{
  zone* temp = z->next; 
  delete z;              
  z = temp;
}

std::ostream& operator<<(std::ostream& os, const zone& z);

#endif
