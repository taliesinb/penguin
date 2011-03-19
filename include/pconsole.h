#ifndef PCONSOLE_H
#define PCONSOLE_H

class window_manager;
class base_window;
struct FONT;

void do_console(window_manager* man, base_window* gui, FONT* _font);
void console_init();

#endif
