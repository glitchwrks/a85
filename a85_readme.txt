Herb Johnson Dec 31 2013
adaptation of A85 C cross assembler
for LCC-WIN32

These are edited versions of Colley's A85 cross assembler from
C User's Group disk number 267. Look for CUG CD-ROM archives on the Web.
MOst of the work was to add "void" to replace declarations of ()
with (void), and in some cases convert (int) to (char). I've identified
my code changes with the initials "HRJ" in comments. 

I borrowed routines from A68eval.c (the 6800 Colley cross assembler
also in the CUG library) because I had 8080 syntax errors when running
the modified a85eval.c.

I attemped to add this syntax:

pop b ! pop d ! pop h

where ! seperates multiple assembly statements on one line. But the 
modified scanner in A85 gets confused by strings like

pop b!pop d

and considers the second operation as a label. So I ended that attempt.