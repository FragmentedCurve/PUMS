
/* pums.c by I. Baumel 11/09/89

	calling sequence:

	pums

	Pums is the main menu and control program for the Prodigy Upload
	Management Station. It will enable access to the major system
	functions and introduce some consistency and qc checks. The final
	outcome should be an easily controled PUMS and error free object
	sets for the TPF.
   - added OPTION TO NOT ZIP PRODUCT                  7/23 NEWMAN
   - added WARNING AND EXIT FOR INVALID NO ZIP OPTION 7/25 NEWMAN
   - changed isin = atoi(sing) to isin = snn to fix lost sing data 7/30 newman
   - added logic to intercept choosing sinn = 0 on replace option 7/30
   - ADDED CONTRACT LOGIC TO CONTROL.TXT     7/30
   - added option to not virus scan 7/31
   - added option to accept input from hard drive 7/31 
   - added option to set default input drive and output path  9/4 
   - added option to show default input/output drive on header  9/5 
   - fixed subdirectory problem - set max_adir to 30  9/6 
   - added logic if output directory alredy exists  9/18 
	- added auto calculation of object size 9/19
	- changed menu layout (removed virus scan and show directory) 9/24
	- modified messages for insert floppy in case of hard drive 9/24
   LAST UPDATED: 9/24/91      VER 2.3.3
*/

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <io.h>
#include <stdlib.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <ctype.h>
#include <dos.h>
#include <setjmp.h>
#include <time.h>
#include <bios.h>
#include <errno.h>

/* general defaults     */

#define VIRUS_SCAN_SW   FALSE    /* change to TRUE for production*/

#define DEFAULT_DRIVE   "A"    /* default diskette drive       */
#define DEFAULT_PATH    "A:\\UPDATE"  /*default output path     */
#define DEFAULT_AUDIT_SW 'N'
#define MAX_ID          4       /* max chars in CSCS id         */
#define MAX_DIR         4       /* max chars in CSCS dirname    */
#define MAX_SINN        4       /* max chars in SINn            */
#define MAX_ADIR        30      /* max chars in path name       */
#define MAX_APP_NAME    25      /* max chars in application name*/

#define MAX_FILEN       50      /* max length of file name      */
#define MAX_SYS_BUFF    100     /* buffer to create system calls*/
#define MAX_AUDIT       20000   /* audit trail file big size    */
#define CHOP_MAX        32000   /* maximum allowable chop size  */

#define ITEM_SIZE       109     /* entry length in FF menu file */
#define REP_LINES       20      /* report lines per page        */
#define MAX_LINES       9       /* directory lines per screen   */
#define MAX_BUFF        500
#define MAX_PASSWD      33      /* any large number             */

#define TRUE    1
#define FALSE   0
#define YES    'Y'
#define NO     'N'
/* CONTROL.TXT file related definitions      */

#define KW_SEP          ':'     /* key word seperator in CONTROL.TXT */
#define MAX_TITLE       20      /* max length of title pieces   */
#define MAX_VDATE       8       /* max length of version date   */
#define MAX_VERSN       8       /* max length of version        */
#define MAX_ZIPFN       12      /* max length of zip file name  */
#define MAX_FILE        12      /* max length of filename       */

/* special characters definitions       */

#define BELL            '\a'            /* bell character               */
#define BS              '\b'            /* backspace character          */
#define CR              '\r'            /* carriage return character    */
#define ESC             '\x1B'          /* escape character             */

/* values for load_sw           */

#define NEW             TRUE
#define OLD             FALSE

/* values for load_show_sw      */

#define LOAD            1
#define LOCK            2
#define SHOW            3

/* defaults for PUMS parameters  */

#define CHOP_SIZE       876             /* block size for zipfile chops */
#define LINES_SCREEN    9               /* description lines per screen */
#define CHARS_LINE      37              /* description chars per line   */
#define ENTRIES_SCREEN  4               /* selection menu entries per screen*/

char upload_txt[]="UPLOAD.LST", upload_old[]="UPLOAD.OLD";
char config_txt[]="PUMSETUP.CFG", passwd_txt[]="terces";
char locked_txt[]="LOCKOUT.TXT",audit_txt[]="PUMSAUDT.TXT";
char control_txt[]="CONTROL.TXT", describe_txt[]="DESCRIBE.TXT";
char applctrl_txt[]="APPLCTRL.TXT", prodlzip_txt[]="PRODLZIP.LST";
char backup_txt[]="call pumsbup";       /* make sure that pumsbup.bat is OK*/

char buffer[MAX_BUFF], buff[ITEM_SIZE+1], dirname[MAX_FILEN];
char control[MAX_FILEN], describe[MAX_FILEN], locked[MAX_FILEN];
char title1[MAX_TITLE+1],  title2[MAX_TITLE+1],  vdate[MAX_VDATE+1];
char version[MAX_VERSN+1], zipfile[MAX_ZIPFN+1], data_size[7];
char zipind[2], zipfile_hold[12];
char CATEGORY[14];  /* category is contract(9) space(1) item(4)  */
char save_ctrl[MAX_FILEN], drv[]=DEFAULT_DRIVE, ndrv[2], sel;
char out_path[24]=DEFAULT_PATH, nout_path[24];
char app_name[MAX_APP_NAME+1], app_code[MAX_ID*2+1];
char ch, id[MAX_ID+1], applctrl[MAX_FILEN], dir[MAX_DIR+1], adir[MAX_ADIR+1];
char ddir[MAX_ADIR+1], filename[MAX_FILEN], oldfname[MAX_FILEN];
char sys_buf[MAX_SYS_BUFF];
char sin_control[MAX_FILEN], sinn[MAX_SINN+1], ssin[MAX_SINN+1];
char menu[MAX_FILEN], sin_dir[MAX_FILEN], file_name[MAX_FILEN];
char app_change_sw=FALSE, show_titles=FALSE;
char sss[MAX_SINN+1], sing[6], fora[MAX_FILEN], dort[MAX_FILEN];
char lsw, operator[12];
char audit_trail_sw=DEFAULT_AUDIT_SW; 
unsigned chop_size=CHOP_SIZE;
int  lines_screen=LINES_SCREEN, chars_line=CHARS_LINE, entries_screen=ENTRIES_SCREEN;
int  title1_l, title2_l, zipfile_l, price_l, category_l, catnum_l;
int  des, zip, snn, show_lines;
int  entries, isin, sini=0, bad_ctrl, load_sw, load_show_sw, echo_sw, full_sw, num_sw, inp_sw_ok;
int  title1_sw, vdate_sw, version_sw, zipfile_sw, file_sw;
int  zipind_sw, zip_file_cnt, category_sw;
int  dctr, i, j, k, r;

time_t ibtm;
long ll, fl, offset;
FILE *logf, *mnu, *prt, *ctrl, *sctrl, *desc, *zlst, *adt, *cfg, *f, *t;

struct diskinfo_t disk_info;
struct stat stat_buf;
struct find_t buf_dir;                  /* scan directory buf   */
jmp_buf env_buf;                        /* buffer for ESC processing    */

main(argc, argv)
char *argv[];
int argc;
{
	setjmp(env_buf);                /* save point to be ESCaped to  */

	echo_sw=TRUE;

	open_config();                  /* open control parameter file  */

	system("cls");                  /* start with clean screen      */

	no_applic();                    /* no application selected      */

	open_audit();                   /* open audit file, check size  */

	sel='0';                        /* signal PUMS entry            */
	audit(0);                       /* report audit                 */

	while(TRUE)
	{

		setjmp(env_buf);        /* save point to be ESCaped to  */
		full_sw=num_sw=FALSE;   /* turn off special requests    */
		echo_sw=TRUE;

		show_menu();            /* show main menu               */
		sel=ch;                 /* save user selection          */
		switch(sel)             /* switch based on selection    */
		{
			case '1':
				select_id();
				break;
			case '2':
				add_new();
				break;
			case '3':
				upd_old();
				break;
			case '4':
				mnu_maker();
				break;
			case '5':
            obj_to_tpf();
 				break;
			case 'D':
				select_drive();
				break;
			case 'S':
				list_info();
				break;
			case 'M':
				print_info();
				break;
			case 'B':
				backup();
				break;
		  	case 'L':
				lock_unlock();
				break;
			case 'T':
				print_audit();
				break;
			case 'P':
				psetup();
				break;
			case 'X':
				leave_now();    /* finish off           */
				exit(0);
			default:
				bell();
				break;
		}
	}
	return(0);                      /* exit now             */
}


