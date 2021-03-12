/* chopdes.c by I. Baumel 11/16/89

	calling sequence:

	chopdes input_file lines_in_screen chars_in_line output_file CSCScscs
			   LIS             CIL

	Chopdes will take a text file and pack it into object set ready files,
	each having LIS lines of CIL characters at most, besides
	the last one which may have less lines. It will scroll the extension
	of the output file name from D01 up to D99 and terminate if not
	finished.

	The names of all chop files will be added to UPLOAD.LST in the parent
	directory.

*/

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <io.h>
#include <direct.h>

#define MAX_BUFF 500
#define SOURCE  1               /* 1st parameter */
#define LIS     2               /* 2nd parameter */
#define CIL     3               /* 3rd parameter */
#define TARGET  4               /* 4th parameter */
#define CSCS    5               /* 5th parameter */

#define OBJ_SIZE 13             /* position in object header of size field */
#define SEG_SIZE 19             /* position in object header of segment sz */
#define SEGS_CTR 16             /* position in object header of seg ctr */

char outfile[15], lstfile[]="..\\UPLOAD.LST";
char target[15],fctr[3], path[25];
char buffer[MAX_BUFF], cscs[5];
char crlf_line[]="\r\n";        /* a CRLF line  */

int c, t, s, r, lis, cil, ctr, lctr, i, obsz, stsz, segments, crlf, lines=0;
int main(argc, argv)
char *argv[];
int argc;
{
    FILE *inp, *lst;
    if (argc < 6)
	{
	printf(
	"Usage: %s Source Lines_in_screen Chars_in_line Outfile CSCS\n",
	 argv[0]);
	return(-1);
	}
    if (0 == (inp = fopen(argv[SOURCE],"rb")))          /* binary */
    {
		printf("%s : cannot open %s\n",argv[0],argv[SOURCE]);
		return(-1);
    }
    i=filelength(fileno(inp));
    lis=atoi(argv[LIS]);                /* binary lines in screen */
    cil=atoi(argv[CIL]);                /* binary chars in line */
    segments=ctr=t=obsz=crlf=0;         /* reset counters               */
    lctr=lis+1;                         /* simulate file change required*/

/* prepare the output file name */

    strcpy(outfile, argv[TARGET]);      /* copy file name               */
    i=strlen(outfile);                  /* check outfile length         */
    if (i > 8) i=8;                     /* take 8 only                  */
    outfile[i]='\0';                    /* terminate string             */
    while (strlen(outfile) < 8) { strcat(outfile, "0"); } /* add zeroes */
    strupr(outfile);                    /* upper case                   */
    strcat(outfile, ".D");              /* add D for Dnn                */

/* open the list file   */

    if(NULL == (lst=fopen(lstfile, "a")))
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

    while(0 != (r= strlen(fgets(buffer, MAX_BUFF, inp)))) /* read line/chop */
    {
	if(buffer[0] == '\x1A') continue;       /* skip EOF mark        */
	if(lctr >= lis)                         /* open new screen      */
	{
		lctr=0;
		if (t) closeobj(t);             /* if previous file exists */
		objname();                      /* get next object name    */
						/* open segment file       */
		if (-1 == (t = open(target,
			O_CREAT | O_TRUNC | O_WRONLY | O_BINARY, 0000200)))
		{
			printf("%s : cannot open %s\n",argv[0],target);
			return(-1);
		}
		obsz=0;                 /* reset accumulated data length */

		fprintf(lst, "%s\\%s\n", getcwd(path, 25), target); /* add fn*/

					/* write object header fields   */
		write(t, cscs, 4);      /* write CSCS           */
		write(t, &outfile[4], 4);     /* last 4 of object id    */
		write(t,"D  ",3);       /* Dbb */
		write(t,&ctr,1);        /* occurance number */
		i=0x0c;
		write(t,&i,1);          /* program/data */
		write(t,&(obsz),2);     /* object size */
		i=0;
		write(t,&i,1);          /* bin zero */
		write(t,&segments,1);   /* total number of objects in set */
		write(t,&i,1);          /* bin zero */
		i=0x61;
		write(t,&i,1);          /* 61 data segment */
		write(t,&(stsz),2);     /* segment size */
		i=0x02;
		write(t,&i,1);          /* 02 segment type data */
	}

	lctr++;

	/* printf("%s", buffer); */
						/* before writing out   */
	if(!strcmp(buffer, crlf_line))  crlf++; /* count empty lines    */
	else                                    /* until non empty line */
	{
		while(crlf)                     /* write accumulated    */
		{
			write(t, crlf_line, 2); /* empty lines          */
			obsz+=2;                /* add 2 to file size   */
			crlf--;                 /* dec empty line ctr   */
		}
		if(-1 == write(t, buffer, r))   /* write current line   */
		{
			perror("");
			printf("%s : Error writing %s\n",argv[0],target);
			return(-1);
		}
		obsz=obsz+r;            /* add text length      */
	}
    }

    closeobj(t);                /* close last object */
    fclose(inp);                /* done with input file */
    fclose(lst);                /* close list file      (/

/* 2nd pass: scan all objects and insert object count in headers */

    printf("\nStart Second Pass. %d Segments\n", segments);

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
	write(t, &segments, 1);         /* one byte binary seg ctr */
	close(t);
    }
    return (0);
}

closeobj()
{
	lseek(t, OBJ_SIZE, SEEK_SET);   /* position to object size      */
	i=obsz+22;
	write(t, &i, 2);            /* write obj size in place          */
	lseek(t, SEG_SIZE, SEEK_SET);   /* position to segment size     */
	i=obsz+4;
	write(t, &i, 2);            /* write seg size in place          */
	close(t);                   /* close current segment file       */

	if(obsz == 0)   unlink(target); /* dont create a zero data obj  */
	else            segments++;     /* increment segment counter    */
	obsz=0;                     /* reset object size accumulator    */
	crlf=0;                     /* reset empty line counter         */
	return(0);
}

objname()
{
	if (++ctr > 99)
	{
		printf("More then 99 segments, Aborting.\n");
		exit(-1);
	}

	/* prepare current output file name */

	itoa(ctr, fctr, 10);    /* convert to ASCII */
	strcpy(target, outfile);
	if (strlen(fctr) == 1) strcat(target,"0");
	strcat(target, fctr);
	printf("\rOut file: %s",target);
	return(0);
 }
