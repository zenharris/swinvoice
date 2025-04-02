#define LINE_LENGTH 100
#define TOTAL_LINES 50

#define SIZE_NAME 30
#define SIZE_ADDRESS 30
#define SIZE_DATIME 8
#define SIZE_JOIC 2
#define SIZE_RUN 2
#define SIZE_UID 16
#define SIZE_COUNTRY 30
#define SIZE_QUOTECLASS 15
#define SIZE_INTEGER 2
#define SIZE_BN2 2
#define SIZE_BN4 4
#define SIZE_BN8 8

#define PRIM_RECORD_SIZE (sizeof fileio.record)
#define SERV_RECORD_SIZE (sizeof servio.record)

#define DEFAULT_FILE_NAME ".dat"

#define CLASSKEYLEN 16
#define PARTKEYLEN 13


struct XABKEY lstfrwd_key,lstrev_key,joic_key,run_key,name_key,country_key;
struct XABKEY quoteclass_key;
void PrimaryFileInit(char *);

/*
struct timebuff
   {
      uint32 b1,b2;
   };

*/


#pragma nomember_alignment


struct primary_rec {
   char name[SIZE_NAME+1];
   char address[SIZE_ADDRESS+1];
   uint16 joic_no;
   uint16 run;
   char suburb[31];
   char state[31];
   char postcode[11];
   char country[SIZE_COUNTRY+1];
   char p_address1[31];
   char p_address2[31];
   char p_address3[31];
   char p_address4[31];
   char b_phone[20];
   char h_phone[20];
   char email[61];
   struct timebuff inst_date;
   int16 reclen;
};

struct iorec
   {
       struct primary_rec record;
       char lines[TOTAL_LINES][LINE_LENGTH];
   }fileio;


struct filestruct PrimFile;

#define CLNT_CUST_KEY 0
#define CLNT_NAMECUST_KEY 1
#define CLNT_FORWARD_KEY 1
#define CLNT_REVERSE_KEY 2
#define CLNT_RUN_KEY 3
#define CLNT_NAME_KEY 4
#define CLNT_COUNTRY_KEY 5

