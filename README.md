A kernel-based keylogger for Linux

# Usage

#### Installation

```
$ git clone https://github.com/NicholasBHubbard/kloggg
$ cd kloggg
$ loadkeys -m us > kloggg-keymap.h # use whatever keymap you would like
$ make
$ sudo insmod ./kloggg.ko
```

#### Log

Key presses are logged to `/proc/kloggg/keylog`

# TODO

* Add detection-aversion capabilities
* Improve implementation for logging non-printable characters