show_menu()
{
	screen_head();                          /* show screen header   */
	printf("\t\t1.  SELECT client\n");
	printf("\t\t2.  ADD NEW Download Package \n");
	printf("\t\t3.  UPDATE EXISTING download Package\n");
	printf("\t\t4.  CREATE menus \n");
	printf("\t\t5.  COPY objects for Delivery to TPF\n");
	printf("\n");
	printf("   D.  Select input/output Drive\tL.  Lock / Unlock Existing Package\n");
	printf("   S.  Show package info by SINN\tT.  Print / Delete Audit Trail\n");
	printf("   M.  Print Selection Menu Report\tP.  PUMS Control Setup\n");
	printf("   B.  Backup PUMS Data Base\t\tX.  Exit this program\n");

	printf("\n");
	printf("\t\tEnter Selection: ");

	full_sw=TRUE;                   /* insist on non empty response */
	read_kb(&ch, 1);                /* get user selection           */
	ch=toupper(ch);                 /* make upper case              */
	return(0);
}

select_id()                             /* select application ID        */
{
	if(app_change_sw)               /* check current app open/upd   */
	{
		bell();                         /* yes  */
		err_msg("Current Application Changed. Please Recreate Selection Menus.");
		return(0);
	}

	if(show_cs_dirs()) return(-1);          /* show existing app id */

	if(strlen(buffer) == 0)                 /* no selection yet     */
	{
		ask_select_app(0);              /* ask select last page */
		if(strlen(buffer) == 0) return(0);
	}

	strcpy(id, buffer);                     /* copy new id          */
	strupr(id);                             /* make upper case      */
	strcpy(dir, id);                        /* make CSCS dirname    */

	applctrl_name();                        /* make applctrl name   */

	i=stat(dir, &stat_buf);                 /* check dir exists     */
	if (i == 0)     open_id();              /* directory exists     */
	else            make_id();              /* does not             */

	return(0);
}

open_id()                               /* open existing application    */
{
	open_log("r+");                         /* open applctrl rd/wr  */

	fscanf(logf, "%d %25c %s", &sini, app_name, app_code); /* get sini, app_name, app_code */
	app_name[MAX_APP_NAME]=0;               /* terminate name string*/

	audit(1);                               /* audit report         */

	return(0);
}

applctrl_name()                          /* create applctrl.txt name    */
{
	strcpy(applctrl, dir);                  /* make CSCS            */
	strcat(applctrl, "\\");                 /* make CSCS\           */
	strcat(applctrl, applctrl_txt);         /* make CSCS\APPLCTRL.TXT*/
	return(0);
}

control_name()                          /* create control.txt name      */
{
	sprintf(sss, "%04.4d", snn);
	strcpy(control, dir);                   /* make CSCS            */
	strcat(control, "\\");                  /* make CSCS\           */
	strcat(control, dir);                   /* make CSCS\CSCS       */
	strcat(control, sss);                   /* make CSCS\CSCSSINn   */
	strcat(control, "\\");                  /* make CSCS\CSCSSINn\  */
	strcpy(locked, control);                /* copy to locked       */
	strcat(control, control_txt);           /* add CONTROL.TXT      */
	strcat(locked, locked_txt);             /* add locked.TXT       */
	return(0);
}

make_id()                               /* create new application dir   */
{
	screen_head();
	printf("\n\tApplication %s does not exist. Create it? (Y/N): ", id);
	read_kb(&ch, 1);

	if(toupper(ch) != 'Y')
	{
		no_applic();                    /* deselct application  */
		return(0);
	}

	printf("\n\n\tEnter Application Name: ");
	read_kb(app_name, MAX_APP_NAME);        /* read name for applic */

	inp_sw_ok=FALSE;                        /* force first time     */
	while(!inp_sw_ok)
	{
		inp_sw_ok=TRUE;                 /* assume input OK      */
		printf("\n\tEnter CSCS as 4 chars, 8 HEX digits or <ENTER> to use %s: ", id);
		read_kb(buffer, 2*MAX_ID);          /* read code for applic */
		if(strlen(buffer) == 0) strcpy(app_code, id);
		else
		if(strlen(buffer) == MAX_ID)
      {   
             strcpy(app_code, buffer);
				 strupr(app_code);
      }
		else
		if(strlen(buffer) == 2*MAX_ID)
		{
			for(i=0; i<2*MAX_ID; i++)
			{
				if(isxdigit(buffer[i]))
				{
					app_code[i]=toupper(buffer[i]);  /* copy char    */
					continue;
				}
				else
				{
					bell();
					err_msg("HEX digits are 0-9 and A-F only.");
					inp_sw_ok=FALSE;        /* set sw NOK   */
					break;                  /* stop loop    */
				}
			}
			app_code[i]='\0';                       /* terminate    */
		}
		else
		inp_sw_ok=FALSE;                /* must be 0, 4 or 8    */
	}

	printf("\n\n\tCreate NEW Application with:\n");
	printf("\n\t\tApplication ID  : %s", id);
	printf("\n\t\tApplication Name: %s", app_name);
	printf("\n\t\tApplication CSCS: %s\n", app_code);
	msg_get("Confirm (Y/N): ");

	if(ch != 'Y')
	{
		no_applic(0);                   /* deselct application  */
		return(0);
	}

	if(-1 == (i=mkdir(dir)))                /* try make directory   */
	{
		perror("");                     /* show system error msg*/
		err_msg("Unable to create Application Directory.");
		exit(-1);
	}


	open_log("w+");                         /* open applctrl wr/rd  */

	sini=0;                                 /* initialize sini ctr  */

	update_log();                           /* update applctrl      */

	audit(2);                               /* audit report         */

	return(0);
}

no_applic()                             /* deselect application id      */
{
	strcpy(id, "");                         /* no applic selected   */
	sini=0;                                 /* no packages          */
	app_change_sw=FALSE;                    /* no changes were made */
	strcpy(app_name, "No Application Selected");
	return(0);
}

leave_now()                             /* finish PUMS process          */
{
	if(app_change_sw)                       /* check app open/upd   */
	{
		bell();                         /* yes                  */
	   printf("\n\tCURRENT APPLICATION CHANGED BUT NEW MENU WAS NOT CREATED");
      printf("\n\tARE YOU SURE YOU WANT TO LEAVE. ENTER (Y/N): ");
   	read_kb(&ch, 1);
   	if(toupper(ch) != 'Y') longjmp(env_buf, 1);   /* RETURN TO MENU  */
	}

	system("cls");                          /* clear screen         */
	audit(0);                               /* audit report         */
	fclose(adt);

	return(0);
}

read_kb(buf, n)                         /* get string from KB length n  */
char *buf;
int n;
{
	int i=0;
	char c;

	if(echo_sw)
	{
		for(i=0; i<n; i++) putchar('_');/* draw undelines       */
		for(i=0; i<n; i++) putchar(BS); /* reposition to start  */
	}

	for(i=0; i<=n; )                        /* loop length and CR   */
	{
		get_one_key(&c);                /* get next key         */

		if (c == BS && i>0)             /* is it a Back Space   */
		{
			if(echo_sw)
			{
				putchar(BS);    /* backspace on screen  */
				putchar('_');   /* prompt char          */
				putchar(BS);    /* reposition           */
			}
			i--;                    /* remove last char     */
			continue;
		}

		if(i == n && c != CR)           /* should CR at length  */
		{
			putchar(BELL);          /* insist on CR at end  */
			continue;
		}

		if(c == CR && (i == n || (!full_sw)))   /* ENTER key    */
		{
			buf[i]=0;               /* terminate string     */
			break;
		}

		if( (num_sw && isdigit(c)) || (!num_sw && isprint(c)) ) /* OK char */
		{
			if(echo_sw) putchar(c); /* echo back char       */
			buf[i++]=c;             /* store char in buffer */
			continue;               /* next char            */
		}

		putchar(BELL);                  /* tell him bad         */
	}
	full_sw=num_sw=FALSE;                   /* turn off special req */
	return(0);
}

get_one_key(c)                          /* get one key press from KB    */
char *c;
{
	*c=getch();                             /* get next char from KB*/
	if(*c == ESC) longjmp(env_buf, 1);      /* forget all, goto menu*/
	return(0);
}

add_new()                               /* add new down load package    */
{
	load_sw=NEW;                            /* set sw for NEW       */
	if(!check_applic_ok(0)) return(0);      /* check applic selected*/
	if(!load_package()) audit(0);           /* load now and report  */
	return(0);
}

upd_old()                               /* update existing package      */
{
	load_sw=OLD;                            /* set sw for OLD       */
	if(!check_applic_ok(1)) return(0);      /* check applic selected*/
	if(!load_package()) audit(0);           /* load now and report  */
	return(0);
}


