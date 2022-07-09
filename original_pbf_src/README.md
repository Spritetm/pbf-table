These are the sources available from
https://github.com/historicalsource/pinballfantasies
but re-encoded into UTF8, plus some lines translated from
Swedish to english, plus a failed attempt to get this mess 
to assemble. I'll leave the Makefile and other notes in place
in case someone else wants to have a go at it.

Step 1: iconv -f CP850 -t utf-8 [all-files]
Step 2: swedish -> english

'should be compatible' masm variant:
uasm - https://github.com/Terraspace/UASM but check PRs on compile fixes
make using 'make -f gccLinux64.mak'

