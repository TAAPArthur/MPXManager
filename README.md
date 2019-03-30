# My Personal XWindow Manager (MPX Manager)

# What
X11 window manager with [MPX (multiple pointer x)](https://wiki.archlinux.org/index.php/Multi-pointer_X) support.

## Motivation
MPX Manager started off as a project to fill void of MPX aware window managers. Before its creation I tried to hack MPX support into existing window managers namely Xmonad. Due to the limitations of hacked support and inconvenience of maintaining, I ended up leaving Xmonad and exploring other WMs. I wanted to compile all the MPX hacks into an independent project that could run in conduction with any EWMH/ICCCM compliant window manager. One of the hacks was setting the  border color of the active window depending on which master device had focus. This involved telling the WM to not set the border color itself. This was a ~1 line change, but the thought occurred to me: why should I have to dig through the source code to make a simple one line change in these supposed customizable WMs? Clearly the level of customization the developers planned did not meet my expectation. So I decided to create MPX Manager.

# Features
## Full MPX Support
MPX Manager was designed from the ground up to support a variable number of master devices. By default master devices operate independently and do not interfere with each other whenever possible.

## Configurable
We strived to make as few assumptions as possible while still be compliant with the EWMH/ICCCM spec. The assumptions we made fo user convenience can be toggled a variable in the configuration file and/or from the command line . For the changes we did account, the code base is well documented to make hacks as painless as possible

## Multi monitor support
We support having multiple monitors that each will hold one workspace.

##  EWMH/ICCCM compliment (mainly)
There is no intention to support `_NET_WM_MOVERESIZE,_NET_DESKTOP_GEOMETRY,_NET_DESKTOP_VIEWPORT,_NET_WORKAREA`
There is currently no support for  `_NET_CLIENT_LIST_STACKING`
There is currently no support for window gravity

## Key/mouse bindings
Key/mouse press/release/motion bindings.  We also support chain bindings.

## Hooks
Users can run custom commands upon any arbitrary X11 event being detected. Users can run custom commands for set points in the MPXManager source like when starting the Xsession or retiling windows

## Sane defaults
   * A useable set a layouts
   * Xmonad-like default keybindings
   * Alt-tab window cycling
   * Default dock detection
   * Auto-float dialog windows

## Scriptable
MPXManager creates and reads from a named pipe to allow arbitary clients to affect that state in addition to regular EWMH/ICCCM methods

## Documentation?
All structs/vars/methods/files have Doxygen documentation. But there is no tutorial. The code base had around ~4000 lines of useable C.




# Code Overview

## BoundFunction
Stores a function ptr and arguments to pass into the function. This is used to trigger an arbitary function on a given set of conditions. There is a `BIND` macro to easily create a BoundFunction

## Rule
Used to select a window given a string. The string can be interpreted as a class, title, type, etc  of the window and be matched case sensitive and/or via regex. When there is a match, a [BoundFunction](BoundFunction) is called

## Binding
Takes a modifier mask, a key/mouse button and a [BoundFunction](BoundFunction). When a key/button is pressed/release we scan through a list of bindings and if the mask and key/button match, we call the BoundFunction. There are options to customize on which device events will cause the button to be triggered
## Master
This struct holds info related to a [master device pair][1] device.
## Window
Represents an X11 with with some [masks](WindowMask)
## Workspace
A collection a windows associated with 0 or 1 [Monitors](#Monitor)
## Monitor
Represents an XRANDR monitor and is associated with 0 or 1 [Workspaces](#Workspace). The bounds of the monitor represent the region tileable windows in a workspace will be tiled in.
[1]:https://wiki.archlinux.org/index.php/Multi-pointer_X
## WindowStack
Every [Workspace](#Workspace) has a window stack, but so does every [Master](#Master).
### Workspace Window Stack
Lists all the windows in the workspace. Will be used to tile windows and determines which windows will be mapped on the screen.
### Master window Stack
Lists all the windows the Master has focused. Used to determine what window is focused for that Master (usually the head of the stack). Also used first when trying to find a window via findOrRaise
## WindowMask
Windows have masks that describe how they should be treated. They corrospond to many the [_NET_WM_STATE](https://standards.freedesktop.org/wm-spec/wm-spec-latest.html#idm140200472615568) options, generic window state like if the client wants it to be mapped and some user defined state like if it is floating

# Examples
You would put the following code in your config file which is located by default in $HOME/.mpxmanager/
You want some window to always be above the others. You found the window class, C, by using the `--dump` command ( or used a tool like xprop, xwininfo etc).

```
// Create a rule to match our target window
// We want a case-insensitive, literal match of the class
// We could get by without RESOURCE, but its similar to class and could easily be mistaken for it, so lets leave it in
static Rule targetWindowRule={C,CLASS|RESOURCE};

// Now always on top is not implemented in X11 so we have to be creative
// First we create a rule that will match every rule besides our target window 
(note the NEGATE mask). The call back function (findAndRaiseLazy) is what we 
will use to raise our target. The function searches for the first window to match
 a given rule and raises it. We pass in the targetWindowRule to match out desired
window
static Rule alwaysOnTopRuleRule={NULL,0, BIND(findAndRaiseLazy,&targetWindowRule)};
// We want this Rule to be applied everytime a window is raised. Any time a window's
 size, position or stacking order change (or when it is first detected), all onWindowMove
rules are triggered, so lets add our rule there
appendRule(onWindowMove,&targetWindowRule);
```
For more examples see settings.c for all the defaults
# Install
`make install`
#  Run
mpxmanager