#ifdef INC_INITS
void PrimaryFileInit(fn) char *fn;
{
   PrimFile.fab = cc$rms_fab;
   PrimFile.fab.fab$b_bks = 16;
   PrimFile.fab.fab$l_dna = DEFAULT_FILE_NAME;
   PrimFile.fab.fab$b_dns = sizeof DEFAULT_FILE_NAME -1;
   PrimFile.fab.fab$b_fac = FAB$M_DEL | FAB$M_GET |  FAB$M_PUT | FAB$M_UPD;
   PrimFile.fab.fab$l_fna = fn;
   PrimFile.fab.fab$b_fns = strlen(fn);
   PrimFile.fab.fab$l_fop = FAB$M_CIF;
   PrimFile.fab.fab$w_mrs = 0;  /*RECORD_SIZE; */
   PrimFile.fab.fab$b_org = FAB$C_IDX;
   PrimFile.fab.fab$b_rat = FAB$M_CR;
   PrimFile.fab.fab$b_rfm = FAB$C_VAR;  /*FAB$C_FIX; */
   PrimFile.fab.fab$b_shr = FAB$V_SHRDEL | FAB$V_SHRGET | FAB$V_SHRPUT | FAB$V_SHRUPD;  /*FAB$M_NIL;  */
   PrimFile.fab.fab$l_xab = &joic_key;

   PrimFile.rab = cc$rms_rab;
   PrimFile.rab.rab$l_fab = &PrimFile.fab;


   joic_key = cc$rms_xabkey;
   
   joic_key.xab$b_dtp = XAB$C_BN2;
   joic_key.xab$b_flg = 0;
   joic_key.xab$w_pos0 = ((char *) &fileio.record.joic_no - (char *)&fileio.record) ;
   joic_key.xab$b_siz0 = SIZE_JOIC;
   joic_key.xab$b_ref = CLNT_CUST_KEY;
   joic_key.xab$l_nxt = &lstfrwd_key;


   lstfrwd_key = cc$rms_xabkey;
   
   lstfrwd_key.xab$b_dtp = XAB$C_STG;
   lstfrwd_key.xab$b_flg = XAB$M_DUP| XAB$M_CHG;
   lstfrwd_key.xab$w_pos0 = ((char *) &fileio.record.name - (char *) &fileio.record) ; 
   lstfrwd_key.xab$b_siz0 = SIZE_NAME;
   lstfrwd_key.xab$w_pos1 = ((char *) &fileio.record.joic_no - (char *)&fileio.record) ;
   lstfrwd_key.xab$b_siz1 = SIZE_JOIC;
   lstfrwd_key.xab$b_ref = CLNT_NAMECUST_KEY;
   lstfrwd_key.xab$l_nxt = &lstrev_key;


   lstrev_key = cc$rms_xabkey;
   
   lstrev_key.xab$b_dtp = XAB$C_DSTG;
   lstrev_key.xab$b_flg = XAB$M_DUP | XAB$M_CHG;
   lstrev_key.xab$w_pos0 = ((char *) &fileio.record.name - (char *) &fileio.record) ; 
   lstrev_key.xab$b_siz0 = SIZE_NAME;
   lstrev_key.xab$w_pos1 = ((char *) &fileio.record.joic_no - (char *)&fileio.record) ;
   lstrev_key.xab$b_siz1 = SIZE_JOIC;
   lstrev_key.xab$b_ref = CLNT_REVERSE_KEY;
   lstrev_key.xab$l_nxt = &run_key;
 
   run_key = cc$rms_xabkey;

   run_key.xab$b_dtp = XAB$C_BN2;
   run_key.xab$b_flg = XAB$M_DUP | XAB$M_CHG | XAB$M_NUL;
   run_key.xab$w_pos0 = ((char *)&fileio.record.run - (char *)&fileio.record);
   run_key.xab$b_siz0 = SIZE_RUN;
   run_key.xab$b_ref = CLNT_RUN_KEY;
   run_key.xab$l_nxt = &name_key;

   name_key = cc$rms_xabkey;
   
   name_key.xab$b_dtp = XAB$C_STG;
   name_key.xab$b_flg = XAB$M_DUP | XAB$M_CHG;
   name_key.xab$w_pos0 = ((char *) &fileio.record.name - (char *) &fileio.record) ; 
   name_key.xab$b_siz0 = SIZE_NAME;
   name_key.xab$b_ref = CLNT_NAME_KEY;
   name_key.xab$l_nxt = &country_key;

   country_key = cc$rms_xabkey;
   
   country_key.xab$b_dtp = XAB$C_STG;
   country_key.xab$b_flg = XAB$M_DUP | XAB$M_CHG | XAB$M_NUL;
   country_key.xab$b_nul = ' ';
   country_key.xab$w_pos0 = ((char *) &fileio.record.country - (char *) &fileio.record) ; 
   country_key.xab$b_siz0 = SIZE_COUNTRY;
   country_key.xab$b_ref = CLNT_COUNTRY_KEY;


}
#endif

struct service_rec {
   uint16 joic_no;
   struct timebuff service_datetime;
   char ascdate[SIZE_UID+1];
   char tech[21];
   char clar1[3];
   char clar2[3];
   char ph1[4];
   char ph2[4];
   char aersludg[3];
   char sedsludg[3];
   char do1[4];
   char do2[4];
   char sed[2];
   char rcl[4];
   char tanksludg[4];
   char chlorexi[4];
   char chlotrep[4];
   char blowsoun[9];
   char blowfilt[2];
   char odour[7];
   char pump[2];
   char watermet[7];

        char fec[6];
        char settle[6];
        char sv[6];
        char ss[4];
        char sr[4];
        char scum_rem[4];

   int16 reclen;
};

struct serviorec
   {
       struct service_rec record;
       char lines[TOTAL_LINES][LINE_LENGTH];
   }servio;

 
struct filestruct ServFile;

struct XABKEY srvfrwd_key,srvrev_key,srvjoic_key;

#define SERV_CUST_KEY 0
#define SERV_CUSTDATE_KEY 1
#define SERV_FORWARD_KEY 1
#define SERV_REVERSE_KEY 2

