#define MESSAGE_H
#define HSTSIZ   57
#define TPCSIZ   51

struct message {
    long msgno;
    char userto[UIDSIZ];
    char from[UIDSIZ];
    char to[UIDSIZ];
    char topic[TPCSIZ];
    char auxtpc[HSTSIZ];
    unsigned crdate;
    unsigned crtime;
    int nreply;
    int flags;
    char spare[5];
    char text[1];
};
