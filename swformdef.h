

/*
struct scr_params {
    int row,col;
    char *prompt;
    char prattr;
    unsigned long format;
    void *field;
    int fattr;
    int lnth;
    int (*validate)();
    struct cmndline_params *cmndline;
};
*/
/*
capital(char *in_str)
{
   strupr(in_str);
   return(TRUE);
}
*/

extern int lock_read_master ();
extern void write_master_record ();


int capital(char *in_str);

void response (struct linedt_msg_rec *msg)
{
    char blank[81];

    memset (blank,' ',msg->lnth);
    blank[msg->lnth] = 0;
    exscrprn(blank,msg->row,msg->col,msg->attr);
    strcpy (msg->edtstr,msg->cmndline->prompt);
    exscrprn(msg->edtstr,msg->row,msg->col,msg->attr);
}

uint16 Find_Free_Joic_Key (filedef)
struct filestruct *filedef;
{
   int i;
   uint16 searchkey;

   if(!lock_read_master()) {
      warn_user("Master Locked");
      return(FALSE);
   }

   filedef->rab.rab$b_krf = CLNT_CUST_KEY;
   filedef->rab.rab$l_kbf = (char *)&searchkey;
   filedef->rab.rab$b_ksz = SIZE_BN2;
   filedef->rab.rab$b_rac = RAB$C_KEY;
   filedef->rab.rab$l_rop = RAB$M_CDK;  /* check for duplicate key  */

   while((searchkey=master_buff.cust_no++) < 65535) {

      rms_status = sys$find(&filedef->rab);

      if (!rmstest(rms_status,2,RMS$_NORMAL,RMS$_RNF)) {
         warn_user("$CHECK %s",rmserror(rms_status,filedef));
         break;          
      } else
         if (rms_status == RMS$_RNF) {
            write_master_record(); 
            return(searchkey);
         }
   }
   sys$free(&master_file.rab);
   return(FALSE);
}

void alloc_joic(msg,filedef)
struct linedt_msg_rec *msg;
struct filestruct *filedef;
{
        char blank[10];

    memset (blank,' ',msg->lnth);
    blank[msg->lnth] = 0;
    exscrprn(blank,msg->row,msg->col,msg->attr);
    sprintf (msg->edtstr,"%-5u",Find_Free_Joic_Key(filedef));
    exscrprn(msg->edtstr,msg->row,msg->col,msg->attr);


}

void tod_datetime (struct linedt_msg_rec *msg)
{
   unsigned short int numvec[7];
   struct timebuff time_now;
   int status;

   status = sys$gettim (&time_now);

   status = sys$numtim (numvec,
                            &time_now);

   sprintf (msg->edtstr,"%02hu/%02hu/%04hu %02hu:%02hu:%02hu.%02hu",
                  numvec[2],
                  numvec[1],
                  numvec[0],
                  numvec[3],
                  numvec[4],
                  numvec[5],
                  numvec[6]);
   exscrprn(msg->edtstr,msg->row,msg->col,msg->attr);
 
}

void tod_date (struct linedt_msg_rec *msg)
{
   unsigned short int numvec[7];
   struct timebuff time_now;
   int status;


   status = sys$gettim (&time_now);

   status = sys$numtim (numvec,
                            &time_now);

   sprintf (msg->edtstr,"%02hu/%02hu/%04hu",
                  numvec[2],
                  numvec[1],
                  numvec[0]);
   exscrprn(msg->edtstr,msg->row,msg->col,msg->attr);
 
}


static struct cmndline_params datetime_response[] = {
        1, NULL, NULL, 0, NULL,
        F2 , "Date", NULL, 0, tod_datetime
};
static struct cmndline_params date_response[] = {
        1, NULL, NULL, 0, NULL,
        F2 , "Date", NULL, 0, tod_date
};




struct cmndline_params pump_responses[] = {
    3,NULL,NULL,0,NULL,
    F1 ,"NOVA600",NULL,0,response,
    F2 ,"SHINMEIWA",NULL,0,response,
    F3 ,"SAKURAGAWA",NULL,0,response
};

struct cmndline_params blower_responses[] = {
    4,NULL,NULL,0,NULL,
    F1 ,"DF120",NULL,0,response,
    F2 ,"LP100",NULL,0,response,
    F3 ,"LP70",NULL,0,response,
    F4 ,"LP60",NULL,0,response
};

