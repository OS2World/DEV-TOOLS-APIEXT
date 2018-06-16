/*
 *  Module Name:    apiext.c	API conversion M extensions by Byron Dazey
 *
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
 *  The format of the routines, data structures and several calls used are
 *  documented in the manual for the editor in the chapter on "Writing M
 *  Extensions".
 *
 *  Make File
 *  ----------------------------------------------------------------------
 *  # makefile for the apiext M extension
 *
 *  apiext.obj:  $*.c
 *	cl /W3 /Gs /c /Asfu $*.c
 *
 *  # make OS/2 version -----------------------------------
 *
 *  apiext.dll: exthdrp.obj $*.obj
 *	link /NOD /NOI $**,$*,nul,clibcep doscalls,$*.def;
 *
 *  # make DOS version ------------------------------------
 *
 *  apiext.exe: exthdr.obj $*.obj
 *	link /NOD /NOI $**,$*,nul,clibcer api;
 *
 *  Change Log
 *  ----------------------------------------------------------------------
 *  04/18/88	0.10	BDD	Initial Release
 *
 */

#include <ext.h>		/*for M extensions			*/
#include <string.h>		/*for C library string functions	*/

#define INCL_DOSPROCESS 	/*for DosBeep				*/
#define INCL_DOSFILEMGR 	/*for DosQFileMode			*/
#include <os2.h>

#undef TRUE			/*defined in os2.h			*/
#define TRUE	-1
#define FALSE	0
#define NULL	((char *) 0)

				/*printed on startup			*/
#define LOADMESSAGE \
    "API Conversion M Extensions Version 0.10 Are Now Loaded."
				/*regular expression for searching for	*/
				/* "#include <doscalls.h>"		*/
#define REGEXPDOSCALLS \
    "^\\\\#include:b[\\\"<][dD][oO][sS][cC][aA][lL][lL][sS].[hH][\\\">]"
				/*regular expression for searching for	*/
				/* "#include <subcalls.h>"		*/
#define REGEXPSUBCALLS \
    "^\\\\#include:b[\\\"<][sS][uU][bB][cC][aA][lL][lL][sS].[hH][\\\">]"

flagType pascal EXTERNAL ConvertAPI(unsigned int, ARG far *, flagType);
flagType pascal EXTERNAL InsertAPIIncl(unsigned int, ARG far *, flagType);
flagType pascal EXTERNAL SwApiList(char far *);
flagType WhenLoaded(void);

char chBuf[128];		/*line buffer				*/
char CmdBuf[128];		/*command buffer			*/
PFILE pListFile;		/*API list file pointer 		*/
PFILE pCurFile; 		/*current file pointer			*/
LINE l; 			/*line counter				*/
LINE qLines;			/*number of lines in the list file	*/
char InclStr[33] = "";		/*save area for the INCL		*/
COL xCur;			/*saved column of the cursor		*/
LINE yCur;			/*saved line of the cursor		*/
				/*API list file name			*/
char ListFileName[128] = "os2api.lst";
unsigned FileAttr;		/*used by DosQFileMode			*/
flagType NewIncl;		/*TRUE if we are on a new INCL		*/

/*
 *  Function Name:  ConvertAPI
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
 *  Parameters:
 *  Name	    Type	    Description
 *  --------------- --------------- --------------------------------------
 *  argData	    unsigned int    Value of the keystroke pressed
 *  pArg	    ARG far *	    Argument type specific data
 *  fMeta	    flagType	    TRUE if META was pressed
 *
 *  Returns:	flagType
 *  Description
 *  ----------------------------------------------------------------------
 *  TRUE for a successful operation or FALSE if an error occurred.
 *
 */

flagType pascal EXTERNAL ConvertAPI(argData, pArg, fMeta)
unsigned int argData;
ARG far * pArg;
flagType fMeta;
{
				/*check for existence of list file	*/
    if (DosQFileMode(ListFileName, &FileAttr, 0L)) {

	strcpy(chBuf, "CONVERTAPI: The list file \"");
	strcat(chBuf, ListFileName);
	strcat(chBuf, "\" was not found!");
	DoMessage(chBuf);
	DosBeep(1200L, 100L);

	return (FALSE); 	/*return failure			*/
    }
				/*make file buffer for list file	*/
    pListFile = AddFile(ListFileName);
				/*read list file into the M buffer	*/
    FileRead(ListFileName, pListFile);
				/*get length of list file		*/
    qLines = FileLength(pListFile);
				/*get a handle to the current file	*/
    pCurFile = FileNameToHandle("", "");

				/*search for the old subcalls.h include */
				/*line in the source			*/
    strcpy(CmdBuf, "mark arg arg \"");
    strcat(CmdBuf, REGEXPSUBCALLS);
    strcat(CmdBuf, "\" psearch");
    if (fExecute(CmdBuf))	/*found! delete line			*/
	fExecute("ldelete");

				/*search for the old doscalls.h include */
				/*line in the source			*/
    strcpy(CmdBuf, "mark arg arg \"");
    strcat(CmdBuf, REGEXPDOSCALLS);
    strcat(CmdBuf, "\" psearch");
    if (fExecute(CmdBuf))	/*found! (replace with os2.h)		*/
	fExecute("arg ldelete \"#include <os2.h>\" begline");
    else			/*not found! (put at top of file)	*/
	fExecute("mark linsert \"#include <os2.h>\"");

    GetCursor(&xCur, &yCur);	/*save position in file 		*/

    NewIncl = TRUE;		/*should not be necessary because first */
				/*non-comment line in os2api.lst should */
				/*be a new INCL 			*/

				/*loop through the list file		*/
    for (l = 0; l < qLines; l++) {

				/*get the next line			*/
	GetLine(l, chBuf, pListFile);

	if (*chBuf == ';')	/*skip if a comment line		*/
	    continue;

	if (*chBuf == '#') {	/*is it an INCL?			*/

				/*save it without the '#',		*/
	    strcpy(InclStr, &chBuf[1]);
	    NewIncl = TRUE;	/*reset flag				*/
	    continue;		/*and go on				*/
	}

	DoMessage(chBuf);	/*display API				*/

				/*do a case insensitive global search	*/
				/*and replace on the API		*/
	strcpy(CmdBuf, "mark replace \"");
	strcat(CmdBuf, chBuf);
	strcat(CmdBuf, "\" arg emacsnewl \"");
	strcat(CmdBuf, chBuf);
	strcat(CmdBuf, "\" arg emacsnewl");

				/*API found and first one for the INCL? */
	if (fExecute(CmdBuf) && NewIncl) {

				/*go to saved file position		*/
	    MoveCur((COL) 0, yCur);

				/*insert a #define for the INCL 	*/
	    strcpy(CmdBuf, "linsert \"#define ");
	    strcat(CmdBuf, InclStr);
	    strcat(CmdBuf, "\"");
	    fExecute(CmdBuf);

	    yCur++;		/*increment saved file position because */
				/*we just added a line			*/

	    NewIncl = FALSE;	/*reset flag so we only add one define	*/
				/*per INCL				*/
	}
    }

    fExecute("mark");		/*go to the top of the file		*/

    RemoveFile(pListFile);	/*remove temporary list file		*/

    DosBeep(1200L, 50L);
    DoMessage("CONVERTAPI: Complete.");

    return TRUE;		/*return success			*/
}

