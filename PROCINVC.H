
enum funct_type { header, footer, cancel, ende };
enum trans_type {debit, credit };

uint32 invc_allocate();
uint32 rcpt_allocate();
uint16 quote_allocate();
float ballance (uint32);
int open_stdprn (char *,char *);


void process_invc (struct quote_rec, struct primary_rec);
void heading_invc (enum funct_type, struct quote_rec, struct primary_rec );

int write_transaction (enum trans_type, int, uint32, uint32, float);

int mkwsflag(char);
int delete_file(char *);
char *drcr(float);

