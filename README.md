# A85 Cross Assembler

A cross assembler for 8080/8085 processors based on William C. Colley, III's A85 from C User's Group disk number 267, with modifications from Herb Johnson and Glitch Works, LLC. 

Original work by William C. Colley III and changes by Herb Johnson are licensed under their own terms. See A85.DOC for more information. Glitch Works changes and improvements are licensed under the GNU GPL v3.

Herb Johnson's distribution of A85 can be found at [http://www.retrotechnology.com/restore/a85.html](http://www.retrotechnology.com/restore/a85.html).

Unless something is tagged with a release number, consider this work to be ALPHA or EXPERIMENTAL.

### Compiling

Just run `make` in the project directory. `make test` will build the test file, `TEST85.ASM`, which runs the assembler through all opcodes. Or, if you want to build by hand:

`cc a85.c a85util.c a85eval.c -o a85`

### Revision History:

```
Ver     Date            Description
-----------------------------------------------------------------------------
0.0     AUG 1987        Derived from version 3.4 of my portable 6800/6801
                        cross-assembler.  WCC3.

0.1     AUG 1988        Fixed a bug in the command line parser that puts it
                        into a VERY long loop if the user types a command 
                        line like "A85 FILE.ASM -L".  WCC3 per Alex Cameron.

0.1+    DEC 2013        edited for LCC-32 by Herb Johnson
                        Mostly by updating function declarations,
                        replacing () with (VOID), etc. SOme fixes need
                        when char and int were used interchangably,
                        unsigned was given signs, etc.
                        labels ending with colon now have colon stripped
                        All fixes have HRJ in comment lines    HRJ.

0.2     FEB 2020        Glitch Works general cleanup, added support for
                        PRINT and INCLUDE.

0.3     JUN 2024        Glitch Works addition of IFDEF, IFNDEF. Light cleanup
                        and improvement of comments, enum names.
```

### Herb's Notes

These are edited versions of Colley's A85 cross assembler from C User's Group disk number 267. Look for CUG CD-ROM archives on the Web. Most of the work was to add `void` to replace declarations of `()` with `(void)`, and in some cases convert `(int)` to `(char)`. I've identified my code changes with the initials "HRJ" in comments. 

I borrowed routines from `A68eval.c` (the 6800 Colley cross assembler also in the CUG library) because I had 8080 syntax errors when running the modified `a85eval.c`.

I attemped to add this syntax:

`pop b ! pop d ! pop h`

where `!` seperates multiple assembly statements on one line. But the 
modified scanner in A85 gets confused by strings like

`pop b!pop d`

and considers the second operation as a label. So I ended that attempt.