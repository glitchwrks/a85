# A85 Cross Assembler

A cross assembler for 8080/8085 processors based on William C. Colley, III's A85 from C User's Group disk number 267, with modifications from Herb Johnson and The Glitch Works. 

Original work by William C. Colley III and changes by Herb Johnson are licensed under their own terms. See A85.DOC for more information. Glitch Works changes and improvements are licensed under the GNU GPL v3.

Herb Johnson's information on A85 can be found at [http://www.retrotechnology.com/restore/a85.html](http://www.retrotechnology.com/restore/a85.html).

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
```