#include <string.h>
#include <sys/utsname.h>

static char *zagtos = "Zagtos";
static char *current = "current";
static char *localhost = "localhost";
static char *computer = "Computer";

int uname(struct utsname *uts)
{
    strcpy(uts->sysname, zagtos);
    strcpy(uts->nodename, localhost);
    strcpy(uts->release, current);
    strcpy(uts->version, current);
    strcpy(uts->machine, computer);
    return 0;
}
