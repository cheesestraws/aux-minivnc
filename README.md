![a screenshot of aux-minivnc](/img/screenshot.png)

# What is this?

This is a VNC server for A/UX, Apple's old UNIX.  It allows graphical remote control of the computer across login sessions, as if you were sitting down at the console.

# I'm a user: how do I install this?

First: I have only tested this under A/UX 3.1.  It will probably work under 3.0.  It will almost certainly not work under anything older.

There are two steps to installing aux-minivnc.  The first step is installing the kernel driver.  The second part is installing the VNC daemon.  All you need to do this is a checkout of this git repository on the A/UX machine in question.  Binaries are in the repository.  You can also download a TAR archive and follow the same instructions.

## Installing the driver

```
# cd <checkout-dir>/kernel
# ./install.sh
# newconfig fb
```

If newconfig works, check that `/dev/fb0` exists, then reboot.

```
# ls /dev/fb0
crw-------   1 bin      bin       62,  0 Nov  6 19:17 /dev/fb0
# reboot
```

## Installing the VNC daemon

```
# cd <checkout-dir>/app
# ./install.sh
```

You can then run `vncd` to start the VNC server directly.  If you want vncd to run at startup, add a line to inittab to start it off:

```
# cat inittab.line >> /etc/inittab
```

Please make **very sure** to use >> in this line, if you use > you will knacker your inittab and your A/UX installation will no longer work.

# I'm a user: how do I connect to this?

Use your favourite VNC viewer to connect to port 5900.  This has been most extensively tested with RealVNC.

Note that RealVNC's default modifier mappings are rather wonky and will lead to weird results.  Use the "expert" tab of the connection settings to change the Left and RightCmdKey settings to Super_L and Super_R, and the Left and RightOptKey settings to Alt_L and Alt_R, respectively.  Otherwise option isn't mapped to alt, which is odd, because that's what it ought to be.

# I'm a user or developer: what doesn't work?

* Full-screen X sessions.  Sorry.  But why are you using ancient full-screen X under A/UX anyway?  (MacX works fine).
* Special keys in the console emulator don't work.  There's really just enough to get you back out of the console.
* Keyboard layout changes only take effect at the beginning of sessions.  If you change keyboard layout on A/UX, you will need to close your VNC session and open a new one to be able to type properly.

# I'm a developer: what tools do I need to hack on this?

If you're an end-user stop reading this now: you do not need development tools installed to *use* aux-minivnc.  Otherwise:

* You will need to make sure the Apple development tools are installed on your A/UX machine.
* To work on the application, you will also need gcc-2.7.2.  This is widely available online, but if you haven't got it, it's also in the tools/ directory of this repository.
	* First, install gzip.  This is in a .tar.Z file that can be decompressed with the `uncompress` command.
	* Then you can install gcc, which is in a .tar.gz file.  You need gzip, of course, to un-gz this file :-).  There are instructions and a script in the tarball.

# I'm a developer: how is the code arranged?

* The kernel driver is in the kernel/ folder
	* This code is built using Apple's `cc`, and results in a single object file to be linked into the kernel.
	* Supporting files are in /etc: these are the files that tell the kernel build process what to do with the object file and what devices to create.
	* There is a build.sh script that will build it for you and an install.sh file that will put the results in the right place to be picked up by newconfig.
	* You would be well advised to look at [SolraBizna's testc driver](https://github.com/SolraBizna/testc) first, if you haven't played with A/UX drivers before.

* The application is in app/
	* This code is built using gcc, and results in an executable called `vncd`.
	* The lookup tables for expanding 1-, 2- and 4-bit modes were generated using the Go code in lutgen/.  These will probably never need changing, and you do not need to regenerate the LUTs.  So you can ignore lutgen, generally.
	* There is a build script called build.sh and an installation script called install.sh.
	* main() is in main.c.  This fires up a socket listener, and for each connection, fires up a session.  Only one session can be active at once.
	* Sessions are defined in session.{c,h}.  The session is responsible for all the resources that pertain to a user connection.
	* The most important of these is the framebuffer, defined in frame_buffer.{c,h}.  This manages the actual remote framebuffer that the RFB protocol is named after. 

# I'm a developer: what's some interesting code to poke?

* The whole kernel driver: there aren't many new ones of these about at the moment.
	* The use of `kallDriver`, both in the CLUT code and in the "current mode" code, shows how to call a Mac video driver from inside A/UX.
	* The `fb_ui_mouse` function demonstrates how to muck about with the "low memory globals" of the current Mac session, and how to inject events into that session.
	* The `fb_phys` function, along with the corresponding call to the `FB_PHYS` ioctl in `app/session.c` can perhaps act as a worked example of how to map physical addresses into a process's address space.  Note that the first argument to phys() is process-local: a nicer way of doing this, if you were aiming for reusability, would be to scan through the phys list in the user struct and find the next free one, rather than just using '1'.

* The keyboard code in the application is probably the most interesting thing.  It works by fishing the keyboard layout out of the kernel (which is a copy of the KCHR resource that the MacOS is currently using) and reverse indexing it, so it can go from character to key codes.  No idea what, if anything, this would be useful for outside of this context, but it demonstrates the strength of the classic Mac approach of keeping as much as possible as data-driven tables.

* app/macroman.{c,h} are not interesting in and of themselves and were intensely tedious to produce.  But they map from X keysyms to MacRoman characters, and may be useful for that incredibly niche use elsewhere, I suppose?

# Credits

This code is mostly &copy; Rob Mitchelmore, 2022.  It started off as a fork of, and some portions remain of, @marciot's mac-minivnc, so portions are &copy; Marcio Teixeira, 2022.  I'd also like to acknowledge SolraBizna for the testc driver, linked above, which started me thinking that this might actually be possible.