struct cmndline_params model_responses[] = {
    10,NULL,NULL,0,NULL,
    F1 ,"11",NULL,0,response,
    F2 ,"12",NULL,0,response,
    F3 ,"13",NULL,0,response,
    F4 ,"MK6",NULL,0,response,
    F5 ,"COMMERCIAL",NULL,0,response,
    F6 ,"15",NULL,0,response,
    F7 ,"23",NULL,0,response,
    F8 ,"50",NULL,0,response,
    F9 ,"75",NULL,0,response,
    F10,"110",NULL,0,response
};


struct cmndline_params joic_responses[] = {
    1,NULL,NULL,0,NULL,
    F1 ,"Allocate",NULL,0,alloc_joic,
    F2 ,"Scan",NULL,0,NULL
};


#define LIGHTGRAY 0
#define WHITE SMG$M_BOLD




#define COMMENTLINE 16
#define PRISCRNLEN 18
struct scr_params primary_entry_screen[] = {
    1,0,"Name    :",LIGHTGRAY,UNFMT_STRING|CAPITALIZE,fileio.record.name,WHITE,30,capital,NULL,
    2,0,"Address :",LIGHTGRAY,UNFMT_STRING|CAPITALIZE,fileio.record.address,WHITE,30,NULL,NULL,
    3,0,"Suburb  :",LIGHTGRAY,UNFMT_STRING|CAPITALIZE,fileio.record.suburb,WHITE,30,NULL,NULL,
    4,0,"State   :",LIGHTGRAY,UNFMT_STRING|CAPITALIZE,fileio.record.state,WHITE,30,capital,NULL,
    5,0,"P/Code  :",LIGHTGRAY,UNFMT_STRING|CAPITALIZE,fileio.record.postcode,WHITE,10,NULL,NULL,
    6,0,"Country :",LIGHTGRAY,UNFMT_STRING|CAPITALIZE,fileio.record.country,WHITE,30,capital,NULL,
    7,0,"Email   :",LIGHTGRAY,UNFMT_STRING,fileio.record.email,WHITE,60,NULL,NULL,
/*    8,0,"Date:",LIGHTGRAY,BTRV_DATE,&fileio.record.inst_date,WHITE,10,NULL,date_response, */
    8,0,"Date    :",LIGHTGRAY,BTRV_DATE_TIME,&fileio.record.inst_date,WHITE,22,NULL,datetime_response,
    1,39,"Postal-:",LIGHTGRAY,UNFMT_STRING|CAPITALIZE,fileio.record.p_address1,WHITE,30,NULL,NULL,
    2,39,"Address:",LIGHTGRAY,UNFMT_STRING|CAPITALIZE,fileio.record.p_address2,WHITE,30,NULL,NULL,
    3,39,"       :",LIGHTGRAY,UNFMT_STRING|CAPITALIZE,fileio.record.p_address3,WHITE,30,NULL,NULL,
    4,39,"       :",LIGHTGRAY,UNFMT_STRING|CAPITALIZE,fileio.record.p_address4,WHITE,30,NULL,NULL,
    5,39,"B/Phone:",LIGHTGRAY,UNFMT_STRING,fileio.record.b_phone,WHITE,19,NULL,NULL,
    6,39,"H/Phone:",LIGHTGRAY,UNFMT_STRING,fileio.record.h_phone,WHITE,19,NULL,NULL,
    8,39,"Run No : %5hu",LIGHTGRAY,NUMERIC,&fileio.record.run,WHITE,5,NULL,NULL,
    8,56,"Cust No: %5.5hu",LIGHTGRAY,NUMERIC|NON_EDIT,&fileio.record.joic_no,WHITE,5,NULL,joic_responses,
    10,19,"Comments",WHITE,FREE_TEXT|WORD_WRAP,NULL,WHITE,700,linedt,NULL,
    0,0,"Customer Record",WHITE,0,NULL,0,0,NULL,NULL
};

/********************************************************************************************/
/*   Service File Definitions   */

static struct cmndline_params odour_responses[] = {
    3,NULL,NULL,0,NULL,
    F1 ,"Sewage",NULL,0,response,
    F2 ,"Soil",NULL,0,response,
    F3 ,"Normal",NULL,0,response
};

static struct cmndline_params on_off_responses[] = {
    2,NULL,NULL,0,NULL,
    F1 ,"ON",NULL,0,response,
    F2 ,"OFF",NULL,0,response
};
static struct cmndline_params yes_no_responses[] = {
    2,NULL,NULL,0,NULL,
    F1 ,"YES",NULL,0,response,
    F2 ,"NO",NULL,0,response
};