/*
 *  Function Name:  InsertAPIIncl
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
 *
 *  Parameters:
 *  Name	    Type	    Description
 *  --------------- --------------- --------------------------------------
 *  argData	    unsigned int    Value of the keystroke pressed
 *  pArg	    ARG far *	    Argument type specific data
 *  fMeta	    flagType	    TRUE if META was pressed
 *
 *  Returns:	flagType
 *  Description
 *  ----------------------------------------------------------------------
 *  TRUE for a successful operation or FALSE if an error occurred.
 *
 */

flagType pascal EXTERNAL InsertAPIIncl(argData, pArg, fMeta)
unsigned int argData;
ARG far * pArg;
flagType fMeta;
{
				/*check for existence of list file	*/
    if (DosQFileMode(ListFileName, &FileAttr, 0L)) {

	strcpy(chBuf, "INSERTAPIINCL: The list file \"");
	strcat(chBuf, ListFileName);
	strcat(chBuf, "\" was not found!");
	DoMessage(chBuf);
	DosBeep(1200L, 100L);

	return (FALSE); 	/*return failure			*/
    }
				/*make file buffer for list file	*/
    pListFile = AddFile(ListFileName);
				/*read list file into the M buffer	*/
    FileRead(ListFileName, pListFile);
				/*get length of list file		*/
    qLines = FileLength(pListFile);

    yCur = (LINE) 0;		/*save position at top of file		*/

				/*loop through the list file		*/
    for (l = 0; l < qLines; l++) {

				/*get the next line			*/
	GetLine(l, chBuf, pListFile);

	if (*chBuf == ';')	/*skip if a comment line		*/
	    continue;

	if (*chBuf == '#') {	/*is it an INCL?			*/

				/*save it without the '#',		*/
	    strcpy(InclStr, &chBuf[1]);
	    DoMessage(InclStr); /*display it,
	    continue;		/*and go on				*/
	}

				/*it is an API.  Go to top of file and	*/
				/*search for it 			*/
	strcpy(CmdBuf, "mark arg \"");
	strcat(CmdBuf, chBuf);
	strcat(CmdBuf, "\" psearch");

	if (fExecute(CmdBuf)) { /*found!				*/

				/*go to saved file position		*/
	    MoveCur((COL) 0, yCur);
				/*insert a #define for the INCL 	*/
	    strcpy(CmdBuf, "linsert \"#define ");
	    strcat(CmdBuf, InclStr);
	    strcat(CmdBuf, "\"");
	    fExecute(CmdBuf);

	    yCur++;		/*increment saved file position because */
				/*we just added a line			*/

				/*skip the remaining API's for the INCL */
	    for (; l < qLines; l++) {

		GetLine(l, chBuf, pListFile);

				/*found next INCL?			*/
		if (*chBuf == '#') {

		    strcpy(InclStr, &chBuf[1]);
		    DoMessage(InclStr);
		    break;
		}
	    }
	}
    }

    fExecute("mark");		/*go to the top of the file		*/

    RemoveFile(pListFile);	/*remove temporary list file		*/

    DosBeep(1200L, 30L);
    DoMessage("INSERTAPIINCL: Complete.");

    return TRUE;		/*return success			*/
}

/*
 *  Standard M extension structures and initialization function follows.
 *
 */

flagType pascal EXTERNAL SwApiList(str)
char far *str;
{
    strcpy(ListFileName, str);	/*update the list file name and path	*/
    return TRUE;
}

struct swiDesc	swiTable[] =
{
    {"apilist", SwApiList, SWI_SPECIAL},
    {NULL, NULL, (int) NULL}
};

struct cmdDesc	cmdTable[] =
{
    {"convertapi", ConvertAPI, 0, NOARG},
    {"insertapiincl", InsertAPIIncl, 0, NOARG},
    {NULL, NULL, (unsigned) NULL, (unsigned) NULL}
};

flagType WhenLoaded ()
{
				/*set convertapi key			*/
    SetKey("convertapi", "alt+2");
				/*set insertapiincl key 		*/
    SetKey("insertapiincl", "alt+1");
    DoMessage(LOADMESSAGE);	/*display loaded message		*/

    return TRUE;
}
