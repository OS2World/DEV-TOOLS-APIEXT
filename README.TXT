
 *  Description:
 *  ----------------------------------------------------------------------
 *  APIEXT is an M extension written by Byron Dazey to help developers
 *  convert from the all uppercase OS/2 API convention using the include
 *  file "doscalls.h" used by C 4.5 to the mixed case API convention using
 *  "os2.h" used in the newer C 5.1 product.
 *
 *  By default,  these functions assume the existence of the file
 *  "os2api.lst" in the current directory.  This file contains the API's
 *  in their mixed case form and is sectioned off in groups by the #define
 *  names that need to be defined to include them (called INCL's).
 *  Lines in this file that start with a semi-colon are comments,
 *  lines that start with a "#" are INCL's. Other lines are assumed to
 *  be OS/2 API's in their mixed case form.  For example:
 *
 *  ----------os2api.lst------------
 *  ;comments here		    <- This is a comment line
 *  #INCL_DOSPROCESS		    <- This is a #define
 *  DosExecPgm			    <-
 *  DosExit			    <- These are API's for INCL_DOSPROCESS
 *
 *  If required, the text switch "apilist" can be set to the path and
 *  file name of the file that contains the API's.  If used, this switch
 *  must contain at least the file name portion of the location of this
 *  list.  For instance:
 *
 *  apilist:c:\init\apilist.txt 	 or
 *  apilist:apilist.txt
 *
 *  would be valid, but not:
 *
 *  apilist:c:\init
 *
 *
 *  This extension needs to be "loaded" into the editor to be used.  This
 *  can be done in the TOOLS.INI file by adding the following lines:
 *
 *  [mep]
 *	load:apiext
 *
 *  for OS/2 or:
 *
 *  [m]
 *	load:apiext.exe
 *
 *  for DOS.
 *
 *  To use in OS/2, copy the file APIEXT.DLL to somewhere on your LIBPATH
 *  and edit your TOOLS.INI file as described above.  Invoke MEP on the
 *  file to convert and press alt+2 or alt+1 (unless you have re-assigned
 *  these keys).
 *
 *  To use in DOS, copy the file APIEXT.EXE to somewhere on your PATH and
 *  edit your TOOLS.INI file as described above.  Invoke M on the file to
 *  convert and press alt+2 or alt+1 (unless you have re-assigned these
 *  keys).  I had trouble with my version of M not finding the apiext.exe
 *  file on the path.  If this happens, copy apiext.exe to your current
 *  directory or fully qualify the pathname in the "load:" statement in
 *  TOOLS.INI (ex. load:c:\tools\bin\apiext.exe).
 *
 *
 *  Extension: convertapi
 *
 *  Description
 *  ----------------------------------------------------------------------
 *  This extension is used to convert all the occurances of an OS/2 API
 *  to its mixed case equivalent.  The os2api.lst file is read and all the
 *  API's listed are searched for without regard for case and globally
 *  replaced with the case as found in the os2api.lst file.  Because the
 *  length of the API is not changing, comments should not need to be
 *  lined up again.
 *
 *  This extension will search through the current file for where the
 *  include file "doscalls.h" is being included and replace it with a
 *  line including "os2.h".  If "doscalls.h" is not found then it will
 *  add the include for "os2.h" at the beginning of the file.
 *
 *  This line can be in any of the following forms, with the case of
 *  "doscalls.h" and the number of spaces after "include" not being
 *  important:
 *
 *  #include <doscalls.h>
 *  #include "doscalls.h"
 *  #include	    <DOSCALLS.H>
 *
 *
 *  These will be replaced with:
 *
 *  #include <os2.h>
 *
 *
 *  A line including "subcalls.h" will also be searched for in a similiar
 *  manner and will be deleted if found.
 *
 *  This extension will then search for the API's in each group in the
 *  os2api.lst file and if an API for a certain "INCL" is found, it will
 *  insert an appropriate #define statement for it before the include for
 *  "os2.h".  For example, if "DosExecPgm" is found in the file, the
 *  following will be added to the file:
 *
 *  #define INCL_DOSPROCESS
 *  #include <os2.h>
 *
 *  WARNING!
 *  API's that appear in comments and conditionally compiled sections of
 *  code will be converted.  In addition, if any function or variable has
 *  been named with part of an API's name, it will be converted to mixed
 *  case also.	For instance, the user function "xDOSEXIT" would be
 *  converted to "xDosExit".  This could also result in more #define
 *  statements than are strictly necessary.  This is not a C preprocessor!
 *
 *  This function is not blinding fast.  It must invoke a global search
 *  and replace on each OS/2 API and this can take a while for large
 *  source files.  It is, however, much faster than doing it by hand and
 *  should only need to be done once.
 *
 *
 *
 *  Extension: insertapiincl
 *
 *  Description
 *  ----------------------------------------------------------------------
 *  This extension will search for the API's in each group in the
 *  os2api.lst file and if an API for a certain "INCL" is found, it will
 *  insert an appropriate #define statement for it at the top of the file.
 *  For example, if "DosExecPgm" is found in the file, the following will
 *  be added to the top of the file:
 *
 *  #define INCL_DOSPROCESS
 *
 *  This functionality is also included in the convertapi function.  It is
 *  repeated in this function so that a programmer can find all the names
 *  of the INCL's that must be defined for a converted program that has
 *  undergone some development/maintenance since being converted.  In some
 *  cases it would be useful to know if all the INCL's being defined still
 *  need to be.
 *
 *  WARNING:
 *  API's that appear in comments or in conditional sections of code will
 *  still be considered when searching.  In addition, if any function or
 *  variable has been named with part of an OS/2 API's name, it will be
 *  considered a match.  For instance, if there is a function named
 *  xDosExecPgm then INCL_DOSPROCESS will be included.	This could result
 *  in more #defines than are strictly necessary.
 *
