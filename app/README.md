# To build:

```
$ cd <this-directory>
$ ./build.sh
```

# To install:

```
# ./install.sh
```

Note that this will install vncd in /etc, as is usual on A/UX for network daemons.

vnc0:2:respawn:/etc/vncd >/dev/syscon

# To run on startup

```
# cat inittab.line >> /etc/inittab
```

BE CAREFUL to use `>>` not `>`!