#ifdef INC_INITS
void ServiceFileInit(fn) char *fn;
{
   ServFile.fab = cc$rms_fab;
   ServFile.fab.fab$b_bks = 16;
   ServFile.fab.fab$l_dna = DEFAULT_FILE_NAME;
   ServFile.fab.fab$b_dns = sizeof DEFAULT_FILE_NAME -1;
   ServFile.fab.fab$b_fac = FAB$M_DEL | FAB$M_GET |  FAB$M_PUT | FAB$M_UPD;
   ServFile.fab.fab$l_fna = fn;
   ServFile.fab.fab$b_fns = strlen(fn);
   ServFile.fab.fab$l_fop = FAB$M_CIF;
   ServFile.fab.fab$w_mrs = 0;  /*RECORD_SIZE; */
   ServFile.fab.fab$b_org = FAB$C_IDX;
   ServFile.fab.fab$b_rat = FAB$M_CR;
   ServFile.fab.fab$b_rfm = FAB$C_VAR;  /*FAB$C_FIX; */
   ServFile.fab.fab$b_shr = FAB$V_SHRDEL | FAB$V_SHRGET | FAB$V_SHRPUT | FAB$V_SHRUPD;  /*FAB$M_NIL;  */
   ServFile.fab.fab$l_xab = &srvjoic_key;

   ServFile.rab = cc$rms_rab;
   ServFile.rab.rab$l_fab = &ServFile.fab;

                      
   srvjoic_key = cc$rms_xabkey;
   
   srvjoic_key.xab$b_dtp = XAB$C_BN2;
   srvjoic_key.xab$b_flg = XAB$M_DUP; /* 0; */
   srvjoic_key.xab$w_pos0 = ((char *) &servio.record.joic_no - (char *)&servio.record) ;
   srvjoic_key.xab$b_siz0 = SIZE_JOIC;
   srvjoic_key.xab$b_ref = SERV_CUST_KEY;
   srvjoic_key.xab$l_nxt = &srvfrwd_key;


   srvfrwd_key = cc$rms_xabkey;
   
   srvfrwd_key.xab$b_dtp = XAB$C_STG;
   srvfrwd_key.xab$b_flg = XAB$M_CHG;
   srvfrwd_key.xab$w_pos0 = ((char *) &servio.record.joic_no - (char *)&servio.record) ;
   srvfrwd_key.xab$b_siz0 = SIZE_JOIC;
   srvfrwd_key.xab$w_pos1 = ((char *) &servio.record.ascdate - (char *) &servio.record) ; 
   srvfrwd_key.xab$b_siz1 = SIZE_UID;
   srvfrwd_key.xab$b_ref = SERV_CUSTDATE_KEY;
   srvfrwd_key.xab$l_nxt = &srvrev_key;

   srvrev_key = cc$rms_xabkey;
   
   srvrev_key.xab$b_dtp = XAB$C_DSTG;
   srvrev_key.xab$b_flg = XAB$M_CHG;
   srvrev_key.xab$w_pos0 = ((char *) &servio.record.joic_no - (char *)&servio.record) ;
   srvrev_key.xab$b_siz0 = SIZE_JOIC;
   srvrev_key.xab$w_pos1 = ((char *) &servio.record.ascdate - (char *) &servio.record) ; 
   srvrev_key.xab$b_siz1 = SIZE_UID;
   srvrev_key.xab$b_ref = SERV_REVERSE_KEY; 

}
#endif


struct inventory_rec {
	char part_code[PARTKEYLEN+1];
	char class[CLASSKEYLEN+1];
	char part_no[16];
	char description[31];
	char units[6];
	float price;
	struct timebuff purchase_dt1;
	char company1[51];
	char salesman1[31];
	float price1;
	struct timebuff purchase_dt2;
	char company2[51];
	char salesman2[31];
	float price2;
	struct timebuff update_dt[5];
	float update_fact[5];
};

struct inventory_key0_rec {
        char part_code[PARTKEYLEN];
};

 
struct filestruct inventory_file;
struct inventory_rec inventory_buff;
struct inventory_key0_rec inventory_listkey;

