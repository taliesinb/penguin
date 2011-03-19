#ifndef PUTIL_H
#define PUTIL_H

#include <sstream>
#include "pdefs.h"

template<typename T>
T fromString(const std::string& s) {
  std::istringstream is(s);
  T t;
  is >> t;
  return t;
}

template<typename T>
std::string toString(const T& t) {
  std::ostringstream s;
  s << t;
  return s.str();
}   

compass_orientation get_dir(int w, int h);

int get_x(compass_orientation c);
    
int get_y(compass_orientation c);
cursor_name compass_to_cursor(compass_orientation c);
#endif
