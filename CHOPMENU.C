/* chopmenu.c by I. Baumel 11/07/89

	calling sequence:

	chopmenu DL_Application_Id Menu_Letter Entries_Screen CSCScscs

	Chopmenu will read the fixed format menu file CSCS0000.MNU in the
	CSCS directory. It will chop it up into Object files each having up
	to Entries_Screen entries. The Object Set for the selection menu
	will be written into CSCS\CSCS000X.D01-D99 where X is the Menu_Letter.

	All chop file filenames will be added to the UPLOAD.LST file in
	same directory.
-added category logic     7/31/91   newman      ver 2.0.0

*/

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <io.h>
#include <direct.h>

#define MAX_BUFF 500
#define APPLIC  1               /* 1st parameter, Application ID        */
#define LETTER  2               /* 2nd parameter, Menu Letter           */
#define ENTRIES 3               /* 3rd parameter, Entries per Screen    */
#define CSCS    4               /* 4th parameter, CSCS for object id    */

#define ITEM_SIZE 109           /* item size in menu file       */

#define MAX_TITLE 20            /* max length of title          */
#define MAX_VDATE 8             /* max length of version date   */
#define MAX_VERSN 8             /* max length of version        */
#define MAX_ZIPFN 12            /* max length of zip file name  */
#define MAX_CATEGORY 14         /* client(9) space contract(4)  */
#define MAX_FILEN 30            /* max length of file name      */

#define OBJ_HEAD_LENGTH 22      /* object header length         */
#define SEG_HEAD_LENGTH 4       /* segment header length        */
#define OBJ_SIZE 13             /* position in object header of size field */
#define SEG_SIZE 19             /* position in object header of segment sz */
#define SEGS_CTR 16             /* position in object header of seg ctr    */
#define OBJS_CTR 22             /* position in data part of object ctr     */

char title1[MAX_TITLE+1],  title2[MAX_TITLE+1],  vdate[MAX_VDATE+1];
char version[MAX_VERSN+1], zipfile[MAX_ZIPFN+1], data_size[7];
char category[MAX_CATEGORY];
char objects[MAX_FILEN], outfile[MAX_FILEN], saveobjs[MAX_FILEN];
char control[MAX_FILEN], target[MAX_FILEN], menu[MAX_FILEN];
char sinn[5], fctr[3], lstfile[MAX_FILEN];
char lstfile_txt[]="UPLOAD.LST", path[25];
char buffer[MAX_BUFF], objid[9], ascii[10], cscs[5], lsw;
int  c, zip, des, t, s, r, ilctr, ctr, entries, i, j, obsz, stsz;
int  segments;
int  title1_l, title2_l, zipfile_l, price_l, category_l, catnum_l;
long bod;
FILE *inf, *lst;