struct XABKEY invfrwd_key,invrev_key,invclass_key;

#define INV_PART_KEY 0
#define INV_FORWARD_KEY 0
#define INV_CLASS_KEY 1
#define INV_REVERSE_KEY 2

#ifdef INC_INITS
void InventoryFileInit(fn) char *fn;
{
   inventory_file.fab = cc$rms_fab;
   inventory_file.fab.fab$b_bks = 8;
   inventory_file.fab.fab$l_dna = DEFAULT_FILE_NAME;
   inventory_file.fab.fab$b_dns = sizeof DEFAULT_FILE_NAME -1;
   inventory_file.fab.fab$b_fac = FAB$M_DEL | FAB$M_GET |  FAB$M_PUT | FAB$M_UPD;
   inventory_file.fab.fab$l_fna = fn;
   inventory_file.fab.fab$b_fns = strlen(fn);
   inventory_file.fab.fab$l_fop = FAB$M_CIF;
   inventory_file.fab.fab$w_mrs = sizeof inventory_buff; /* RECORD_SIZE;*/
   inventory_file.fab.fab$b_org = FAB$C_IDX;
   inventory_file.fab.fab$b_rat = FAB$M_CR;
   inventory_file.fab.fab$b_rfm = FAB$C_FIX;
   inventory_file.fab.fab$b_shr = FAB$V_SHRDEL | FAB$V_SHRGET | FAB$V_SHRPUT | FAB$V_SHRUPD;  /*FAB$M_NIL;  */
   inventory_file.fab.fab$l_xab = &invfrwd_key;

   inventory_file.rab = cc$rms_rab;
   inventory_file.rab.rab$l_fab = &inventory_file.fab;
                           

   invfrwd_key = cc$rms_xabkey;
   
   invfrwd_key.xab$b_dtp = XAB$C_STG;
   invfrwd_key.xab$b_flg = 0;
   invfrwd_key.xab$w_pos0 = ((char *) &inventory_buff.part_code - (char *)&inventory_buff) ;
   invfrwd_key.xab$b_siz0 = PARTKEYLEN;
   invfrwd_key.xab$b_ref = INV_PART_KEY;
   invfrwd_key.xab$l_nxt = &invclass_key;


   invclass_key = cc$rms_xabkey;
   
   invclass_key.xab$b_dtp = XAB$C_STG;
   invclass_key.xab$b_flg = XAB$M_CHG|XAB$M_DUP| XAB$M_NUL;
   invclass_key.xab$b_nul = ' ';
   invclass_key.xab$w_pos0 = ((char *) &inventory_buff.class - (char *)&inventory_buff) ;
   invclass_key.xab$b_siz0 = CLASSKEYLEN;
   invclass_key.xab$b_ref = INV_CLASS_KEY;
   invclass_key.xab$l_nxt = &invrev_key;


   invrev_key = cc$rms_xabkey;
   
   invrev_key.xab$b_dtp = XAB$C_DSTG;
   invrev_key.xab$b_flg = 0;  /*XAB$M_CHG;  */
   invrev_key.xab$w_pos0 = ((char *) &inventory_buff.part_code - (char *)&inventory_buff) ;
   invrev_key.xab$b_siz0 = PARTKEYLEN;
   invrev_key.xab$b_ref = INV_REVERSE_KEY; 

}
#endif

struct kit_rec {
        char kit_code[PARTKEYLEN+1];
        char description[31];
        float price_offset;
        int16 reclen;
};


struct kit_part_rec {
   char part_code[PARTKEYLEN+1];
   char description[31];
   char units[6];
   float number;
   char ext_description[25];
};

struct kit_key0_rec {
        char kit_code[PARTKEYLEN];
};


struct filestruct kit_file;
struct kit_rec kit_buff;
struct kit_key0_rec kit_listkey;
char *kit_diskio_buff;

struct XABKEY kitfrwd_key,kitrev_key;

#define KIT_PART_KEY 0
#define KIT_FORWARD_KEY 0
#define KIT_REVERSE_KEY 1