load_package()                          /* load package into directory  */
{
	if  (drv[0] == 'A' || drv[0] == 'B')
      printf("\n\tPlace Package Diskette in Drive %s.", drv);
	else
	   printf("\n\t PACKAGE WILL BE LOADED FROM DRIVE %s.", drv); 
	wait_kb();

/* cancelled out. using diskette ready creates diskette processing problems */
/*        diskette_ready();     */               /* make sure ready      */

	if(VIRUS_SCAN_SW) scan_now(drv[0]);   /* IF SWITCH TRUE - MUST SCAN  */
	else
	{
	printf("\n\tDO YOU WANT TO VIRUS SCAN THIS DISK BEFORE CONTINUING? ENTER (Y/N):");
	read_kb(&ch, 1);
	if(toupper(ch) == 'Y')  scan_now(drv[0]);
	}


	strcpy(ddir, "nodir");                  /* assume single package*/
	strcpy(control, drv);                   /* copy current drive   */
	strcat(control, ":\\");                  /* add :\ after drive:   */
	strcpy(describe, control);              /* copy same            */
	strcpy(adir, control);                  /* copy same            */
	strcat(control, control_txt);           /* make A:\CONTROL.TXT  */
	if(NULL == (ctrl=fopen(control, "r")))  /* check CONTROL.TXT    */
	{
		if(show_a_dirs()) return(-1);   /* show subdirectories  */

		if(strlen(buffer) == 0)         /* no selection yet     */
		{
			ask_select_pkg(0);      /* ask select last page */
			if (strlen(buffer) == 0) return(-1); /* CR to end    */
		}

		strcpy(ddir, buffer);           /* copy selection 2 ddir*/

		strcpy(control, drv);           /* copy current drive   */
		strcat(control, ":\\");          /* add :\ after drive:   */
		strcat(control, ddir);          /* make A:\dirname      */
		strcat(control, "\\");          /* make A:\dirname\     */
		strcpy(adir, control);          /* copy A:\dirname\     */
		strcpy(describe, control);      /* copy A:\dirname\     */
		strcat(control, control_txt);   /* add CONTROL.TXT      */
		strupr(control);                /* upper case name      */

		if(NULL == (ctrl=fopen(control, "r")))  /* file exists  */
		{
			bell();                 /* no   */
			printf("\n\t\Cannot open %s on diskette.", control);
			wait_kb();
			return(-1);
		}
		fclose(ctrl);                   /* exists, so close it  */
	}

	strcat(describe, describe_txt);         /* add DESCRIBE.TXT     */
	strupr(describe);                       /* upper case name      */

	if(NULL == (desc=fopen(describe, "r"))) /* check file exists    */
	{
		bell();                         /* no   */
		printf("\n\tCannot open %s on diskette.", describe);
		wait_kb();
		return(-1);
	}
	fclose(desc);                           /* close file DESCRIBE  */

	if(load_sw == NEW)                      /* is it a NEW load     */
	{
		isin=sini+1;                    /* yes  */
		if(check_sin_exist())           /* dl package dir exist */
		{
			bell();
			err_msg("Directory EXISTS. Cannot Load NEW Package into it.");
			return(-1);
		}
	}

	if(load_sw == OLD)                      /* is it an OLD load    */
	{
		screen_head();                  /* display screen header*/
		strcpy(save_ctrl, control);     /* save diskette control*/
		strcpy(operator, "Load Into");
		load_show_sw=SHOW;              /* show control only    */
		display_sinn();
		isin=snn;                /* convert to binary    */
		msg_get("Confirm Loading OVER this Package. (Y/N): ");
		if(ch != 'Y') return(-1);       /* dont load here       */
		strcpy(control, save_ctrl);     /* restore diskette ctrl*/
		check_sin_exist();              /* need some of code    */
	}

	load_show_sw=LOAD;                      /* this is a load req.  */

	if(get_control_info()) return(-1);      /* bad ctrl file or pckg*/

	if (strcmp(ddir,"nodir") == 0) {
		printf("\n\n\tWill load package with SIN ID=%s.  Confirm (Y/N): ", ssin);
	} else {
		printf("\n\n\tWill load \"%s\" package with SIN ID=%s. Confirm (Y/N): ",
						ddir, ssin);
	}
	read_kb(&ch, 1);
	if (toupper(ch) != 'Y') return(-1);     /* dont do it           */


	if(mkdir(sin_dir))                      /* make CSCS\CSCSSINn   */
	{
		if(load_sw == NEW)              /* is it a NEW load     */
		{
			bell();
			err_msg("Cannot create the Application directory.");
			return(-1);
		}
	}

/* delete all files in the sinn directory       */

	sprintf(buffer, "%s\\*.*", sin_dir);            /* make CSCS\CSCSSINn\*.* */

	if(0 == (_dos_findfirst(buffer, _A_NORMAL, &buf_dir)))    /* any file */
	{
		sprintf(buffer, "%s\\%s", sin_dir, buf_dir.name);
		unlink(buffer);

		while(0 == (_dos_findnext(&buf_dir)))   /* check following  */
		{
			sprintf(buffer, "%s\\%s", sin_dir, buf_dir.name);
			unlink(buffer);
		}
	}

/* copy files to CSCS\CSCSSINn directory        */

	screen_head();
  
   if (zipind_sw == TRUE && zipind[0] == 'N' && zip_file_cnt != 1)
                                  		  /*if more than 1 file being packaged*/
     {
      printf("WARNING-NO ZIP OPTION CANNOT BE USED WITH MULTIPLE FILES\n"); 
      printf("TO CONTINUE ENTER Y - TO STOP ENTER N \n");
     	read_kb(&ch, 1);
   	if (toupper(ch) != 'Y') return(-1);     /* dont do it */
      zipind[0] = 'Y';    /* reset zip switch to zip on */
		}
	printf("\nCopying files to Package Directory. Please wait...\n");
	fcopy(prodlzip_txt, sin_dir);           /* copy PRODLZIP.LST    */
	fcopy(control,      sin_dir);           /* copy CONTROL.TXT     */
	fcopy(describe,     sin_dir);           /* copy DESCRIBE.TXT    */

	copyf_to_disk();                        /* copy files to disk   */
   if (zipind_sw == TRUE && zipind[0] == 'N')
   	sprintf(sys_buf, "call nzipchop %s %s %-04.4d %d %d %d %s",
   	id, app_code, isin, chop_size, lines_screen, chars_line, zipfile_hold);
   else
      sprintf(sys_buf, "call zipchop %s %s %-04.4d %d %d %d", /* zipchop */
			id, app_code, isin, chop_size, lines_screen, chars_line);

   system(sys_buf);

	if(load_sw == NEW)
	{
		sini=isin;                      /* update sini          */
		strcpy(sinn, ssin);             /* update sinn          */
	}

	app_change_sw=TRUE;                     /* set app change sw    */

	update_log();                           /* update log           */

	delete_files();                         /* delete original files*/

	unlink(prodlzip_txt);                   /* list file, end usage */

/* load report deactivated currently */
/*      load_report();  */                      /* print load report    */
/* load report deactivated currently */

	if (0 != (f = fopen(locked, "r")))      /* try to open lock file*/
	{
		printf("\n\tThis Package Locked Out. To make it accessible, it must be Unlocked.");
		fclose(f);                      /* close lock file      */
	}
	err_msg("Reminder: ReCreate Selection Menus to activate ADDs or UPDATEs.");
	return(0);
}

copyf_to_disk()                         /* copy files: diskette to disk */
{
	zlst=fopen(prodlzip_txt, "r");          /* re-open PRODLZIP.LST */
	strcpy(filename, adir);                 /* prepare active drive */

	while(EOF != (fscanf(zlst, "%s\n", &filename[strlen(adir)])))
	{
		fcopy(filename, sin_dir);       /* copy current DL file */
	}
	fclose(zlst);

	return(0);
}

delete_files()                          /* delete files after zipchoped */
{
	zlst=fopen(prodlzip_txt, "r");          /* re-open PRODLZIP.LST */
	strcpy(filename, sin_dir);              /* prepare active drive */

	while(EOF != (fscanf(zlst, "%s\n", buffer)))
	{
		strcpy(filename, sin_dir);      /* prepare dir name     */
		strcat(filename, "\\");         /* add \ after dirname  */
		strcat(filename, buffer);       /* add filename after \ */
		unlink(filename);               /* delete file now      */
	}
	fclose(zlst);

	return(0);
}