int main(argc, argv)
char *argv[];
int argc;
{
	if (argc < 3)
	{
		printf(
		"Usage: makemenu Application_id Menu_Letter Entries_Screen, Aborted\n");
		return(-1);
	}

	make_file_names(argv[APPLIC], argv[LETTER]); /* based on CS and LT */

	entries=atoi(argv[ENTRIES]);    /* Entries per Screen 2 Binary  */

	segments=0;                     /* initiate segment counter     */
	t=0;                            /* no object file open          */
	ilctr=entries+1;                /* make sure to open first menu */
	ctr=0;                          /* object counter               */
	obsz=0;                         /* initiate object size         */

/* open the CSCS\CSCS0000.MNU file      */

	if (0 == (inf = fopen(menu, "r")))      /* open FF menu file    */
	{
		perror("");             /* print system error msg       */
		printf("%s : cannot open menu file: %s\n",argv[0],menu);
		return(-1);
	}

	if (0 == (lst = fopen(lstfile, "a")))   /* open list file       */
	{
		printf("%s : cannot open list file: %s\n",argv[0],lstfile);
		return(-1);
	}

/* prepare the CSCS part of object ID   */

	i=strlen(argv[CSCS]);
	if(i == 4) strcpy(cscs, argv[CSCS]);    /* copy it */
	else
	{
		for(i=0;i<8;i++)        /* convert HEX string into CSCS */
		{
			c=argv[CSCS][i];        /* take next char       */
			if(c >= '0' && c <= '9') c=c-'0';
			if(c >= 'A' && c <= 'F') c=c-'A'+0x0a;
			if((i%2) == 0) cscs[i/2]=c;     /* move high nibble */
			else           cscs[i/2]=cscs[i/2]*0x10+c; /* add lower */
		}
	}

/* read all menu lines */

	while(EOF != (r=fscanf(inf, "%c ", &lsw)))
	{
		fscanf(inf, "%8c ",     vdate);
		fscanf(inf, "%8c ",     version);
		fscanf(inf, "%4c ",     sinn);
		fscanf(inf, "%d ",      &des);
		fscanf(inf, "%d ",      &zip);
		fscanf(inf, "%6c ",     data_size);
		fscanf(inf, "%d ",      &title1_l);
		fscanf(inf, "%20c ",    title1);
		fscanf(inf, "%d ",      &title2_l);
		fscanf(inf, "%20c ",    title2);
		fscanf(inf, "%d ",      &zipfile_l);
		fscanf(inf, "%12c ",    zipfile);
		fscanf(inf, "%d",       &price_l);
		fscanf(inf, "%d ",      &category_l);
		fscanf(inf, "%14c ",    category);
		fscanf(inf, "%d\n",     &catnum_l);

		if(lsw == 'L') continue;        /* skip LockedOut       */

/* This is required to circumvent a bug in the fscanf library routine. It does
   not correctly parse an empty %20c field.
*/
		if(title2[0] == '|') title2[0]=' ';

		vdate[MAX_VDATE]=version[MAX_VERSN]=sinn[4]=0;
		title1[MAX_TITLE]=title2[MAX_TITLE]=zipfile[MAX_ZIPFN]=0;
		data_size[6]=0;

		for(i=0; i<strlen(data_size) && data_size[i] == '0'; i++)
			data_size[i]=' ';       /* repl lead zero by space */

		if(ilctr >= entries)            /* write out current object*/
		{
			if (t)                  /* if current object open  */
			{                       /* close it                */
				obsz=tell(t)-bod;  /* calc data length  */
				closeobj(t);
			}
			objname();              /* prepare next object name */
			if (-1 == (t = open(target,
			O_CREAT | O_TRUNC | O_WRONLY | O_BINARY, 0000200)))
			{
				perror("");     /* print system error msg   */
				printf("%s : cannot open %s\n",argv[0],target);
				return(-1);
			}
			ilctr=0;                /* reset line ctr in obj    */
			write_obj_header();     /* write object header      */

			fprintf(lst, "%s\\%s\n", getcwd(path, 25), target);/* add fn */
		}
		make_item_line();       /* make selection menu item line    */
		ilctr++;                /* increment line ctr in object     */
	}

	if (t)                          /* if object open       */
	{                               /* close it             */
		obsz=tell(t)-bod;       /* calc data length     */
		closeobj(t);
	}
	fclose(lst);                    /* close list file      */
	fclose(inf);                    /* close FF menu file   */

/* 2nd pass: scan all objects and insert object count in headers */

    printf("\nStart Second Pass. %d Segments\n", segments);

    sprintf(ascii, "%03.3d", segments);     /* convert to ascii     */

    ctr=0;
    for (i=1; i<=segments; i++)
    {
	objname();              /* prepare next object name */
	if (-1 == (t = open(target,
		       O_WRONLY | O_BINARY)))
	{
		printf("%s : cannot open %s\n",argv[0],target);
		return(-1);
	}
	lseek(t, SEGS_CTR, SEEK_SET);
	write(t, &segments, 1);         /* one byte binary seg ctr      */
	lseek(t, OBJS_CTR, SEEK_SET);
	write(t, ascii, 3);             /* three byte ascii obj ctr     */

	close(t);
    }

	printf("\n\n");
	exit(0);
}