#ifdef INC_INITS
void KitFileInit(fn) char *fn;
{
   kit_file.fab = cc$rms_fab;
   kit_file.fab.fab$b_bks = 4;
   kit_file.fab.fab$l_dna = DEFAULT_FILE_NAME;
   kit_file.fab.fab$b_dns = sizeof DEFAULT_FILE_NAME -1;
   kit_file.fab.fab$b_fac = FAB$M_DEL | FAB$M_GET |  FAB$M_PUT | FAB$M_UPD;
   kit_file.fab.fab$l_fna = fn;
   kit_file.fab.fab$b_fns = strlen(fn);
   kit_file.fab.fab$l_fop = FAB$M_CIF;
   kit_file.fab.fab$w_mrs = 0;  /*RECORD_SIZE; */
   kit_file.fab.fab$b_org = FAB$C_IDX;
   kit_file.fab.fab$b_rat = FAB$M_CR;
   kit_file.fab.fab$b_rfm = FAB$C_VAR;  /*FAB$C_FIX; */
   kit_file.fab.fab$b_shr = FAB$V_SHRDEL | FAB$V_SHRGET | FAB$V_SHRPUT | FAB$V_SHRUPD;  /*FAB$M_NIL;  */
   kit_file.fab.fab$l_xab = &kitfrwd_key;

   kit_file.rab = cc$rms_rab;
   kit_file.rab.rab$l_fab = &kit_file.fab;

                      
   kitfrwd_key = cc$rms_xabkey;
   
   kitfrwd_key.xab$b_dtp = XAB$C_STG;
   kitfrwd_key.xab$b_flg = 0; /* XAB$M_CHG; */
   kitfrwd_key.xab$w_pos0 = ((char *) &kit_buff.kit_code - (char *)&kit_buff) ;
   kitfrwd_key.xab$b_siz0 = PARTKEYLEN;
   kitfrwd_key.xab$b_ref = KIT_PART_KEY;
   kitfrwd_key.xab$l_nxt = &kitrev_key;

   kitrev_key = cc$rms_xabkey;
   
   kitrev_key.xab$b_dtp = XAB$C_DSTG;
   kitrev_key.xab$b_flg = 0; /*XAB$M_CHG;     */
   kitrev_key.xab$w_pos0 = ((char *) &kit_buff.kit_code - (char *)&kit_buff) ;
   kitrev_key.xab$b_siz0 = PARTKEYLEN;
   kitrev_key.xab$b_ref = KIT_REVERSE_KEY;


}
#endif

struct master_rec {
   float retail_markup;
   float wholesale_markup;
   float sales_tax[5];
   uint16 cust_no;
   uint16 quote_no;
   uint16 job_no;
   uint16 invoice_no;
   uint16 receipt_no;
   char d_flag_w;
   char d_flag_r;
   char d_flag_c;
   int labels_pl;
   int labels_wdth;
   int labels_lnth;
   int invoice_lnth;
   int receipt_lnth;
   struct timebuff statement_date;
   char email_subject[61];
   char email_address[61];
   char address1[31];
   char address2[31];
   char address3[31];
   char address4[31];
   char address5[31];
   char print_queue[31];
};

struct filestruct master_file;
struct master_rec master_buff;

#ifdef INC_INITS                           
void MasterFileInit(fn) char *fn;
{
   master_file.fab = cc$rms_fab;
   master_file.fab.fab$b_bks = 4;
   master_file.fab.fab$l_dna = DEFAULT_FILE_NAME;
   master_file.fab.fab$b_dns = sizeof DEFAULT_FILE_NAME -1;
   master_file.fab.fab$b_fac = FAB$M_DEL | FAB$M_GET |  FAB$M_PUT | FAB$M_UPD;
   master_file.fab.fab$l_fna = fn;
   master_file.fab.fab$b_fns = strlen(fn);
   master_file.fab.fab$l_fop = FAB$M_CIF;
   master_file.fab.fab$w_mrs = sizeof master_buff; /* RECORD_SIZE;*/
   master_file.fab.fab$b_org = FAB$C_REL; /* C_IDX; */
   master_file.fab.fab$b_rat = FAB$M_CR;
   master_file.fab.fab$b_rfm = FAB$C_FIX;
   master_file.fab.fab$b_shr = FAB$V_SHRDEL | FAB$V_SHRGET | FAB$V_SHRPUT | FAB$V_SHRUPD; /*FAB$M_NIL;  */

   master_file.rab = cc$rms_rab;
   master_file.rab.rab$l_fab = &master_file.fab;
}                           
#endif