load_report()                           /* produce report for this load */
{
	msg_get("Confirm Printer ready for Load Report. (Y/N): ");

	if(ch != 'Y') return(0);

	if (0 == (prt = fopen("PRN", "a")))   /* open printer file    */
	{
		perror("");
		printf("\n\tCannot open Printer file: %s\n", "PRN");
		return(0);
	}

	time(&ibtm);                          /* get time             */

	fprintf(prt, "Download Application: %s %s\t%s\n", id, app_name, ctime(&ibtm));

	if(load_sw == NEW)      fprintf(prt, "Add NEW ");
	else                    fprintf(prt, "Update  ");

	fprintf(prt, "Package.    Load Report for Package ID: %d\n", isin);
	fprintf(prt, "\nCONTROL.TXT file follows:\n\n");
	fflush(prt);                            /* write out            */

	sprintf(sys_buf, "copy %s\\%s PRN: > NUL:", sin_dir, control_txt);
	system(sys_buf);

	fprintf(prt, "\n\n\nDESCRIBE.TXT file follows:\n\n");
	fflush(prt);                            /* write out            */

	sprintf(sys_buf, "copy %s\\%s PRN: > NUL:", sin_dir, describe_txt);
	system(sys_buf);

	fprintf(prt, "\014");                   /* form feed            */
	fclose(prt);

	return(0);
}

fcopy(from, to)                         /* copy file from from to to    */
char *from, *to;
{
	sprintf(sys_buf, "copy %s %s > NUL:", from, to);
	system(sys_buf);                        /* copy file now        */
	return(0);
}

get_control_info()                      /* parse CONTROL.TXT            */
{
	if(load_show_sw == LOAD)                /* is it a LOAD req.    */
	{
		if(NULL == (zlst=fopen(prodlzip_txt, "w"))) /* open PRODLZIP.LST*/
		{
			bell();
			err_msg("Cannot open PRODLZIP.LST file.");
			return(r=-1);
		}
	}

	screen_head();                          /* show screen header   */

	if(NULL == (ctrl=fopen(control, "r")))
	{
		bell();
		printf("\n\t\Cannot open %s file.", control);
		wait_kb();
		return(-1);
	}
	parse_ctrl();                           /* parse CONTROL.TXT file*/
	fclose(ctrl);                           /* close CONTROL.TXT     */

	if (load_show_sw == LOAD) fclose(zlst); /* close PRODLZIP.LST   */

	if (bad_ctrl)                           /* bad lines found      */
	{
		err_msg("Check CONTROL.TXT file.");
		return(-1);                     /* dont load package    */
	}
						/* or missing mandatory lines */
	if(!(title1_sw & vdate_sw & version_sw & zipfile_sw & file_sw))
	{
		err_msg("Missing Mandatory Lines in CONTROL.TXT file.");
		return(-1);                     /* dont load package    */
	}

	return(0);
}

parse_ctrl()                            /* parse CONTROL.TXT file       */
{
	printf("\n");

	title1_sw=vdate_sw=version_sw=zipfile_sw=file_sw=FALSE;
   zipind_sw = FALSE;    /* IS ZIP OPTION INDICATOR PRESENT */
   category_sw = FALSE;  /* IS CONTRACT NUMBER PRESENT */
	bad_ctrl=FALSE;
   zip_file_cnt = 0;
	zipind[0] = 'Y';		/* ASSUME ZIP UNLESS TOLD DIFFERENT */
	printf("CONTROL.TXT file contents follows:\n");
	printf("==================================\n");
	while(NULL != (fgets(buffer, MAX_BUFF, ctrl)))  /* read file    */
	{
		if (strlen(buffer) == 0 || buffer[0] == '\n')
				continue; /* skip blank lines    */
		printf("%s", buffer);           /* show line on screen  */
		if (buffer[0] == 0x1A) break;   /* EOF, finish          */
		parse_line(buffer);
		if(r == -1) bad_ctrl=TRUE;
	}

    return(r=0);                                /* return, no error     */
}

parse_line(buf)                         /* parse individual line        */
char *buf;
{
	char *p;
	r=0;

	if (NULL == (p = strchr(buf, ':')))  /* look for :           */
	{
		printf("\t\aNo \":\" to seperate Keyword.\n");
		return (r=-1);
	}

	*p=0;                           /* terminate keyword with NULL  */
	p++;                            /* point to variable            */

	/* remove spaces at beginning of keyword      */
	while(isspace(*buf)) buf++;

	/* remove spaces at end of keyword      */
	while(isspace(buf[strlen(buf)-1])) buf[strlen(buf)-1]='\0';

	/* remove nl at end of variable         */
	while(p[strlen(p)-1] == '\n') p[strlen(p)-1]='\0'; /* remove nl */

	while(isspace(*p)) p++;         /* skip lead blanks on variable */

	if (0 == (strcmpi(buf, "title1")))
	{
		if(strlen(p) == 0 || strlen(p) > MAX_TITLE)
		{
			r=-1;
			printf("\t\aBad Title1.\n");
		}
		else
		{
			strcpy(title1, p);
			title1_sw=TRUE;
		}
	}
	else
	if (0 == (strcmpi(buf, "title2")))
	{
		if(strlen(p) == 0 || strlen(p) > MAX_TITLE)
		{
			r=-1;
			printf("\t\aBad Title2.\n");
		}
		else
		{
			strcpy(title2, p);
		}
	}
	else
	if (0 == (strcmpi(buf, "date")))
	{
		if(strlen(p) == 0 || strlen(p) > MAX_VDATE)
		{
			r=-1;
			printf("\t\aBad Date.\n");
		}
		else
		{
			strcpy(vdate, p);
			vdate_sw=TRUE;
		}
	}
	else
	if (0 == (strcmpi(buf, "version")))
	{
		if(strlen(p) == 0 || strlen(p) > MAX_VERSN)
		{
			r=-1;
			printf("\t\aBad Version.\n");
		}
		else
		{
			strcpy(version, p);
			version_sw=TRUE;
		}
	}
	else
	if (0 == (strcmpi(buf, "zipfile")))
	{
		if(strlen(p) == 0 || strlen(p) > MAX_ZIPFN)
		{
			r=-1;
			printf("\t\aBad Zipfile Name.\n");
		}
		else
		{
			strcpy(zipfile, p);
			zipfile_sw=TRUE;
		}
	}
	else
	if (0 == (strcmpi(buf, "zipind")))
	{
		if(strlen(p) == 0 || strlen(p) != 1)
		{
			r=-1;
			printf("\t\aBad Zipfile INDICATOR.\n");
		}
		else
		{
			strcpy(zipind, p);
			zipind_sw=TRUE;
		}
	}
	else
	if (0 == (strcmpi(buf, "CATEGORY")))
	{
		if(strlen(p) == 0 || strlen(p) > 15)
		{
			r=-1;
			printf("\t\A BAD CATEGORY FIELD.\n");
		}
		else
		{
			strcpy(CATEGORY, p);
			category_sw=TRUE;
		}
	}
	else
	if (0 == (strcmpi(buf, "file")))
	{
		if(load_show_sw == LOAD) chkfile(p);  /* check file exist, length */
	}
	else
	{
		printf("\t\aUnknown Keyword in Line.\n");
		return(r=-1);
	}
	return(r);
}

chkfile(buf)                    /* parse file line and check file size   */
char *buf;
{
	char *p;

	if(NULL == (p=strchr(buf, ',')))        /* no , in line         */
	{
		printf("\t\aMissing \",\" before Length.\n");
		return(r=-1);
	}

	if(0L == (ll=atol(p+1)))                /* bad length field     */
	{
		printf("\t\aBad Length.\n");
		return(r=-1);
	}

	*p=0;                                   /* terminate file name  */

	p=buf;                                  /* point to file name   */

	while(isspace(*p)) p++;                 /* skip leading blanks  */

	if(strlen(p) == 0 || strlen(p) > MAX_FILE)
	{
		printf("\t\aBad File Name.\n");
		return(r=-1);
	}
	strcpy(filename, adir);                 /* copy a:\dirname\     */
	strcat(filename, p);                    /* add filename to it   */

	if(NULL == (f=fopen(filename, "r")))    /* check file exists    */
	{
		bell();
		printf("\t\aFile not Found on %s.\n", filename, drv);
		return(r=-1);
	}

	fl=filelength(fileno(f));               /* get file size        */

	fclose(f);                              /* close current file   */

	if(ll != fl)                            /* file size differs    */
	{
		printf("\t\aFile length mismatch. Actual Length=%ld, CONTROL.TXT length=%ld\n",
			fl, ll);
		return(r=-1);
	}

			/* file is OK add it's name to PRODLZIP.LST     */
	fprintf(zlst, "%s\n", p);               /* file name only       */
	strcpy(zipfile_hold, p);  /*hold file name incase no zip is used*/
   ++zip_file_cnt;
	file_sw=TRUE;                           /* found good file line */
	return(0);
}

