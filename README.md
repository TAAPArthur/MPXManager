# My Personal XWindow Manager (MPX Manager)

## Intro
MPX Manager started off as a project to fill void of MPX client window managers. Before its creation I tried to hack MPX support into existing window managers namely Xmonad. For reasons I ended up leaving Xmonad and exploring other WMs (mainly Franklen) and compiling all the MPX hacks into their own independent projects with the goal being able to work with any window manager. One of the features was using setting border color of the active window depending on which master device had focus. This involved telling the WM to not set the border color itself. In both Xmonad and Franlen, this was a ~1 line change, but the thought occured to me: why should I have to dig through the source code to make a simple one line change in these supposed customizable WMs? Clearing the level of customization the developers planned did not meet my expectation. So I decided to create MPX Manager.

# Design Goals
## Full MPX Support
MPX Manager was designed from the ground up to support a variable number of master devices. By default master devices operate independently and do not interfere with each other whenever possible.



## Configurable
We strived to make as few assumptions as possible while still be compliant with the EWMH/ICCCM spec. Any ones we use can be tuned with a variable in the configuration file. For the changes we did account, the code base is well documented to make hacks as painless as possible

## Non Goals
We did not make it a priority to be small or efficient

## Features
  * MPX Support
  * Workspaces
    *  Windows can be in multiple workspaces
  * Multi-monitor (Xrandr)
  * EWMH/ICCCM compliment
  * Key/mouse bindings
      * On Press and release
      *  Chain bindings
  *  Hooks for all events (X11 and custom)
  *  Sane defaults
  *  Scriptable (TODO)


## Missing Features
  * Advanced layouts
  * Live config editing (needs to be recompiled)



## Features
### Event Hooks
We all users to add run arbitrary function when certain events (either standard X11 events or changes in out internal state) are detected.

## Key/Mouse Bindings
When a designed key/modifer or button/modifer combination (henceforth a Binding) is pressed or released, a desired function will be called. 
In addition, you can specify combination to signal the start of a "chain." A chain is an array of Bindings that get processed before all other bindings. When the last member of the chain is called (by default this will be any other key is pressed), the chain ends and any grabs assosiated with the chain are released