struct quote_rec {
        uint16 cust_no;
        char ascdate[SIZE_UID+1];
        uint16 quote_no;
        uint16 job_no;
        uint16 invoice_no;
        struct timebuff date;
        char cust_code[7];
        char class[SIZE_QUOTECLASS+1];
        char quote_for[31];
        float length;
        float discount;
        char comment1[70];
        char comment2[80];
        char comment3[80];
        struct timebuff quote_date;
        struct timebuff job_date;
        struct timebuff invoice_date;
        char order_num[11];
        char type;
        char tax_code;
        char tax_num[11];
    uint16 reclen;
};                                                

struct quote_part_rec {
        char kit_part_code[PARTKEYLEN];
        char description[31];
        char units[6];
    float number;
        float cost_price;
        float wholesale;
        float retail;
    int kit;
        char tax_code;
};

struct quote_key0_rec {
        uint16 cust_no;
        char ascdate[SIZE_UID];
};


struct filestruct quote_file;
struct quote_rec quote_buff;
struct quote_key0_rec quote_listkey;
char *quote_diskio_buff;

struct XABKEY quotefrwd_key,quoterev_key,quotedate_key;
struct XABKEY quotecust_key,quotequo_key,quotejob_key,quoteinv_key;

#define INVC_DATE_KEY 0
#define INVC_CUST_KEY 1
#define INVC_CUSTDATE_KEY 2
#define INVC_FORWARD_KEY 2
#define INVC_REVERSE_KEY 3
#define INVC_QUOTE_KEY 4
#define INVC_INVC_KEY 5
#define INVC_JOB_KEY 6
#define INVC_CLASS_KEY 7