screen_head()                           /* show screen upper part       */
{
	system("cls");                          /* clear screen         */
	time(&ibtm);
	printf("Input Drive : %s\t\t\t\t\t%s", drv, ctime(&ibtm));
	printf("Output path : %s\t\t\t", out_path);
	printf("Current Directory: %s\n", getcwd(buffer, 50));
	printf("\n\t\tPUMS - Prodigy Upload Management Station\n");
	printf(  "\t\t========================================\n\n");

	printf("\t\tCurrent Application ID / Name: %4.4s / %s\n", id, app_name);
	printf("\t\tNumber of Current Packages   : %04.4d\n\n", sini);

	return(0);
}

check_sin_exist()
{
	sprintf(ssin, "%04.4d", isin);          /* edit isin            */

	strcpy(sin_control, dir);               /* make CSCS            */
	strcat(sin_control, "\\");              /* make CSCS\           */
	strcat(sin_control, id);                /* make CSCS\CSCS       */
	strcat(sin_control, ssin);              /* make CSCS\CSCSSINn   */

	strcpy(sin_dir, sin_control);           /* save sin_dir         */

	strcat(sin_control, "\\");              /* make CSCS\CSCSSINn\  */
	strcat(sin_control, control_txt);       /* add CONTROL.TXT      */

	if(NULL != (sctrl=fopen(sin_control, "r")))
	{
		fclose(sctrl);                   /* make sure to close   */
		return(TRUE);
	}
	else    return(FALSE);
}

mnu_maker()                             /* create fixed format menu file*/
{
	if(!check_applic_ok(1)) return(0);      /* check applic selected*/

	if(!app_change_sw)
		printf("\n\tNo changes to %s Application in this Session.", id);

	printf("\n\tConfirm Create Selection Menus for %s? (Y/N): ", id);
	read_kb(&ch, 1);
	if (toupper(ch) != 'Y') return(0);      /* no OK from user      */

	sprintf(sys_buf, "mnumaker %s %d %s", id, entries_screen, app_code);    /* call mnumaker        */
	system(sys_buf);

	app_change_sw=FALSE;                    /* indicate done        */
	audit(0);                               /* audit report         */
	wait_kb();
	return(0);
}

obj_to_tpf()                    /* zip copy objects to diskette         */
{
	struct diskfree_t ds;                   /* struct for space call*/

	if(!check_applic_ok(1)) return(0);      /* check applic selected*/

	screen_head();

	printf("\n\tYou Should Have Already Created the Selection Menu Objects.");
	printf("\n\tConfirm Create Object Delivery Diskette for %s. (Y/N): ", id);
	read_kb(&ch, 1);
	if (toupper(ch) != 'Y') return(0);

	sprintf(filename, "%s\\%s", id, upload_txt); /* create CSCS\UPLOAD.LST*/
	sprintf(oldfname, "%s\\%s", id, upload_old); /* create CSCS\UPLOAD.OLD*/


		if(NULL == (f=fopen(filename, "r")))      /* open UPLOAD.LST      */
		{
			printf("\n\tNo Objects Created since Last Transfer to TPF.");
			msg_get("Do you want to ReTransfer the last Object list? (Y/N): ");
			if(ch != 'Y') return(0);
			if(NULL == (f=fopen(oldfname, "r"))) /* try UPLOAD.OLD*/
			{
				err_msg("Last Object List Empty. Nothing to Transfer.");
				return(0);
			}
			fclose(f);              /* close UPLOAD.OLD     */
			fcopy(oldfname, filename);      /* copy UPLOAD.OLD 2 UPLOAD.LST*/
			f=fopen(filename, "r");         /* open UPLOAD.LST      */
		}

		dctr=0;                 /* reset diskette counter       */
		if(ask_for_diskette()) return(0);

		while(EOF != (fscanf(f, "%s\n", buffer)))
		{
			t=fopen(buffer, "r");           /* open current file */
			fl=filelength(fileno(t));       /* get file length   */
			fclose(t);                      /* close current file*/

			_dos_getdiskfree((unsigned)(out_path[0]-'A'+1), &ds);
			ll=((long)ds.avail_clusters*ds.sectors_per_cluster*ds.bytes_per_sector);
			if(fl > ll)
				{
					if(ask_for_diskette()) return(0);
				}

			fcopy(buffer, out_path);         /* copy current file */
		}
		fclose(f);
		audit(0);                       /* audit report         */
		msg_get("\n\tPrint Diskette Directory Contents? (Y/N): ");
		if(ch == 'Y')
		{
			sprintf(sys_buf, "dir %s > PRN:", out_path);
			system(sys_buf);        /* show on printer      */
		}
		ask_del_lst();                  /* ask del upload.lst   */
		return(0);
 }
	
ask_for_diskette()                      /* ask for empty diskette       */
{
	if(++dctr > 1)
	{
		msg_get("\n\tPrint Diskette Directory Contents? (Y/N): ");
		if(ch == 'Y')
		{
			sprintf(sys_buf, "dir %s > PRN:", out_path);
			system(sys_buf);        /* show on printer      */
		}
	}
	if ( out_path[0]=='A' || out_path[0]=='B') 
   	printf("\n\n\tPLEASE PUT EMPTY DISKETTE (%d) IN DRIVE %c ", dctr, out_path[0]);
   else
      printf("\n\n\tOUTPUT WILL BE SENT TO %s",out_path);
	wait_kb();


	if(-1 == (i=mkdir(out_path)))             /* try make directory   */
	{	 
		if (errno != EACCES)
		{	
		printf("\n\tUnable to create Sub Directory %s. Diskette may be NOT Empty.", out_path);
		wait_kb();
		return(-1);
		}
	}

	printf("\n\tCopying Object Files for %s onto Diskette (%d). Please Wait...", id, dctr);

	return(0);
}

ask_del_lst()                           /* after used, can be renamed   */
{
	unlink(oldfname);                       /* remove previous old  */
	rename(filename, oldfname);             /* create new old       */
	return(0);
}

update_log()                            /* update APPLCTRL.TXT file     */
{
	rewind(logf);                   /* rewind logfile to beginning  */
	fprintf(logf, "%04.4d %-25.25s %-s\n", sini, app_name, app_code);/* update log*/
	fflush(logf);                   /* flush out file               */

	return(0);
}

open_log(mode)                          /* open APPLCTRL.TXT file mode  */
char *mode;
{
	if(NULL == (logf=fopen(applctrl, mode)))
	{
		bell();
		printf("\n\tUnable to open %s in %s mode.\n",
					applctrl, mode);
		wait_kb();
		return(0);
	}
	return(0);
}

open_config()                           /* open PUMSETUP.CFG file        */
{
	if(NULL == (cfg=fopen(config_txt, "r"))) update_config(); /* create */
	fscanf(cfg, "%s %d\n", buffer, &chop_size);     /* read         */
	fscanf(cfg, "%s %d\n", buffer, &lines_screen);  /* control      */
	fscanf(cfg, "%s %d\n", buffer, &chars_line);    /* values       */
	fscanf(cfg, "%s %d\n", buffer, &entries_screen); /* into vars   */
	fscanf(cfg, "%s %s\n", buffer, drv);          
	fscanf(cfg, "%s %s\n", buffer, out_path);

	return(0);
}

update_config()                         /* update PUMSETUP.CFG          */
{
	if(NULL == (cfg=fopen(config_txt, "w")))
	{
		perror("");
		err_msg("Cannot create PUMSETUP.CFG file. Using Default Control Values.");
	}
	fprintf(cfg, "chop_size: %d\n", chop_size);
	fprintf(cfg, "lines_screen: %d\n", lines_screen);
	fprintf(cfg, "chars_line: %d\n", chars_line);
	fprintf(cfg, "entries_screen: %d\n", entries_screen);
	fprintf(cfg, "input_drive:  %s\n", drv);
	fprintf(cfg, "output_path: %s\n", out_path);

	fclose(cfg);
	return(0);
}

list_info()                             /* display SINn info on screen  */
{
	if(!check_applic_ok(1)) return(0);

	system("cls");                          /* clear screen         */

	load_show_sw=SHOW;                      /* set SHOW request     */
	strcpy(operator, "Display");            /* select operator      */

	snn=1;                                  /* make sure 1st time   */
	while(snn != 0) display_sinn();         /* loop on displays     */
	return(0);
}

