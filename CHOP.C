/* chop.c by I. Baumel 10/24/89

	calling sequence: chop in_file chop_size out_file CSCS 1st_ext [no_header]

	Chop will take a file and pack it into object set ready files,
	each having chop_size characters of the original file, besides
	the last one which may have less. It will scroll the extension
	of the output file name from D01 up to D99 and terminate if not
	finished.

	If an additional parameter is specified (6) then clean files
	will be created on output without object headers.

	All chop file names will be added to a fixed name file UPLOAD.LST
	at the parent directory.

*/

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <io.h>
#include <direct.h>

#define MAX_BUFF 32000

#define SOURCE  1               /* source file */
#define SIZE    2               /* chop size */
#define TARGET  3               /* target file */
#define CSCS    4               /* 4 char CSCS or 8 digit HEX CSCS      */
#define EXT     5               /* 1st extension letter */
#define NO_HEAD 6               /* no object header indicator, optional */

char buffer[MAX_BUFF];
char path[25];
char source[25], outfile[15], lstfile[]="..\\UPLOAD.LST";
char target[15],fctr[3], cscs[]="cscs";
int c, t,s,r,sz,ctr,i,obsz,stsz,segments;
long l;
FILE *lst;
size_t size;

int main(argc, argv)
char *argv[];
int argc;
{
	if (argc < 6)
	{
		printf("Usage: Chop Source Chop_Size Outfile Ext [No_Header]\n");
		return(-1);
	}

	strcpy(source, argv[SOURCE]);

						/* open source file     */

	if (-1 == (s = open(source, O_BINARY | O_RDONLY)))
	{
		printf("%s : can't open %s\n",argv[0],source);
		return(-1);
	}
	l=filelength(s);                        /* get file length      */
	sz=atoi(argv[SIZE]);                    /* convert to binary    */
	segments=((l/sz)+1);                    /* segments expected    */
	while (segments > 99)
     {
		sz=sz+898;
   	segments=((l/sz)+1);                /* segments expected    */
   	 }
	ctr=0;
printf("package will be converted to %d objects, each being %d bytes \n",segments,sz); 
/* prepare the output file name */

	strcpy(outfile, argv[TARGET]);          /* copy file name       */
	i=strlen(outfile);                      /* check outfile length */
	if (i > 8) i=8;                         /* take 8 chars only    */
	outfile[i]='\0';                        /* terminate string     */

	while (strlen(outfile) < 8)
		{ strcat(outfile, "0"); }       /* add ASCII zeroes     */

	strcat(outfile, ".");                   /* . for NNNNNNNN.TTT   */
	strcat(outfile, argv[EXT]);             /* T of Tnn             */
	outfile[10]='\0';                       /* 1 char extension     */
	strupr(outfile);                        /* upper case           */

/* open filename list file */
	if (NULL == (lst=fopen(lstfile, "a")))
	{
		printf("%s : cannot open %s\n",argv[0],lstfile);
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

/* start copy loop */

	while((r = read(s,buffer,sz)) != 0 && r != -1) /* read next chop*/
	{
		if (++ctr > 99)                 /* too many objects     */
		{
			printf("More then 99 segments, Aborting.\n");
			return(-1);
		}
		itoa(ctr, fctr, 10);

		strcpy(target, outfile);        /* start with base name */
		if (strlen(fctr) == 1) strcat(target,"0");
		strcat(target, fctr);
		printf("\rOut file: %s",target);
		if (-1 == (t = open(target,
		     O_BINARY | O_CREAT | O_WRONLY | O_TRUNC, 0000200)))
		{
			printf("%s : can't open %s\n",argv[0],target);
			return(-1);
		}

		fprintf(lst, "%s\\%s\n", getcwd(path, 25), target); /* add fn */

		if (argc <= NO_HEAD)      /* if object headers required */
			make_obj_header(argv[EXT]); /* write it         */

		if(-1 == write(t, buffer, r))   /* write current chop   */
		{
		    perror("");
		    printf("%s : Error writing %s\n", argv[0], target);
		    return(-1);
		}
		if (-1 == (i = close(t)))       /* close current chop   */
		{
		    perror("");
		    printf("%s: Unable to Close %s\n", argv[0], target);
		};
	}
	close(s);                               /* close source file    */
	fclose(lst);                            /* close list file      */
	return (0);
}
make_obj_header(ext)
char *ext;
{
	write(t, cscs, 4);                      /* write CSCS           */
	write(t,&outfile[4],4);                 /* file name            */
	write(t, ext, 1);                       /* 1st letter of EXT    */
	write(t,"  ",2);                        /* bb after 1st letter  */
	write(t,&ctr,1);                        /* occurance byte       */
	i=0x0c;
	write(t,&i,1);                          /* program/data         */
	obsz=r+22;
	write(t,&(obsz),2);                     /* object size          */
	i=0;
	write(t,&i,1);                          /* bin zero             */
	write(t,&segments,1);                   /* total num of objects */
	write(t,&i,1);                          /* bin zero             */
	i=0x61;
	write(t,&i,1);                          /* 61 data segment      */
	stsz=r+4;
	write(t,&(stsz),2);                     /* segment size         */
	i=0x02;
	write(t,&i,1);                          /* 02 segment type data */
	return(0);
}

