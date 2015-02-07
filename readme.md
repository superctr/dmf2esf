     ____  __  __ _____ ____  _____ ____  _____
    |  _ \|  \/  |  ___|___ \| ____/ ___||  ___|
    | | | | |\/| | |_    __) |  _| \___ \| |_
    | |_| | |  | |  _|  / __/| |___ ___) |  _|
    |____/|_|  |_|_|   |_____|_____|____/|_|
Converts Deflemask DMF modules to Echo Stream Format.

=====================================================

Usage
=====

Ini mode:

    ./dmf2esf <options> input.ini

Individual file mode:

    ./dmf2esf <options> input.dmf output.esf

Options:
--------
* `-i` - Extracts FM instruments from the DMF file and outputs them to stdout.
    The output is designed to work with an ASM macro.

    I recommend piping the output to a file for easier copy-pasting.

* `-a` - Output ESF data as ASM. Can be used to further tweak the module after
    conversion.
	
* `-e` - Output ESF data optimized for EchoEX. Work in progress.

Ini mode:
---------
The recommended way to convert files. Here's an example:

    [0]                 ; File id. You can convert many files at once with the ini
                        ; mode.
    input  = input.dmf  ; Input file
    output = output.esf ; Output file

    ins_00 = 0x0a       ; Instrument conversion table.
    ins_01 = 0x0b    ; <- Here, DMF instrument 01 is converted to ESF instrument 0b
    ins_02 = 0x0c       ; Using this, you can have multiple tracks use the same
    ins_03 = 0x0d       ; instrument table.
    ins_04 = 0x0e
    ins_05 = 0x0f
    ins_06 = 0x10
    ins_07 = 0x07

    pcm_c  = 0x09       ; Sample conversion table. First letter is the tone letter,
    pcm_cs = 0x08       ; sharps are marked with 's'. Example:
                        ; pcm_c, pcm_cs, pcm_d, pcm_ds, etc.

    [1]                 ; Next file.
    input  = blah.dmf
    ...

Supported Effects
=================

* `8xx` Set panning

* `Bxx` Position jump (Only backwards jumps are supported, and only one per module)

* `Dxx` Pattern break (Make sure not to include any position jumps in the skipped area)

* `17xx` DAC enable

* `20xx` PSG noise mode

Effects that will not work with vanilla Echo
--------------------------------------------
Could be implemented in Echo-EX later.

* `1xxx` effects that modify instrument parameters.

Notes
=====
Echo handles volume changes in a different way to Deflemask.

While Deflemask will decrease the total level for all operators,
Echo will only decrease the total level for the final operator(s)
using the current algorithm.

How to build
=============

Use codeblocks to build. Remember to clean the project before building a
new version or for another platform.

Tested to work on Windows 7 (with MinGW) and on Linux Mint 16.

Should only require GCC, the C library and STL. Using another compiler will
probably work (with a few edits, maybe) but it's untested.

Credits
=======

* Programming: ctr

* Echo sound driver: Sik

* `xm2esf` (which this project is inspired by and has stolen code from): Oerg866

Libraries
---------
* [`miniz`](http://code.google.com/p/miniz/): Rich Geldreich

* [`inih`](http://code.google.com/p/inih/): Brush Technology
