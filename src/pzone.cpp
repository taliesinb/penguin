#include "pzone.h"
#include "pbasewin.h"

/*
  "Clips" is set after execution of the "intersect" function, and will be:
  -1 - if the zones did not overlap,
   0 - if the passed zone was within the called zone,
   x - otherwise, the number of sides that had to be clipped,

  "Num" is a simple zone count; it is incremented in every initializer, and
  decremented in every destructor. A memory leak will yield a greater-than-zero
  zone count.
*/
int zone::count = 0;
int zone::clips = 0;

// Checks (this) against (other) for overlap. Returns -1 if none, otherwise number of
// conflicting sides. Assumes (other) != NULL.
int zone::check_intersect(const zone* other) const
{
  if (other->ax > bx || other->bx < ax || other->ay > by || other->by < ay)
    return -1;

  return ((other->ax < ax) ? 1 : 0) +
         ((other->ay < ay) ? 1 : 0) +
         ((other->bx > bx) ? 1 : 0) +
         ((other->by > by) ? 1 : 0);
}

// Returns list of dubbed zones identical to our list from this point onwards.
zone* zone::duplicate() const
{
  zone* loop = next;
  zone* start = new zone(*this);
  zone* cur = start;

  while (loop)
  {
    cur->next = new zone(*loop);

    cur = cur->next;
    loop = loop->next;
  }

  return start;
}

// Constructor that initializes to (win)'s physical co-ords. Assumes (win) != NULL.
zone::zone(const base_window* win)
: next(0)
{
  ax = win->get_cx();
  ay = win->get_cy();
  bx = win->get_dx();
  by = win->get_dy();
  count++;
}

// Returns zone just before (other) in (this) list, and NULL if not found.
zone* zone::find(const zone* other)
{
  zone* step = this;
  
  while (step->next != other && step) step = step->next;

  return step;
}

// Offsets each zone in this list by the specified x and y offsets
void zone::offset_list(coord_int x, coord_int y)
{
  for (zone* loop = this; loop; loop = loop->next)
  {
    loop->ax += x;
    loop->ay += y;
    loop->bx += x;
    loop->by += y;
  }
}

/* This function must compare every zone in its list against every zone in the mask list.
 * For any zones it finds that overlap with a mask zone, it will subdivide the old zone
 * into 4 or less new zones that hug the sides of the mask zone. Pretty safe. */
 
void occlude(zone*& list, const zone* mask)
{
  zone* next_vis; // Temporary for incrementing a pointer afer deleting its zone

  for (; mask; mask = mask->next) 
  {
    for (zone* vis = list; vis; vis = next_vis)
    {
      // Make sure we skip over any freshly-inserted zones, and to avoid reading (vis->next)
      // after vis has been deleted.
      next_vis = vis->next;

      // If the mask doesn't overlap the vis_zone, skip it
      if (vis->bx < mask->ax || vis->by < mask->ay || vis->ax > mask->bx || vis->ay > mask->by)
        continue;

      int mask_ay = mask->ay; // The mask co-ords might need to be clipped to the top/bottom
      int mask_by = mask->by; // of the vis_zone so we'll store them in some temp variables

      // Here we will check whether the sides of the mask go beyond the sides of the vis_zone,
      // clipping them if they DO, and if they DON'T, of course, we need to subdivide on that
      // side so we create a new zone for that portion.
      if (mask_ay <= vis->ay) mask_ay = vis->ay; else vis->shift_zone(new zone(vis->ax, vis->ay, vis->bx, mask_ay - 1));
      if (mask_by >= vis->by) mask_by = vis->by; else vis->shift_zone(new zone(vis->ax, mask_by + 1, vis->bx, vis->by));
      if (mask->ax > vis->ax) vis->shift_zone(new zone(vis->ax, mask_ay, mask->ax - 1, mask_by));
      if (mask->bx < vis->bx) vis->shift_zone(new zone(mask->bx + 1, mask_ay, vis->bx, mask_by));

      // If we are deleting the first vis_zone, we need to update the 'first' variable
      if (vis == list)
      {
        list = vis->next;
      } else {
        // Otherwise we just 'stitch' the linked list to avoid the "doomed" zone
        zone* last = list;
        while (last->next != vis) last = last->next;
        
        last->next = vis->next;
      }

      // Delete the zone that has just been occluded
      delete vis;
    }
  }
}

// This function returns a new zone initialized with the co-ordinates of the intersection
// between (this) and (other). If there is none, NULL is returned. Assumes (other) != NULL.
zone* zone::intersect(const zone* other) const
{
  // If the other zone doesn't overlap us, leave it.
  if (other->ax > bx || other->bx < ax || other->ay > by || other->by < ay)
  {
    zone::clips = -1;
    return 0;
  }

  zone::clips = 0;

  // Clip the other zone to our dimensions and return the result
  return new zone((other->ax < ax) ? (++zone::clips, ax): other->ax,
                  (other->ay < ay) ? (++zone::clips, ay): other->ay,
                  (other->bx > bx) ? (++zone::clips, bx): other->bx,
                  (other->by > by) ? (++zone::clips, by): other->by);
}

// Clips (this) to a maximum defined by (other), returns collisions, -1 if no overlap.
int zone::clip(const zone* other)
{
  int clips = 0;

  // If the other zone doesn't overlap us, leave it.
  if (other->ax > bx || other->bx < ax || other->ay > by || other->by < ay)
  {
    return -1;
  }

  // Clip the other zone to our dimensions and return the number of required clips.
  ax = (other->ax < ax) ? (++clips, ax): other->ax;
  ay = (other->ay < ay) ? (++clips, ay): other->ay;
  bx = (other->bx > bx) ? (++clips, bx): other->bx;
  by = (other->by > by) ? (++clips, by): other->by;

  return clips;
}

// Deprecated function that removes (victim) from (this) list.
zone* zone::remove(zone* victim)
{
  if (victim == this)
  {
    zone* temp = next;
    delete this;
    
    return temp;
  }

  zone* loop = this;
  while (loop && (loop->next != victim)) loop = loop->next;

  if (loop && loop->next == victim)
  {
    loop->next = victim->next;
    delete victim;
  }

  return this;
}

// std::ostream operator to print passed zone followed by others in its list.
std::ostream& operator<<(std::ostream& os, const zone& z)
{
  os << '[' << z.ax << ',' << z.ay << '|' << z.bx << "," << z.by << ']';

  int limit = 64;
  zone* cur = z.next;
  while (cur && --limit)
  {
    os << ":[" << cur->ax << ',' << cur->ay << '|' << cur->bx << "," << cur->by << ']';
    cur = cur->next;
  }
  
  return os;
}
