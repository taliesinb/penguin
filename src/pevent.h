#ifndef EVENT_H
#define EVENT_H

#include <list>    // For ll of senders and receivers
#include <typeinfo>
#include <string>

class event_participant;
struct event_info;

// The event-participant callback type
typedef void (event_participant::*part_method)(const event_info&);
typedef void (event_participant::*void_part_method)();

/* Helper macros. 
  
   The first one is used to generate a listener pattern for the 
   "listen(window, ...pattern...)" call. It is passed the name of the class
   wanting to listen, the method to call, and the type to use. It will ignore
   type discrepancies. 
   
   The second one creates a pattern, but where the type of the callback takes
   no arguments. This is for conveneinces sake.
   
   The third is almost the same as the first, except it will result in an
   error if the type being listened to and the signature of the callback are 
   in any way different (even if derived from the other and thus valid).
  
   The fourth one is used by event-info objects to build its RTTI matching system */

#define UNSAFE_LISTENER(a, c) reinterpret_cast<part_method>(&a), typeid(c)
#define VOID_LISTENER(a, c) static_cast<void_part_method>(&a), typeid(c)
#define LISTENER(a, c) (part_method)static_cast<void (event_participant::*)(const c &)>(&a), typeid(c)
#define DEFINE_EI(our, parent) bool match(const std::type_info& t) const { return (typeid(our) == t) ? true : parent::match(t); } std::string type() const { return (parent::type() + "::" + #our) ; }

/* Class that acts as a mediator between an event_participant that will generate
   events, and another participant that will accept them through a callback. The
   events will be passed as an event_info class, that can be derived to 
   incorporate more specific information for certain types of event */
   
class event_knot
{
  private:

    event_participant& sender; // Participant who issues the events
    event_participant& receiver; // Participant who receives the events
    
    enum 
    {
      normal_method,
      void_method,
    } method_type;      
    
    union
    {
      part_method receiver_method; // The function to call on the receiver
      void_part_method void_receiver_method;
    };
    
    const std::type_info& model; // The type the receiver wants to listen to

  public:
    
    // Initializer to bind together an eh and a window
    event_knot(event_participant& s, event_participant& r, part_method m, const std::type_info& t);
    event_knot(event_participant& s, event_participant& r, void_part_method m, const std::type_info& t);
    ~event_knot();

    // Check if the receiver wants this type of event. If so, call the callback
    void issue(const event_info& info); 
    
    // Retrun true if we match this description
    bool is(const event_participant& s, const event_participant& r, part_method p, const std::type_info& m) const;
    bool is(const event_participant& s, const event_participant& r, void_part_method p, const std::type_info& m) const;    
  
    static int count;
};

/* This is a structure representing an event that has occured to a particular
   participant. It has a pointer to the participant who issued the event in the
   first place, and can return this pointer in the form of a reference. It can
   yield a std::string representing its type (only interesting for child classes).
   It can tell when it is confronted with a type_info() object that describes
   an event_info form that it is derived from. It can be refined to include more
   information on the type of event, building up a 'tree' of event_info classes
   that can be used by the client to listen to specific types or categories of
   events */
 
class base_window;
 
struct event_info
{
  private:
  
    mutable event_participant* origin; // The object that generated this event

  public:
  
    event_participant* get_origin() const { return origin; }
  
    // Returns a std::string detailing this event's type and ancestry
    virtual std::string type() const { return "event_info"; } 
  
    // Returns true if we are an instance of 't' or descended from 't'
    virtual bool match(const std::type_info& t) const 
    { return (typeid(event_info) == t) ? true : false; }
                                 
    // Returns a reference to the window that generated this event
    virtual base_window& source() const; 
  
    virtual ~event_info() { } // Empty destructor

  friend class event_participant;
};

// Saves typing: 
typedef std::list<event_knot*> knot_list; // List of knot-pointers is a knot_list
typedef std::list<event_knot*>::iterator knot_iterator; // Iterator of a knot_list

/* This class represents an interface that should exist for any type that wishes
   to send and/or receive objects. It can listen to custom events, and can also 
   be listened to. When an event occurs to it that it would like to warn other 
   participants about, it can use 'transmit' to alert any interested parties... */
   
class event_participant
{
  private:

    knot_list knot_send_list;    // A list of knots who we might transmit to
    knot_list knot_receive_list; // A list of knots who might send events to us
    bool invalidate;

  public:

    event_participant() : invalidate(false) { }
    virtual ~event_participant(); // This destroys any knots linked to us
        
    void clear_send_list();    // This clears off any ties to listening knots
    void clear_receive_list(); // This clears off any ties to receiving knots
    
    // These are used by the event_knot class to register/deregister itself
    void tie_knot_to(event_knot* knot);
    void untie_knot_to(event_knot* knot);
    void tie_knot_from(event_knot* knot);                                 
    void untie_knot_from(event_knot* knot);    
  
    /* This registers that we would like to receive events from a particular 
       participant, through the method 'func', with any events that match the 
       given model. For simplicities sake, macros can be used to do all the 
       casting - See above. */ 
       
    void listen(event_participant& sender, part_method func, const std::type_info& model);
    void listen(event_participant& sender, void_part_method func, const std::type_info& model);
    
    /* The opposite of listen. It de-allocates any previous request to listen to
       a particular sender for the particular event to the particular function. */
         
    void forget(event_participant& sender, part_method func, const std::type_info& model);
    void forget(event_participant& sender, void_part_method func, const std::type_info& model);

    // Infrom interested parties that 'ei' occured.
    void transmit(const event_info& ei)
    {
      invalidate = false;
      ei.origin = this; // Set the event to point to us as its origin
      for (knot_iterator i = knot_send_list.begin(); i != knot_send_list.end() && !invalidate; i++)
        (*i)->issue(ei); // Issue the event to each knot in the send list.
    }
    void transmit(const event_info& ei, base_window* win);
};

#endif
