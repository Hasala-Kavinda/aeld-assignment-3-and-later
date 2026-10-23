#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>

int main(int argc, char *argv[])
{
    openlog(NULL, 0, LOG_USER);

    if (argc != 3) {
        syslog(LOG_ERR, "Invalid number of arguments: %d (expected 2)", argc - 1);
        fprintf(stderr, "Usage: %s <writefile> <writestr>\n", argv[0]);
        closelog();
        return 1;
    }

    const char *writefile = argv[1];
    const char *writestr  = argv[2];

    syslog(LOG_DEBUG, "Writing %s to %s", writestr, writefile);

    FILE *fp = fopen(writefile, "w");
    if (!fp) {
        syslog(LOG_ERR, "Error opening file %s: %s", writefile, strerror(errno));
        fprintf(stderr, "Error opening file %s: %s\n", writefile, strerror(errno));
        closelog();
        return 1;
    }

    if (fprintf(fp, "%s", writestr) < 0) {
        syslog(LOG_ERR, "Error writing to file %s: %s", writefile, strerror(errno));
        fprintf(stderr, "Error writing to file %s: %s\n", writefile, strerror(errno));
        fclose(fp);
        closelog();
        return 1;
    }

    if (fclose(fp) != 0) {
        syslog(LOG_ERR, "Error closing file %s: %s", writefile, strerror(errno));
        fprintf(stderr, "Error closing file %s: %s\n", writefile, strerror(errno));
        closelog();
        return 1;
    }

    closelog();
    return 0;
}
