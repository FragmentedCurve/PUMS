#include <dos.h>
#include <string.h>
#include <stdio.h>

main ()

{

  char testit[] = "this is a test";
  strupr(testit);
  printf("%s",testit);
  printf("%s",strupr(testit));



}
