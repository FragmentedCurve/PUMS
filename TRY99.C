#include <dos.h>

main ()

{

	struct	find_t	c_file;
	
	/* find first dir of ???? in current directory.			    */
 /*	if ( !(_dos_findfirst ("????", _A_SUBDIR, &c_file)) ) {	*/
   printf ("aa %x aa", _dos_findfirst ("????", _A_SUBDIR, &c_file));
		printf ("First dir = %s\n", c_file.name);
 /*	}	*/
	
	/* Find rest.							    */
	while ( !(_dos_findnext (&c_file) ) ) {
		printf ("Addn dir = %s\n", c_file.name);
	}

}
