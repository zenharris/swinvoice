
    unsigned char tid[16];
    struct iosb {
        unsigned short status, mbz;
        unsigned long  reason;
    } iosb;

#define check_status(syscall) \
    if (status != SS$_NORMAL || (status = iosb.status) != SS$_NORMAL) { \
        fprintf (stderr, "Failed to %s the transaction\n", syscall); \
        fprintf (stderr, "Error was: %s", strerror (EVMSERR, status)); \
        exit (EXIT_FAILURE); \
    }

#define event_flag 0