closeobj()                              /* close current object         */
{
	lseek(t, OBJ_SIZE, SEEK_SET);   /* position to object size      */
	i=obsz+OBJ_HEAD_LENGTH;         /* calculate object size        */
	write(t, &i, 2);                /* write obj size in place      */

	lseek(t, SEG_SIZE, SEEK_SET);   /* position to segment size     */
	i=obsz+SEG_HEAD_LENGTH;         /* calculate segment size       */
	write(t, &i, 2);                /* write seg size in place      */

	close(t);                       /* close current segment file   */

	obsz=0;                         /* reset object size for next   */
	segments++;                     /* increment segment counter    */

	return(0);
}

objname()                               /* prepare next object name     */
{
	if (++ctr > 99)                 /* increment and check obj ctr  */
	{
		printf("More then 99 objects in set, Aborting.\n");
		exit(-1);
	}

	/* prepare current output file name */

	strcpy(target, outfile);
	sprintf(fctr, "%02.2d", ctr);   /* convert to ASCII             */
	strcat(target, fctr);
	printf("\rOut file: %s",target);
	return(0);
 }

write_obj_header()                      /* write object header fields   */
{
	write(t, cscs, 4);              /* CSCS for object id           */
	write(t, &objid[4], 4);         /* last 4 of obj id             */
	write(t, "D  ", 3);             /* Dbb                          */
	write(t, &ctr, 1);              /* occurance number             */
	i=0x0c;
	write(t, &i, 1);                /* program/data                 */
	write(t, &(obsz), 2);           /* object size                  */
	i=0;
	write(t, &i, 1);                /* bin zero                     */
	write(t, &segments, 1);         /* total number of objects/set  */
	write(t, &i, 1);                /* bin zero                     */
	i=0x61;
	write(t, &i, 1);                /* 61 data segment              */
	write(t, &(stsz), 2);           /* segment size                 */
	i=0x02;
	write(t, &i, 1);                /* 02 segment type data         */
	bod=tell(t);                    /* save start of data position  */
	write(t, ascii, 3);             /* NO.OBJs in set               */
	return(0);
}

make_item_line()                        /* make selection item line     */
{
	write(t, vdate,   MAX_VDATE);
	write(t, version, MAX_VERSN);
	write(t, sinn,    4);
	write(t, &des,    1);           /* description object ctr bin   */
	write(t, &zip,    1);           /* zipfile object ctr bin       */
	write(t, data_size, 6);         /* zipfile data size            */
	write(t, &title1_l, 1);         /* title1 length binary         */
	write(t, title1, title1_l);     /* title1                       */
	write(t, &title2_l, 1);         /* title2 length binary         */
	write(t, title2, title2_l);     /* title2                       */
	write(t, &zipfile_l, 1);        /* zipfile length binary        */
	write(t, zipfile, zipfile_l);   /* zipfile name                 */
	write(t, "\0",    1);           /* length of price              */
	write(t, &category_l, 1);       /* length of category           */
   write(t, category, category_l); /* category-client contract     */ 
	write(t, "\0",    1);           /* length of catalog num        */
	return(0);
}

make_file_names(cs, lt)                 /* make file names              */
char *cs, *lt;
{
	strcpy(objects, cs);            /* copy CSCS                    */
	strcat(objects, "\\");          /* make app. directory CS00\    */
	strcpy(lstfile, objects);       /* copy CSCS\                   */
	strcat(lstfile, lstfile_txt);   /* make CSCS\UPLOAD.LST         */
	strcat(objects, cs);            /* make name CSCS\CSCS          */
	strcpy(saveobjs, objects);      /* save partial object name     */

	strcpy(outfile, objects);       /* make CSCS\CSCS for menu      */
	strcat(outfile, "000");         /* make CSCS\CSCS000 for menu   */
	strncat(outfile, lt, 1);        /* make CSCS\CSCS0000X          */

	strcpy(menu, outfile);          /* make CSCS\CSCS000X           */

	strcat(outfile, ".D");          /* make CSCS\CSCS0000.D obj menu*/
	strcat(menu, ".MNU");           /* make CSCS\CSCS0000.MNU  raw  */

	strcpy(objid, cs);              /* make CSCS for object ID      */
	strcat(objid, "000");           /* make CSCS000                 */
	strncat(objid, lt, 1);          /* make CSCS000X                */
	strupr(objid);                  /* make object ID upper case    */
	return(0);
}

