/* makemenu.c by I. Baumel 11/07/89

	calling sequence:

	makemenu DL_application_id

	Makemenu will scan through the sub-directories of the named DL
	application id, will identify all CONTROL.TXT files, will count
	the number of files in the description Object Set and the
	DL Package Object Set and create an entry for the Selection Menu.
	This entry will go into the CSCS0000.MNU file in the CSCS directory.
	It is a fixed format record/file that will be used as input to
	different SORT criteria. The output from the SORT will will chopped
	into Selection Menu Objects by CHOPMENU.
	It will scan all sub-directories up to the highest consequtive SINn
	directory found.
	Any directory which has got a LOCKOUT.TXT will be excluded from the
	menu build.
   
   - ADDED CATEGORY FIELD FOR CONTRACT(9) AND ITEM(4)  7/31/91 NEWMAN
*/

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <io.h>

#define MAX_BUFF 500
#define APPLIC  1               /* 1st parameter */

#define KW_SEP    :             /* key word seperator in CONTROL.TXT */
#define MAX_TITLE 20            /* max length of title pieces   */
#define MAX_VDATE 8             /* max length of version date   */
#define MAX_VERSN 8             /* max length of version        */
#define MAX_ZIPFN 12            /* max length of zip file name  */
#define MAX_CATEGORY 14         /* contract(9) space(1) item(4) */

#define MAX_FILEN 30            /* max length of file name      */
#define OBJ_HEAD_LENGTH 22
char title1[MAX_TITLE+1],  title2[MAX_TITLE+1],  vdate[MAX_VDATE+1];
char version[MAX_VERSN+1], zipfile[MAX_ZIPFN+1], category[MAX_CATEGORY];
char objects[MAX_FILEN], outfile[MAX_FILEN], saveobjs[MAX_FILEN];
char control[MAX_FILEN], logbook[MAX_FILEN], lockout[MAX_FILEN];
char sinn[5], fctr[3], lsw;
char buffer[MAX_BUFF], ascii[10];

char logbook_txt[]="APPLCTRL.TXT";      /* logbook file name    */
char control_txt[]="CONTROL.TXT";       /* control file name    */
char lockout_txt[]="LOCKOUT.TXT";       /* lockout file name    */

int z, d, t, s, r, ctr, ilctr, i, j, obsz, stsz, segments, lines=0, sinilim;
long bod, data_size;
FILE *inf, *out, *lgf, *loc;

int main(argc, argv)
char *argv[];
int argc;
{
	int sini;

	if (argc < 2)
	{
		printf(
		"Usage: makemenu Application_id, Aborted\n");
		return(-1);
	}

	make_file_names(argv[APPLIC]);  /* make file names based on CS  */

	/* open APPLCTRL.TXT file in CSCS       */

	if (NULL == (lgf=fopen(logbook, "r")))
	{
		printf("%s : cannot open %s\n",argv[0], logbook);
		return(-1);
	}
	fscanf(lgf, "%d", &sinilim);            /* get sin limit        */
	fclose(lgf);                            /* close logbook file   */

	/* open CSCS0000.MNU file in CSCS       */

	if (NULL == (out=fopen(outfile, "w")))
	{
		perror("");     /* print system error msg   */
		printf("%s : cannot open %s\n",argv[0], outfile);
		return(-1);
	}

/* loop over all CSCSSINn directories within CSCS application directory */

	for (sini=1; sini<=sinilim; sini++)     /* loop on packages     */
	{
		sprintf(sinn, "%-04.4d", sini); /* make SINn 0 padded   */

		strcpy(objects, saveobjs);      /* get CSCS\CSCS        */

		strcat(objects, sinn);          /* make CSCS\CSCSSINn   */
		strcat(objects, "\\");          /* make CSCS\CSCSSINn\  */

		strcpy(control, objects);       /* copy CSCS\CSCSSINn\  */
		strcat(control, control_txt);   /* add CONTROL.TXT      */

		strcpy(lockout, objects);       /* copy CSCS\CSCSSINn\  */
		strcat(lockout, lockout_txt);   /* add LOCKOUT.TXT      */

					/* try open, if not, report error*/
		if (0 == (inf = fopen(control,"rb")))
		{
			printf("\nUnable to open %s, skipping to next package.", control);
			continue;
		}

					/* yes, we have a directory     */
		printf("\nSub-Directory: %13.13s", control);

		parse_ctrl();           /* parse CONTROL.TXT file       */
		fclose(inf);            /* close CONTROL.TXT            */

		if (r == -1)            /* check result of parse_ctrl() */
		{
			printf("\n%s: Control file bad, skipped.");
			continue;       /* skip, try next               */
		}

		strcat(objects, argv[APPLIC]);  /* make CSCS\CSCSSINn\CSCS */
		strcat(objects, sinn);          /* CSCS\CSCSSINn\CSCSSINn  */
		strcat(objects, ".");           /* put dot before extension*/

		d=fcount(objects, "D");         /* count description files */
		z=fcount(objects, "Y");         /* count zipfile files     */

		lsw='U';                        /* assume package unlocked */
		if(0 != (loc=fopen(lockout, "r+")))     /* open lockout */
		{
			printf("\nPackage %s, LOCKOUT effective.", control);
			fclose(loc);            /* make sure to close   */
			lsw='L';                /* prepare lsw L        */
		}

		make_item_line();       /* make selection menu item line    */
		printf("\n");
	}

	printf("\n");
	exit(0);                        /* end of program               */
}