display_sinn()
{
	if(!sini) return(0);                    /* no packages at all   */
	snn=get_sin();                          /* get SIN to display   */
	while(snn > sini || snn == 0 )
	{
		bell();
		printf("\n\tYOUR ENTRY %d IS NOT BETWEEN 1 AND %d", snn, sini);
		snn=get_sin();                  /* get SIN to display   */
	}
	if(snn == 0) return(0);                 /* finished             */

	printf("\n");

	control_name();                         /* CONTROL.TXT file name*/
	if(NULL == (ctrl=fopen(control, "r")))
	{
		bell();
		printf("\n\t\Cannot open %s.", control);
		read_kb(&ch, 1);
		return(-1);
	}
	parse_ctrl();
	fclose(ctrl);

	if (0 != (f=fopen(locked, "r+")))       /* open lock file*/
	{
		printf("\n\tThis Package LOCKED OUT.");
		fclose(f);
	}
	printf("\n");
	if(load_show_sw == SHOW) audit(0);      /* audit report         */

	return(0);
}

make_menu_name(cs)                      /* make menu file name          */
char *cs;
{
	strcpy(menu, cs);               /* copy CSCS                    */
	strcat(menu, "\\");             /* make app. directory CSCS\    */
	strcat(menu, cs);               /* make name CSCS\CSCS          */
	strcat(menu, "000");            /* make name CSCS\CSCS000       */

	msg_get("Sort by: A-Sin(\030), S-Sin(\031), T-Title, D-Date, Z-Zipfile, V-Version: ");

	if(ch == 'A')  strcat(menu, "0.MNU");
	else
	if(ch == 'S')  strcat(menu, "S.MNU");
	else
	if(ch == 'T')  strcat(menu, "T.MNU");
	else
	if(ch == 'D')  strcat(menu, "D.MNU");
	else
	if(ch == 'Z')  strcat(menu, "Z.MNU");
	else
	if(ch == 'V')  strcat(menu, "V.MNU");
	else make_menu_name(cs);                /* call itself          */

	return(0);
}

get_sin()                               /* get integer sin from KB      */
{
	printf("\n\tEnter SIN ID to %s, 1 to %d: ", operator , sini);
	num_sw=TRUE;                    /* numeric input only           */
	read_kb(sing, 4);
	return(atoi(sing));             /* convert to integer, return   */
}

lock_unlock()                              /* deactivate / reactivate package */
{
	if(!check_applic_ok(1)) return(0);

	system("cls");                          /* clear screen         */

	load_show_sw=LOCK;                      /* set SHOW request     */
	strcpy(operator, "Lock/Unlock");        /* select operator      */

	display_sinn();
	if(snn == 0) return(0);                 /* finished             */

	if (0 == (f = fopen(locked, "r")))      /* try to open lock file*/
	{
		msg_get("Confirm LOCK OUT Package now? (Y/N):");
		if(ch != 'Y') return(0);
		if (0 == (f = fopen(locked, "w"))) /* make lock file*/
		{
			perror("");
			printf("\n\tCannot open Lock file: %s\n", locked);
			return(0);
		}
		fclose(f);                      /* close lock file      */
		audit(1);
		app_change_sw=TRUE;
		printf("\n\tThis Package now Locked Out.");
	}
	else
	{
		fclose(f);                      /* close lock file      */
		msg_get("Confirm UNLOCK Package now? (Y/N):");
		if(ch != 'Y') return(0);
		unlink(locked);                 /* delete lock file     */
		audit(2);
		app_change_sw=TRUE;
		printf("\n\tThis Package now Unlocked.");
	}

	printf("\n\tYou must ReCreate Selection Menus and Deliver to TPF");
	err_msg("For Your Changes to Take Effect.");
	return(0);
}

print_info()                            /* print sinn menu report       */
{
int forf, fort, ford;                   /* for loop from, to, delta     */
int lc=0;

	if(!check_applic_ok(1)) return(0);

	make_menu_name(id);                     /* prepare menu filename*/

	if (0 == (mnu = fopen(menu, "r")))      /* open menu file       */
	{
		printf("\n\tCannot open menu file: %s\n", menu);
		err_msg("You have to create Selection Menu first");
		return(0);
	}

	msg_get("Confirm printer ready for Selection Menu report. (Y/N): ");

	if(ch != 'Y') return(0);

	if (0 == (prt = fopen("PRN", "a")))   /* open printer file    */
	{
		perror("");
		printf("\n\tCannot open Printer file: %s\n", "PRN");
		return(0);
	}
/* this code is commented out. Reinstate it if you want to be able to print you report
   in forward or backward sequence. If you do it, remove the ch='A'.

	msg_get("\r\tPlease select A=ascending D=descending order report. (A/D): ");

   This is the end of the comment out. next statement is part of the selection
   simulation.
*/
	ch='A';                                         /* simulate file order */

	entries=(filelength(fileno(mnu)))/ITEM_SIZE;    /* calc entries */

	if(ch == 'A')
	{
		forf=1; fort=entries+1; ford=+1;        /* go from 1 to last*/
	}
	else
	{
		forf=entries; fort=0; ford=-1;          /* go from last to 1*/
	}

	time(&ibtm);                            /* get time             */

	for(snn=forf; snn!=fort; snn+=ford)     /* get SINs to print    */
	{

		if( (((++lc)-1)%REP_LINES) == 0)
		{
			if(lc-1) fprintf(prt, "\014"); /* form feed on 2-n page */
			fprintf(prt, "Selection Menu Report\t\t\t\t%s\n", ctime(&ibtm));
			fprintf(prt, "Download Application: %s / %s\tCSCS Code: %s\n\n", id, app_name, app_code);

			fprintf(prt, "L SINn  Rel_Date  Version   Title                 ZipFile        Size\n");
			fprintf(prt, "================================================================================\n");
		}

		offset=(snn-1)*(ITEM_SIZE);
		fseek(mnu, offset, SEEK_SET);/* position 2 SINinfo */

		fgets(buff, ITEM_SIZE+1, mnu);          /* get item line*/

		sscanf(buff, "%c %8c %8c %4c %d %d %6c %d %20c %d %20c %d %12c %d %d %d",
		&lsw, vdate, version, sinn, &des, &zip, data_size, &title1_l, title1,
		&title2_l, title2, &zipfile_l, zipfile,
		&price_l, &category_l, &catnum_l);

/* The following is required to circumvent a bug in the fscanf library routine.
   It does not correctly parse a %20c empty field.
*/
		if(title2[0] == '|') title2[0]=' ';

		vdate[MAX_VDATE]=version[MAX_VERSN]=sinn[4]=0;
		title1[MAX_TITLE]=title2[MAX_TITLE]=zipfile[MAX_ZIPFN]=0;
		data_size[6]=0;

		for(i=0; (i<strlen(data_size)) && (data_size[i] == '0'); i++) data_size[i]=' ';

		if(lsw == 'L') fprintf(prt, "%c ", lsw);
		else           fprintf(prt, "  ");
		fprintf(prt, "%s  %s  %s  %s  %s  %s\n%48s\n\n",
		sinn, vdate, version, title1, zipfile, data_size, title2);
	}

	fprintf(prt, "L-marked Packages are Locked Out. Will not be included in Selection Menus.\n");

	if (entries < sini)             /* new packages not in menu yet */
	{
		fprintf(prt, "%d Package(s) are Currently added\nyet to be processed by Selection Menu Creation.\n"
				,sini-entries);
	}

	fprintf(prt, "\014\n");         /* form feed the printer*/

	fclose(mnu);                    /* close FF menu file   */
	fclose(prt);                    /* close printer file   */

	audit(0);                       /* audit report         */
	return(0);
}

err_msg(msg)                            /* show err msg and wait        */
char *msg;
{
	printf("\n\t%s", msg);
	wait_kb();
	return(0);
}

msg_get(msg)                            /* show err msg and get inp     */
char *msg;
{
	printf("\n\t%s", msg);
	read_kb(&ch, 1);
	ch=toupper(ch);                 /* upper case user's response   */
	return(0);
}

msg_wait(msg)                           /* show err msg and wait inp    */
char *msg;
{
	printf("\n\t%s", msg);
	get_one_key(&ch);
	return(0);
}


bell()                                  /* ring bell                    */
{
	putchar(BELL);
	return(0);
}

virus_scan()                            /* virus scan diskette/hard disk*/
{
	unsigned d;

	printf("\n\tSelect Virus Scan for D-Diskette or H-Hard Disk: ");
	read_kb(&ch, 1);
	if (toupper(ch) == 'D')
	{
		printf("\n\tConfirm Virus Scan Diskette %s (Y/N): ", drv);
		read_kb(&ch, 1);
		if (toupper(ch) != 'Y') return(0);
		scan_now(drv[0]);               /* scan diskette drv    */
		audit(1);                       /* audit report         */
	}
	else
	if (toupper(ch) == 'H')
	{
		_dos_getdrive(&d);              /* get drive number     */
		d+='A'-1;                      /* convert to letters   */
		printf("\n\tConfirm Virus Scan Hard Disk %c (Y/N): ", d);
		read_kb(&ch, 1);
		if (toupper(ch) != 'Y') return(0);
		scan_now(d);                    /* scan hard disk       */
		audit(2);                       /* audit report         */
	}
	return(0);
}