#ifdef INC_INITS
void QuoteFileInit(fn) char *fn;
{
   quote_file.fab = cc$rms_fab;
   quote_file.fab.fab$b_bks = 16;
   quote_file.fab.fab$l_dna = DEFAULT_FILE_NAME;
   quote_file.fab.fab$b_dns = sizeof DEFAULT_FILE_NAME -1;
   quote_file.fab.fab$b_fac = FAB$M_DEL | FAB$M_GET |  FAB$M_PUT | FAB$M_UPD;
   quote_file.fab.fab$l_fna = fn;
   quote_file.fab.fab$b_fns = strlen(fn);
   quote_file.fab.fab$l_fop = FAB$M_CIF;
   quote_file.fab.fab$w_mrs = 0;  /*RECORD_SIZE; */
   quote_file.fab.fab$b_org = FAB$C_IDX;
   quote_file.fab.fab$b_rat = FAB$M_CR;
   quote_file.fab.fab$b_rfm = FAB$C_VAR;  /*FAB$C_FIX; */
   quote_file.fab.fab$b_shr = FAB$V_SHRDEL | FAB$V_SHRGET | FAB$V_SHRPUT | FAB$V_SHRUPD;  /*FAB$M_NIL;  */
   quote_file.fab.fab$l_xab = &quotedate_key;

   quote_file.rab = cc$rms_rab;
   quote_file.rab.rab$l_fab = &quote_file.fab;


   quotedate_key = cc$rms_xabkey;
   
   quotedate_key.xab$b_dtp = XAB$C_BN8;
   quotedate_key.xab$b_flg = XAB$M_DUP;
   quotedate_key.xab$w_pos0 = ((char *) &quote_buff.date - (char *)&quote_buff) ;
   quotedate_key.xab$b_siz0 = SIZE_BN8;
   quotedate_key.xab$b_ref = INVC_DATE_KEY;
   quotedate_key.xab$l_nxt = &quotecust_key;

   quotecust_key = cc$rms_xabkey;

   quotecust_key.xab$b_dtp = XAB$C_BN2;
   quotecust_key.xab$b_flg = XAB$M_DUP;
   quotecust_key.xab$w_pos0 = ((char *) &quote_buff.cust_no - (char *)&quote_buff) ;
   quotecust_key.xab$b_siz0 = SIZE_BN2;
   quotecust_key.xab$b_ref = INVC_CUST_KEY;
   quotecust_key.xab$l_nxt = &quotefrwd_key;


   quotefrwd_key = cc$rms_xabkey;
   
   quotefrwd_key.xab$b_dtp = XAB$C_STG;
   quotefrwd_key.xab$b_flg = 0; /* XAB$M_DUP;  */
   quotefrwd_key.xab$w_pos0 = ((char *) &quote_buff.cust_no - (char *)&quote_buff) ;
   quotefrwd_key.xab$b_siz0 = SIZE_JOIC;
   quotefrwd_key.xab$w_pos1 = ((char *) &quote_buff.ascdate - (char *) &quote_buff) ; 
   quotefrwd_key.xab$b_siz1 = SIZE_UID;
   quotefrwd_key.xab$b_ref = INVC_CUSTDATE_KEY;
   quotefrwd_key.xab$l_nxt = &quoterev_key;

   quoterev_key = cc$rms_xabkey;
   
   quoterev_key.xab$b_dtp = XAB$C_DSTG;
   quoterev_key.xab$b_flg = 0; /*XAB$M_DUP; */
   quoterev_key.xab$w_pos0 = ((char *) &quote_buff.cust_no - (char *)&quote_buff) ;
   quoterev_key.xab$b_siz0 = SIZE_JOIC;
   quoterev_key.xab$w_pos1 = ((char *) &quote_buff.ascdate - (char *) &quote_buff) ; 
   quoterev_key.xab$b_siz1 = SIZE_UID;
   quoterev_key.xab$b_ref = INVC_REVERSE_KEY;
   quoterev_key.xab$l_nxt = &quotequo_key; 

   quotequo_key = cc$rms_xabkey;

   quotequo_key.xab$b_dtp = XAB$C_BN2;
   quotequo_key.xab$b_flg = XAB$M_NUL;
   quotequo_key.xab$w_pos0 = ((char *) &quote_buff.quote_no - (char *)&quote_buff) ;
   quotequo_key.xab$b_siz0 = SIZE_JOIC;
   quotequo_key.xab$b_ref = INVC_QUOTE_KEY;
   quotequo_key.xab$l_nxt = &quoteinv_key;

   quoteinv_key = cc$rms_xabkey;

   quoteinv_key.xab$b_dtp = XAB$C_BN2;
   quoteinv_key.xab$b_flg = XAB$M_CHG| XAB$M_NUL;
   quoteinv_key.xab$w_pos0 = ((char *) &quote_buff.invoice_no - (char *)&quote_buff) ;
   quoteinv_key.xab$b_siz0 = SIZE_JOIC;
   quoteinv_key.xab$b_ref = INVC_INVC_KEY;
   quoteinv_key.xab$l_nxt = &quotejob_key;

   quotejob_key = cc$rms_xabkey;

   quotejob_key.xab$b_dtp = XAB$C_BN2;
   quotejob_key.xab$b_flg = XAB$M_DUP|XAB$M_CHG| XAB$M_NUL;
   quotejob_key.xab$w_pos0 = ((char *) &quote_buff.job_no - (char *)&quote_buff) ;
   quotejob_key.xab$b_siz0 = SIZE_JOIC;
   quotejob_key.xab$b_ref = INVC_JOB_KEY;
   quotejob_key.xab$l_nxt = &quoteclass_key;

   quoteclass_key = cc$rms_xabkey;
   
   quoteclass_key.xab$b_dtp = XAB$C_STG;
   quoteclass_key.xab$b_flg = XAB$M_CHG|XAB$M_DUP| XAB$M_NUL;
   quoteclass_key.xab$b_nul = ' ';
   quoteclass_key.xab$w_pos0 = ((char *) &quote_buff.class - (char *)&quote_buff) ;
   quoteclass_key.xab$b_siz0 = SIZE_QUOTECLASS;
   quoteclass_key.xab$b_ref = INVC_CLASS_KEY;


}
#endif


