JSON-C built as follows:

1. Download JSON-C 0.9 from http://oss.metaparadigm.com/json-c/json-c-0.9.tar.gz

2. Unzip and untar the file

3. Copy the following files to the "json" directory in the MUSHclient source:

config.h.win32

arraylist.c
arraylist.h
bits.h
debug.c
debug.h
json.h
json_object.c
json_object.h
json_object_private.h
json_tokener.c
json_tokener.h
json_util.c
json_util.h
linkhash.c
linkhash.h
printbuf.c
printbuf.h

4. Rename config.h.win32 to config.h.

5: Edit: json_object.h and change

typedef int boolean;

to

typedef unsigned char boolean;