scan_now(what)                          /* virus scan selected drive    */
char what;
{
	system("cls");
	if (sel=='V') {
		printf("VIRUS Scan. Please Wait...\n\n");
	} else {
		printf("VIRUS Scan Diskette prior to Loading. Please Wait...\n\n");
	}
	sprintf(sys_buf, "scan %c:", what);
	system(sys_buf);                        /* do it now            */
	wait_kb();
	return(0);
}

select_drive()                          /* select diskette drive        */
{
	screen_head();
	printf("\n\n\tCurrent PUMS Control Values:\n");

	printf("\n\tINPUT WILL COME FROM DRIVE: %s", drv);
	printf("\n\tOUTPUT WILL GO TO PATH:  %s", out_path);

	printf("\n\n\tPress <ENTER> to Leave Parameter Unchanged.");
	printf("\n");
	printf("\n\tFROM WHICH DRIVE WILL INPUT TO PUMS COME?"); 
	printf("\n\tEnter drive selection, (A,B,C etc ): ");
	full_sw=FALSE;                           /* DO NOT insist on input      */
	read_kb(ndrv, 1);
   if (strlen(ndrv))
   	strcpy(drv, strupr(ndrv));        /* save new path*/

	audit(0);                               /* audit report         */
	printf("\n");
	printf("\n\tWHERE SHOULD OUTPUT FROM PUMS GO TO?"); 
	printf("\n\tEnter full path (ex. c:\\dir1\\dir2): ");
	full_sw=FALSE;                           /* insist on input      */
	read_kb(nout_path, 24);
   if (strlen(nout_path))
   	strcpy(out_path, strupr(nout_path));        /* save new path*/

	printf("\n");
	audit(0);
	err_msg("The NEW control values have been set");
	update_config(); 
	return(0);
}

show_dir()                              /* show drive directory         */
{
	printf("\n\tShow Directory for Drive %s (Y/N): ", drv);
	read_kb(&ch, 1);
	if(toupper(ch) != 'Y') return(0);       /* no dir required      */
	system("cls");
	sprintf(sys_buf, "dir %s:", drv);
	system(sys_buf);
	return(0);
}


 /*  get_passwd()                             get password to enter PUMS   */
/*{													 */
/*	echo_sw=FALSE;                          dont echo input      */
/*	printf("\n\nEnter Password: ");	 */
/*	read_kb(buffer, MAX_PASSWD);                       read passwd          */
/*	if(!strcmp(buffer, passwd_txt)) return(TRUE);      continue     */
/*	printf("\n\tIncorrect Password.");	  */

/*	printf("\n\nEnter Password: ");	 */
/*	read_kb(buffer, MAX_PASSWD);                       read passwd          */
/*	if(!strcmp(buffer, passwd_txt)) return(TRUE);      continue     */
/*	printf("\n\tIncorrect Password.");	 */

/*	printf("\n\nEnter Password: ");		  */
/*	read_kb(buffer, MAX_PASSWD);                       read passwd          */
/*	if(!strcmp(buffer, passwd_txt)) return(TRUE);       continue     */

/*	return(FALSE);                          tell no passwd       */
/*}	 */

wait_kb()                               /* show message and wait any key*/
{
	msg_wait("Press any key to continue. ");
	return(0);
}

show_a_dirs()                           /* show dirs selected diskette  */
{
	show_titles=TRUE;                       /* ask show titles      */
	sprintf(sys_buf, "%s:\\*.*", drv);
	return(show_dirs(sys_buf, "\n\t%s"));   /* dirs and show name   */
}

show_cs_dirs()                          /* show dirs current application*/
{
	show_titles=FALSE;                      /* dont check controls  */
	return(show_dirs("????", "\n\t%4.4s")); /* dirs show CSCS only  */
}

show_dirs(path, show_mask)      /* show names of directories on drive   */
char *path, *show_mask;
{

	screen_head();
	show_lines=MAX_LINES+1;                 /*force header display  */

  	if(0 != (_dos_findfirst(path, _A_SUBDIR, &buf_dir)))
 {  
      buffer[0] = '\0';
  	 	return(0);  
 } 
   else
       {
       look_title(buf_dir.name, show_mask);
       buffer[0] = '\0';
       }                 

	while(0 == (_dos_findnext(&buf_dir)))   /* check following      */
		if(buf_dir.attrib == _A_SUBDIR)        /* directory?    */
		{
         if (-1 == (look_title(buf_dir.name, show_mask))) break;
               buffer[0] = '\0';
      }
	return(0);
}

look_title(dname, mask)
char *dname, *mask;
{
	int (* ask_select_next)();              /* pointer to function  */
	char cname[MAX_FILEN], *p;

	if(!show_titles)                        /* if show A: dont worry*/
	{
		if(strlen(dname) < MAX_ID) return(0);   /* no CSCS start*/
	}

	if(show_lines == MAX_LINES)
	{
		if(show_titles) ask_select_pkg(1);
		else            ask_select_app(1);
		if(strlen(buffer)) return(-1);  /* selection made       */
		screen_head();
		show_lines++;                   /* cause header display */
	}
	if(show_lines > MAX_LINES)
	{
		if(show_titles)
		{
			printf("\n\tDirectory      \tDescription");
			printf("\n\t======================================");
		}
		else
		{
			printf("\n\tAppl.ID        Description");
			printf("\n\t======================================");
		}
		show_lines=0;                   /* reset line counter   */
	}

	show_lines++;                           /* count lines displayed*/
	printf(mask, dname);                    /* show dir name        */
	if(show_titles && strlen(dname)<8) printf("\t"); /* allign names*/

	if(!show_titles)                        /* show app names req   */
	{
		strcpy(cname, dname);           /* prepare file name    */
		strcat(cname, "\\");            /* for APPLCTRL.TXT on  */
		strcat(cname, applctrl_txt);    /* disk directory       */
		if(NULL == (ctrl=fopen(cname, "r"))) return(0); /* cannot open  */
		fscanf(ctrl, "%d %25c", &j, buffer); /* read line    */
		buffer[25]=0;
		printf("\t%s", buffer);         /* show appname         */
		fclose(ctrl);
		return(0);
	}

	strcpy(cname, drv);             /* prepare file name    */
	strcat(cname, "\\");            /* for CONTROL.TXT on   */
	strcat(cname, dname);           /* diskette directories */
	strcat(cname, "\\");
	strcat(cname, control_txt);

	title1[0]=title2[0]=0;                  /* empty strings        */

	if(NULL == (ctrl=fopen(cname, "r"))) return(0); /* cannot open  */

	while(NULL != (fgets(buffer, MAX_BUFF, ctrl)))  /* read file    */
	{
		if (buffer[0] == 0x1A) break;   /* EOF, finish          */
		if (NULL == (p=strchr(buffer, ':'))) return (0); /* look 4 : */

		*p=0;                           /* terminate keyword    */
		p++;                            /* point to variable    */
		while((*p) == ' ') p++;         /* skip lead spaces     */
		for(i=strlen(p)-1; i>=0; i--)
		{
			if((*(p+i) == '\n') ||( *(p+i) == ' ')) *(p+i)='\0';
			else    break;          /* remove trail sp or nl*/
		}
		if(strlen(p) > MAX_TITLE) *(p+MAX_TITLE)='\0';  /* vrfy length*/

		if (!(strcmpi(buffer, "title1"))) strcpy(title1, p);

		if (!(strcmpi(buffer, "title2"))) strcpy(title2, p);
	}

	fclose(ctrl);                           /* close control        */

	if(strlen(title1) || strlen(title2))    /* if titles found      */
	printf("\t%s %s", title1, title2);      /* show them            */

	return(0);
}

ask_select_pkg(i)
int i;
{
	if(i) printf("\n THERE ARE MORE DIRECTORIES - PRESS <ENTER> TO SEE NEXT PAGE OR");
	printf("\nENTER DIRECTORY NAME TO LOAD FROM: ");
	read_kb(buffer, MAX_ADIR);
	return(0);
}

ask_select_app(i)
int i;
{
	printf("\n\n\tSelect Application ID or Enter NEW ID");
	if(i) printf(", <ENTER> for Next Page");
	printf(": ");
	read_kb(buffer, MAX_ID);

	if(strlen(buffer) == 0) return(0);      /* no selection made    */
	for(j=0; j<MAX_ID; j++)                 /* if any input         */
	{                                       /* make sure it 4 chars */
		if( (buffer[j] == ' ')          /* reject any spaces    */
		||  (!isalnum(buffer[j])) )      /* must be alphanumeric */
		{
			printf("\n\n\tApplication Id must be 4 letters or digits.");
			ask_select_app(i);      /* call ourself         */
		}
	}
	return(0);
}