parse_ctrl()                            /* parse CONTROL.TXT file       */
{
    char *p;

    title1[0]=title2[0]=0;
    vdate[0]=0;
    version[0]=0;
    zipfile[0]=0;                       /* set all strings to null      */
    category[0]=0;

    while(0 != (r= strlen(fgets(buffer, MAX_BUFF, inf)))) /* read line/chop */
    {                                                     /* look for : */
	if (buffer[0] == 0x1A) break;   /* if EOF, finish               */
	/* printf("%s", buffer); */           /* show line                    */
	if (NULL == (p = strchr(buffer, ':'))) return (r=-1);

	*p=0;                           /* terminate keyword with NULL  */
	p++;                            /* point to variable            */

	if (0 == (strcmpi(buffer, "title1")))  copyvar(p, title1, MAX_TITLE);
	if (0 == (strcmpi(buffer, "title2")))  copyvar(p, title2, MAX_TITLE);
	if (0 == (strcmpi(buffer, "date")))    copyvar(p, vdate,  MAX_VDATE);
	if (0 == (strcmpi(buffer, "version"))) copyvar(p, version, MAX_VERSN);
	if (0 == (strcmpi(buffer, "zipfile"))) copyvar(p, zipfile, MAX_ZIPFN);
	if (0 == (strcmpi(buffer, "category"))) copyvar(p, category, MAX_CATEGORY);

    }

    /* The following is required to circumvent a bug in the fscanf library
       routine. It does not pick up 20 spaces as input for %20c and skips
       to the next non space character on input, thus shifting everything.
       Our fix will make sure that title2 field (which is optional) is never
       empty.
       This technique should be followed for each and every optional control
       item added in the future. DO NOT leave a field full spaces.
    */
    if(!strlen(title2)) strcpy(title2, "|");
    if(!strlen(category)) strcpy(category, "|");

    return(r=0);
}

copyvar(buf, var, max)                  /* copy variable from line      */
char *buf, *var;
int max;
{
	int i=0;

	while (*buf == ' ') buf++;      /* skip lead spaces in var      */
	while (*buf != 0 && i < max) var[i++]=*buf++; /* copy char 2 var*/
	var[i]=0;                       /* terminate string with null   */
	while (    (--i)>=0
		&& ( var[i] == ' ' || var[i] == '\n' || var[i] == '\015') )
		var[i]=0;       /* replace trailing spaces, CR or LF
				   by zeroes */

	return(0);
}

int fcount(fname, fext)                 /* count object files in set    */
char *fname, *fext;
{
	char file_name[MAX_FILEN], nn[3];
	int i;

	printf("\nCount: %s%snn ", fname, fext);

	data_size=0;                    /* reset data size accumulator  */

	for (i=1; i<100; i++)
	{
		sprintf(nn, "%02.2d", i);       /* make nn 2 digits     */
		strcpy(file_name, fname);
		strcat(file_name, fext);
		strcat(file_name, nn);          /* dir\CSCSSINn.Tnn     */

					/* if not exist, stop loop      */
		if (0 == (inf = fopen(file_name, "rb"))) break;

		data_size += ( filelength(fileno(inf)) - OBJ_HEAD_LENGTH );
		fclose(inf);                    /* close current obj file */
		printf(".");                    /* one dot per file     */
	}

	return(i-1);                            /* return file count    */
}

make_item_line()                        /* make selection item line     */
{
	fprintf(out,
	/*lock date   versn  SINn  #Dobjs #Zobjs ZIP-lgh titl1#  title1  titl2#  title2  ZIP-f#  zipfnam price# categ# categ    catno# */
	 "%c %-8.8s %-8.8s %-4.4s %02.2d %02.2d %06.6ld %02.2d %-20.20s %02.2d %-20.20s %02.2d %-12.12s %01.1d %01.01d %-14.14s %01.1d\n"
	 ,lsw, vdate, version, sinn, d, z, data_size,
	 strlen(title1), title1, strlen(title2), title2, strlen(zipfile),strupr(zipfile),
	 0, strlen(category), category, 0);
	ilctr++;                        /* increment item line ctr      */
	return(0);
}

make_file_names(cs)                     /* make file names              */
char *cs;
{
	strcpy(objects, cs);            /* copy CSCS                    */
	strcat(objects, "\\");          /* make app. directory CSCS\    */

	strcpy(logbook, objects);       /* copy CSCS\                   */
	strcat(logbook, logbook_txt);   /* add APPLCTRL.TXT             */

	strcat(objects, cs);            /* make name CSCS\CSCS          */
	strcpy(saveobjs, objects);      /* save partial object name     */

	strcpy(outfile, objects);       /* make CSCS\CSCS for menu      */
	strcat(outfile, "0000.MNU");    /* make CSCS\CSCS0000.MNU menu  */

	return(0);
}

