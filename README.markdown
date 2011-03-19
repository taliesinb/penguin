#README

## About *penguin*	

Penguin was a project I embarked on at the tender age of 13. I wanted a meaty C++ project that involved a strong object heirarchy and algorithms I would have to design myself. I picked a GUI toolkit because I was developing games using the Allegro graphics library under DOS (using the DJCPP compiler) and I wanted a powerful GUI to enrich my other projects.

I invented the fragment- and window-management code myself in a bottom-up design kind of way. I based the event system and layout management on Java's SWING.

The resulting system took me about a year to write. While single-threaded, it was performant enough to run under DOS, Windows, and Linux using only Allegro's software-rendering. It could support transparency, drop-shadows, and all kinds of effects. 

I based the look-and-feel on Windows 98, which I was using at the time, but I pursued skinning to emulate MacOS's Aqua look-and-feel as well. 

I wrote a 200-page Word document that detailed the design and implemntation of the system, which you can find [here](http://dl.dropbox.com/u/2295398/penguin-manual.pdf)

## Screenshots

For some screenshots, see the [wiki](https://github.com/taliesinb/penguin/wiki).

## Original README

Here was the original README, though I never got round to putting it on SourceForge, as was my plan:

    What is Penguin?
    
    Penguin is a simple GUI library, with its own display routines, callback system, and widget classes. It is meant to function as a "lightweight" GUI system for any platforms supporting the Allegro graphics library -these include Windows, DOS, Linux, MacOS and BeOS. It supports basic 'cool stuff' like transparency and masked images, and sports a reasonable set of widgets that should suffice for common usage.
    
    What Penguin is not:
    
    It is not thread-safe, and all callback functions execute from within the GUI management code. It does not have an extensive widget set, just enough to get going with. It does not feature multi-platform graphics (pluggable look 'n feel). It is obviously not competing with the vast arena of proffesional-level projects that are in one way or another associated with Graphical User Interfaces - the primary function of Penguin was to provide the author with a learning experience, and to gain some technical knowledge in C++ (which it did). I decided to release it in case it turned out to be useful to anyone who was interested in a non-native GUI for use in an Allegro environment.     
    On the Penguin wish-list:
    - Dockable widgets to use with a toolbar.
    - Further set of 'bind' template functions as in GTK's GCode.