psetup()                        /* PUMS parameter setup         */
{
	long    tmp_chop_size;

	screen_head();
	printf("\n\n\tCurrent PUMS Control Values:\n");

	printf("\n\tZipFile Package Block Size      : %5d", chop_size);
	printf("\n\tDescription Lines on Screen     : %5d", lines_screen);
	printf("\n\tDescription Chars in Line       : %5d", chars_line);
	printf("\n\tSelection Menu Entries on Screen: %5d", entries_screen);


	printf("\n\n\tPress <ENTER> to Leave Parameter Unchanged.");
	printf("\n");

	do {
		printf("\n\tEnter New Value for Download Block Size : ");
		num_sw=TRUE;                    /* numeric input only   */
		read_kb(buffer, 5);
		if(strlen(buffer))  {
                        tmp_chop_size=atol(buffer);
                } else {
                        break;
                }
		if ( tmp_chop_size > CHOP_MAX ) {
			printf ("\n\tDownload block size must not be greater than %i\n", CHOP_MAX);
		} else {
			chop_size = tmp_chop_size;
		}
	} while ( tmp_chop_size > CHOP_MAX );

	printf("\n\tEnter New Value for Lines in Screen     : ");
	num_sw=TRUE;                            /* numeric input only   */
	read_kb(buffer, 2);
	if(strlen(buffer)) lines_screen=atoi(buffer);

	printf("\n\tEnter New Value for Characters in Line  : ");
	num_sw=TRUE;                            /* numeric input only   */
	read_kb(buffer, 2);
	if(strlen(buffer)) chars_line=atoi(buffer);

	printf("\n\tEnter New Value for Entries on Screen   : ");
	num_sw=TRUE;                            /* numeric input only   */
	read_kb(buffer, 2);
	if(strlen(buffer)) entries_screen=atoi(buffer);


	printf("\n");
	audit(0);                               /* audit report         */
	err_msg("The NEW Control Values Have Been Set");
	update_config();                        /* update config file   */
	return(0);
}

backup()                                /* PUMS backup procedure        */
{
	screen_head();

	msg_get("Select D-Diskette Backup, T-Cartridge Tape Backup: ");
	if(ch == 'D') sprintf(dort, "%s", drv);
	else
	if(ch == 'T') sprintf(dort, "%s", "tape");
	else return(0);

	msg_get("Select F-Full PUMS Backup, A-Application Only Backup: ");
	if(ch == 'F') sprintf(fora, "%s", getcwd(buffer, 50));
	else
	if(ch == 'A')
	{
		if(!check_applic_ok(0)) return(0);      /* select app 1st */
		sprintf(fora, "%s\\%s", getcwd(buffer, 50), id);
	}
	else return(0);


	msg_get("Confirm PUMS Backup NOW, (Y/N): ");
	if(ch != 'Y') return(0);                /* do nothing           */
	sprintf(sys_buf, "%s %s %s", backup_txt, fora, dort);
	system(sys_buf);                        /* do backup            */
	audit();                                /* audit report         */
	wait_kb();
	return(0);
}

check_applic_ok(e)                      /* verify application selected  */
int e;
{
	if(!strcmp(id, ""))             /* no applic selected   */
	{
		bell();
		err_msg("Please select Application ID first.");
		return(FALSE);
	}
	if(e && sini == 0)              /* must have packages to act on */
	{
		bell();
		err_msg("No Packages in this Application to Act On.");
		return(FALSE);
	}

	return(TRUE);
}

audit(i)                                /* write audit message to file  */
int i;
{
   if (audit_trail_sw = 'N') 	/* default is no audit trail  */
      return(0);
	time(&ibtm);
	fprintf(adt, "%.20s: ", &((ctime(&ibtm))[4])); /* write time stamp */

	switch(sel)              /* switch based on selection    */
	{
		case ' ':
			fprintf(adt, "INIT    Create NEW Audit Trail File");
			break;
		case '0':
			fprintf(adt, "ENTER   PUMS");
			break;
		case '1':
			if(i == 1)
			fprintf(adt, "SELECT  ");
			if(i == 2)
			fprintf(adt, "CREATE  ");
			fprintf(adt, "Appl %s/%s/%s", id, app_name, app_code);
			break;
		case '2':
			fprintf(adt, "ADD NEW %s%04.4d %s %s", id, sini, title1, title2);
			break;
		case '3':
			fprintf(adt, "UPD OLD %s%04.4d %s %s", id, isin, title1, title2);
			break;
		case '4':
			fprintf(adt, "MENUS   Recreation %s/%s", id, app_name);
			break;
		case '5':
			fprintf(adt, "DISPLAY %s%04.4d %s %s", id, snn, title1, title2);
			break;
		case '6':
			fprintf(adt, "PRINT   Selection Menu for %s/%s", id, app_name);
			break;
		case '7':
			fprintf(adt, "PREPARE Objects in %s/%s to TPF", id, app_name);
			break;
		case 'D':
			fprintf(adt, "SELECT  Diskette Drive %s", drv);
			break;
		case 'S':
			fprintf(adt, "SHOWDIR for Drive %s", drv);
			break;
		case 'V':
			if(i == 1)
			fprintf(adt, "VIRUSCK Scan Diskette Drive %s", drv);
			if(i == 2)
			fprintf(adt, "VIRUSCK Scan Hard Disk");
			break;
		case 'P':
			fprintf(adt, "SETUP   Parameter Setup. Block=%d, Lines=%d, Chars=%d, Entries=%d"
			,chop_size, lines_screen, chars_line, entries_screen);
			break;
		case 'B':
			fprintf(adt, "BACKUP  %s onto %s", fora, dort);
			break;
		case 'T':
			fprintf(adt, "AUDIT   Audit Trail Report");
			break;
		case 'L':
			if(i == 1) fprintf(adt, "LOCKOUT ");
			if(i == 2) fprintf(adt, "UNLOCK  ");
			fprintf(adt, "%s%04.4d %s %s", id, snn, title1, title2);
			break;
		case 'X':
			fprintf(adt, "EXIT    PUMS\n\n");
			break;
		default:
			fprintf(adt, "UNKNOWN action report");
			break;
	}
	fprintf(adt, "\n");             /* put end of line              */
	fflush(adt);                    /* always flush audit, in case  */
	return(0);
}

print_audit()
{
	msg_get("Confirm Printer ready for Audit Trail Report. (Y/N): ");
	if(ch == 'Y')
	{
		audit(0);                       /* audit report         */
		fcopy(audit_txt, "PRN");        /* print report now     */
	}

	msg_get("Confirm DELETE Current Audit Trail File. (Y/N): ");
	if(ch != 'Y') return(0);
  /*	if(!get_passwd()) return(0);          dont delete          */
	fclose(adt);
	unlink(audit_txt);
	open_audit();
	return(0);
}

open_audit()                            /* prepare audit trail file     */
{
	if(NULL == (adt=fopen(audit_txt, "a")))
	{
		perror("");                     /* show system error    */
		printf("Cannot open %s file.\n", audit_txt);
		return(0);
	}

	if(filelength(fileno(adt)) == 0)
	{
		sel=' ';                        /* create audit trail   */
		audit(0);
	}

	if(filelength(fileno(adt)) > MAX_AUDIT)
	{
		printf("\n\tPUMS Audit Trail File VERY LARGE. Suggest you Print and DELETE.");
		sel='T';                        /* simulate T selected  */
		print_audit();
	}

	return(0);

}
/*   diskette_ready()                         will find if diskette not ready */
/* {
	unsigned status;
	void (far *pums_err)();                  our error handler    */


/*	disk_info.drive=drv[0]-'A';             make drive number    */
/*	disk_info.head=0;
	disk_info.track=disk_info.sector=disk_info.nsectors=1;
	disk_info.buffer=buffer;

	_harderr(pums_err);                     set hw err handler   */

/*	_bios_disk(_DISK_VERIFY, &disk_info);  dummy read, required */

/*	while(0x0000 != ((_bios_disk(_DISK_VERIFY, &disk_info)) & 0xFF00))
	{
		bell();                          error while reading  */
 /*		printf("\n\t\tPlace Diskette in Drive %s Press any key when ready: ", drv);
		read_kb(&ch, 1);
		_bios_disk(_DISK_VERIFY, &disk_info);  dummy read, required */
/*	}
	return(0);
}

pums_err(de, ec, dh)                    our hw error handler         */
/* unsigned de, ec;
unsigned far *dh;
{
	_hardresume(_HARDERR_FAIL);             error? go report     */
/*	return(0);
}
*/
