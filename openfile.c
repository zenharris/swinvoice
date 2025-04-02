
#include stdio
#include stdlib
#include rms
#include ints
#include starlet

struct timebuff
   {
      uint32 b1,b2;
   };


#include "filebrowse.h"

int rms_status;

/*
#define error_exit(format,filedef) error_exit(format,filedef)
*/

static void error(operation,filedef)
const char *operation;
struct filestruct *filedef;
{
   printf("RMSEXP - file %s failed (%s)\n",operation,filedef->filename);
   exit(rms_status);
}


void open_file(filedef)
struct filestruct *filedef;
{

   (*filedef->initialize)(filedef->filename);

   rms_status = sys$create(&filedef->fab);
   if (rms_status != RMS$_NORMAL &&
       rms_status != RMS$_CREATED)
      error("$OPEN",filedef);

   if (rms_status == RMS$_CREATED)
      printf("[Created New Data file.]\n");

   rms_status = sys$connect(&filedef->rab);
   if (rms_status != RMS$_NORMAL)
      error("$CONNECT",filedef);
}