struct transaction_rec {
	struct timebuff date;
        uint32 cust_no;
        int debit_flag;
        int wholesale_flag;
        uint32 invc_no;
        uint32 rcpt_no;
	float amnt;
};

 
struct filestruct transaction_file;
struct transaction_rec transaction_buff;

struct XABKEY transdate_key,transcust_key,transinvc_key,transrcpt_key;

#define TRANS_DATE_KEY 0
#define TRANS_CUST_KEY 1
#define TRANS_RCPT_KEY 2
#define TRANS_INVC_KEY 3

#ifdef INC_INITS
void TransactionFileInit(fn) char *fn;
{
   transaction_file.fab = cc$rms_fab;
   transaction_file.fab.fab$b_bks = 8;
   transaction_file.fab.fab$l_dna = DEFAULT_FILE_NAME;
   transaction_file.fab.fab$b_dns = sizeof DEFAULT_FILE_NAME -1;
   transaction_file.fab.fab$b_fac = FAB$M_DEL | FAB$M_GET |  FAB$M_PUT | FAB$M_UPD;
   transaction_file.fab.fab$l_fna = fn;
   transaction_file.fab.fab$b_fns = strlen(fn);
   transaction_file.fab.fab$l_fop = FAB$M_CIF;
   transaction_file.fab.fab$w_mrs = sizeof transaction_buff; /* RECORD_SIZE;*/
   transaction_file.fab.fab$b_org = FAB$C_IDX;
   transaction_file.fab.fab$b_rat = FAB$M_CR;
   transaction_file.fab.fab$b_rfm = FAB$C_FIX;
   transaction_file.fab.fab$b_shr = FAB$V_SHRDEL | FAB$V_SHRGET | FAB$V_SHRPUT | FAB$V_SHRUPD;  /*FAB$M_NIL;  */
   transaction_file.fab.fab$l_xab = &transdate_key;

   transaction_file.rab = cc$rms_rab;
   transaction_file.rab.rab$l_fab = &transaction_file.fab;
                           

   transdate_key = cc$rms_xabkey;
   
   transdate_key.xab$b_dtp = XAB$C_BN8;
   transdate_key.xab$b_flg = 0;
   transdate_key.xab$w_pos0 = ((char *) &transaction_buff.date - (char *)&transaction_buff) ;
   transdate_key.xab$b_siz0 = SIZE_BN8;
   transdate_key.xab$b_ref = TRANS_DATE_KEY;
   transdate_key.xab$l_nxt = &transcust_key;


   transcust_key = cc$rms_xabkey;
   
   transcust_key.xab$b_dtp = XAB$C_BN4;
   transcust_key.xab$b_flg = XAB$M_DUP;
   transcust_key.xab$w_pos0 = ((char *) &transaction_buff.cust_no - (char *)&transaction_buff) ;
   transcust_key.xab$b_siz0 = SIZE_BN4;
   transcust_key.xab$b_ref = TRANS_CUST_KEY;
   transcust_key.xab$l_nxt = &transrcpt_key;

   transrcpt_key = cc$rms_xabkey;
   
   transrcpt_key.xab$b_dtp = XAB$C_BN4;
   transrcpt_key.xab$b_flg =  XAB$M_NUL;
   transrcpt_key.xab$w_pos0 = ((char *) &transaction_buff.rcpt_no - (char *)&transaction_buff) ;
   transrcpt_key.xab$b_siz0 = SIZE_BN4;
   transrcpt_key.xab$b_ref = TRANS_RCPT_KEY;
   transrcpt_key.xab$l_nxt = &transinvc_key;

   transinvc_key = cc$rms_xabkey;
   
   transinvc_key.xab$b_dtp = XAB$C_BN4;
   transinvc_key.xab$b_flg =  XAB$M_NUL;
   transinvc_key.xab$w_pos0 = ((char *) &transaction_buff.invc_no - (char *)&transaction_buff) ;
   transinvc_key.xab$b_siz0 = SIZE_BN4;
   transinvc_key.xab$b_ref = TRANS_INVC_KEY;

}
#endif

