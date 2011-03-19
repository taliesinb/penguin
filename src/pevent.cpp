#include "pevent.h"
#include "pbasewin.h"
#include "allegro.h"         

int event_knot::count = 0;

void event_participant::listen(event_participant& sender, part_method func, const std::type_info& model)
{
  new event_knot(sender, *this, func, model);
}

void event_participant::listen(event_participant& sender, void_part_method func, const std::type_info& model)
{
  new event_knot(sender, *this, func, model);
}

void event_participant::forget(event_participant& sender, part_method func, const std::type_info& model)
{
  for (knot_list::iterator r = knot_receive_list.begin(); r != knot_receive_list.end(); r++)
  {
    if ((*r)->is(sender, *this, func, model)) 
    {
      delete *r;
      break;
    }
  }
}

void event_participant::forget(event_participant& sender, void_part_method func, const std::type_info& model)
{
  for (knot_list::iterator r = knot_receive_list.begin(); r != knot_receive_list.end(); r++)
  {
    if ((*r)->is(sender, *this, func, model)) 
    {
      delete *r;
      break;
    }
  }
}

void event_participant::transmit(const event_info& ei, base_window* win)
{
  ei.origin = win;
  for (knot_iterator i = knot_send_list.begin(); i != knot_send_list.end(); i)
    (*i)->issue(ei); // Issue the event to each knot in the send list.
}

// Event_participant destructor:
event_participant::~event_participant()
{
  clear_send_list();    // Delete all the event_knots in our send-list
  clear_receive_list(); // Delete all the event_knots in our receive-list
}

base_window& event_info::source() const
{
  return *dynamic_cast<base_window*>(origin);
}

void event_participant::clear_send_list()
{
  invalidate = true;
  // While our send-list is not empty, delete the first knot in the list.
  while (!knot_send_list.empty()) delete knot_send_list.front();
}

void event_participant::clear_receive_list()
{
  invalidate = true;
  // While our receive-list is not empty, delete the first knot in the list.
  while (!knot_receive_list.empty()) delete knot_receive_list.front();
}

// This function attempts to remove the given knot pointer from the knot_send_list
void event_participant::untie_knot_to(event_knot* knot)
{
  invalidate = true;
  Assert(!knot_send_list.empty(), "Untie_knot_to: Send list exhausted on window" << this);

  for (knot_list::iterator r = knot_send_list.begin(); r != knot_send_list.end(); r++)
  {
    if (*r == knot) 
    {
      knot_send_list.erase(r);
      break;
    }
  }
}

// This function attempts to remove the given knot pointer from the knot_receive_list
void event_participant::untie_knot_from(event_knot* knot)
{
  invalidate = true;
  Assert(!knot_receive_list.empty(), "Untie_knot_from: Receive list exhausted on window" << this);

  for (knot_list::iterator r = knot_receive_list.begin(); r != knot_receive_list.end(); r++)
  {
    if (*r == knot) 
    {
      knot_receive_list.erase(r);
      break;
    }
  }
}

// This function adds the given knot pointer to the knot_send_list at the end
void event_participant::tie_knot_to(event_knot* knot)
{
  knot_send_list.push_back(knot);
}

// This function adds the given knot pointer to the knot_receive_list at the end
void event_participant::tie_knot_from(event_knot* knot)
{
  knot_receive_list.push_back(knot);
}

// Event knot constructor:
event_knot::event_knot(event_participant& s, event_participant& r, part_method m, const std::type_info& t)
: sender(s), receiver(r), receiver_method(m), model(t), method_type(normal_method)
{
  event_knot::count++;
  
  sender.tie_knot_to(this);     // Bind ourselves to the sender's knot_send_list
  receiver.tie_knot_from(this); // Bind ourselves to the receiver's knt_receive_list
}

event_knot::event_knot(event_participant& s, event_participant& r, void_part_method m, const std::type_info& t)
: sender(s), receiver(r), void_receiver_method(m), model(t), method_type(void_method)
{
  event_knot::count++;
  
  sender.tie_knot_to(this);
  receiver.tie_knot_from(this);
}

// Event knot destructor:
event_knot::~event_knot()
{
  event_knot::count--;
  
  sender.untie_knot_to(this);     // Remove this knot from the sender's knot_send_list
  receiver.untie_knot_from(this); // Remove this knot from the receiver's knot_receive_list
}

bool event_knot::is(const event_participant& s, const event_participant& r, part_method p, const std::type_info& m) const
{
  if (&sender == &s && &receiver == &r && receiver_method == p && model == m && method_type == normal_method) return true;
  else return false;
}

bool event_knot::is(const event_participant& s, const event_participant& r, void_part_method p, const std::type_info& m) const
{
  if (&sender == &s && &receiver == &r && void_receiver_method == p && model == m && method_type == void_method) return true;
  else return false;
}

void event_knot::issue(const event_info& info)
{
  if (info.match(model)) // If the type to send matches the model...
  {
	  std::cout << "Matched";
    // Send it to the callback!
    if (method_type == normal_method) 
	{(receiver.*receiver_method)(info);
			  std::cout << "Sent";
	}
    else (receiver.*void_receiver_method)();
  } else {
	  std::cout << info.type() << " didn't match our type!";
  }

}