#define SERVSCRNLEN 31
#define SERVTEXTLN 29
#define SERVDATELN 3
struct scr_params service_entry_screen[] = {
    1,0,"Service Record",WHITE,0,NULL,0,0,NULL,NULL,
    2,0,"Name   :",LIGHTGRAY,UNFMT_STRING|NON_EDIT,fileio.record.name,WHITE,39,NULL,NULL,
    2,52,"Joic No:%5hu",LIGHTGRAY,NUMERIC|NON_EDIT,&fileio.record.joic_no,WHITE,5,NULL,NULL,
    4,0,"Date and Time of Service:",LIGHTGRAY,BTRV_DATE_TIME,&servio.record.service_datetime,WHITE,22,NULL,datetime_response,
    4,50,"Tech. :",LIGHTGRAY,UNFMT_STRING|CAPITALIZE,servio.record.tech,WHITE,20,NULL,NULL,
    6,0,"Clarity 1  :",LIGHTGRAY,UNFMT_STRING,servio.record.clar1,WHITE,2,NULL,NULL,
    6,24,"Clarity 2 :",LIGHTGRAY,UNFMT_STRING,servio.record.clar2,WHITE,2,NULL,NULL,
    6,49,"Ph 1 :",LIGHTGRAY,UNFMT_STRING,servio.record.ph1,WHITE,3,NULL,NULL,
    6,68,"Ph 2 :",LIGHTGRAY,UNFMT_STRING,servio.record.ph2,WHITE,3,NULL,NULL,
    7,0,"Aer.Sludge :",LIGHTGRAY,UNFMT_STRING,servio.record.aersludg,WHITE,2,NULL,NULL,
    7,49,"DO 1 :",LIGHTGRAY,UNFMT_STRING,servio.record.do1,WHITE,3,NULL,NULL,
    7,68,"DO 2 :",LIGHTGRAY,UNFMT_STRING,servio.record.do2,WHITE,3,NULL,NULL,
    8,0,"Sed.Sludge :",LIGHTGRAY,UNFMT_STRING,servio.record.sedsludg,WHITE,2,NULL,NULL,
    8,30,"Scum:",LIGHTGRAY,UNFMT_STRING,servio.record.sed,WHITE,1,capital,NULL,
    8,41,"Scum Removed :",LIGHTGRAY,UNFMT_STRING,servio.record.scum_rem,WHITE,3,NULL,yes_no_responses,
    8,61,"Sept Sludge :",LIGHTGRAY,UNFMT_STRING,servio.record.tanksludg,WHITE,3,NULL,NULL,
    9,30,"RcL :",LIGHTGRAY,UNFMT_STRING,servio.record.rcl,WHITE,3,NULL,NULL,
    9,45,"Existing :",LIGHTGRAY,UNFMT_STRING,servio.record.chlorexi,WHITE,3,NULL,NULL,
    9,68,"Repl :",LIGHTGRAY,UNFMT_STRING,servio.record.chlotrep,WHITE,3,NULL,NULL,
    10,5,"F.E.C :",LIGHTGRAY,UNFMT_STRING,servio.record.fec,WHITE,5,NULL,NULL,
    10,23,"Settlement :",LIGHTGRAY,UNFMT_STRING,servio.record.settle,WHITE,5,NULL,NULL,
    10,49,"S.V. :",LIGHTGRAY,UNFMT_STRING,servio.record.sv,WHITE,5,NULL,NULL,
    11,7,"S/S :",LIGHTGRAY,UNFMT_STRING,servio.record.ss,WHITE,3,NULL,on_off_responses,
    11,30,"S/R :",LIGHTGRAY,UNFMT_STRING,servio.record.sr,WHITE,3,NULL,on_off_responses,
    13,0,"Pump Operating:",LIGHTGRAY,UNFMT_STRING,servio.record.pump,WHITE,1,capital,NULL,
    13,48,"Odour :",LIGHTGRAY,UNFMT_STRING|CAPITALIZE,servio.record.odour,WHITE,6,NULL,odour_responses,
    14,0,"Blower Sound :",LIGHTGRAY,UNFMT_STRING|CAPITALIZE,servio.record.blowsoun,WHITE,8,NULL,NULL,
    14,27,"Filter :",LIGHTGRAY,UNFMT_STRING,servio.record.blowfilt,WHITE,1,capital,NULL,
    14,42,"Water Meter :",LIGHTGRAY,UNFMT_STRING,servio.record.watermet,WHITE,6,NULL,NULL,
    16,19,"Observations / Recommendations",WHITE,FREE_TEXT|WORD_WRAP,NULL,WHITE,700,linedt,NULL,
    9,2,"Chlorine :",WHITE,0,NULL,0,0,NULL,NULL
};





