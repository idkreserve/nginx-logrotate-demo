#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#define PATTERN "access.log"
#define MAXLINE 100
#define MAXPATH 1024
#define MAXFILENAME 255

#ifndef LOGDIR
#define LOGDIR "log"
#endif

#ifndef OUTDIR
#define OUTDIR "parsed"
#endif

#ifndef OUTFILE
#define OUTFILE "lines.txt"
#endif

#ifndef MAXFILE_B
#define MAXFILE_B 30
#endif

#ifndef MASK
#define MASK "favicon.ico"
#endif

static size_t nrotate = 1;
static char *logdir = LOGDIR;
static char *outdir = OUTDIR;
static char *outfile = OUTFILE;
static long maxfile = MAXFILE_B;
static char *mask = MASK;

extern char *optarg;
extern int opterr;

static inline int fpattern(const char *);
static inline long filesize(const char *);
static inline int rotate(const char *s);
static inline long fsize(FILE *fp);

int main(int argc, char *argv[])
{
    int opt;
    char line[MAXLINE], _dummy;
    char filename[MAXPATH];
    DIR *dir;
    FILE *logfile, *rotatefile;
    struct dirent *dirbuf;

    while ((opt = getopt(argc, argv, "l:o:f:s:m:")) != -1)
        switch (opt) {
            case 'l':
            if (strlen(optarg) >= 1)
                logdir = optarg;
            break;
            case 'o':
            if (strlen(optarg) >= 1)
                outdir = optarg;
            break;
            case 'f':
            if (strlen(optarg) >= 1)
                outfile = optarg;
            break;
            case 's':
            if (sscanf(optarg, "%lu%c", &maxfile, &_dummy) != 1)
                maxfile = MAXFILE_B; 
            break;
            case 'm':
            if (strlen(optarg) >= 1)
                mask = optarg;
            break;
            default:
            return opterr;
            break;
        }
    if ((dir = opendir(logdir)) == NULL) {
        fprintf(stderr, "Can't open logging directory %s\n", logdir);
        return 1;
    }

    sprintf(filename, "%s/%s", outdir, outfile);
    if ((rotatefile = fopen(filename, "w")) == NULL) {
        fprintf(stderr, "Failed to create rotation file %s\n", filename);
        return 1;
    }

    while ((dirbuf = readdir(dir)) != NULL) {
        sprintf(filename, "%s/%s", logdir, dirbuf->d_name);
        if (fpattern(filename) && (logfile = fopen(filename, "r")) != NULL) {
            while (fgets(line, sizeof line, logfile) != NULL) {
                sprintf(filename, "%s/%s", outdir, outfile);
                if (strstr(line, mask)) {
                    if (fsize(rotatefile) >= maxfile) {
                        if (!rotate(filename))
                            fprintf(stderr, "Failed to rotate %s\n", filename);
                        fclose(rotatefile);
                        rotatefile = NULL;
                    }
                    if (rotatefile == NULL && (rotatefile = fopen(filename, "w")) == NULL)
                        return 1;
                    if (fputs(line, rotatefile) == EOF)
                        fprintf(stderr, "Failed to write \"%s\" in rotation file\n", line);
                }
            }
            fclose(logfile);
        } 
    }
    return 0;
}

static inline int rotate(const char *name)
{
    int c;
    char rotatefile_i_name[MAXFILENAME];
    FILE *rotatefile, *rotatefile_i;

    sprintf(rotatefile_i_name, "%s.%lu", name, nrotate++);
    if ((rotatefile = fopen(name, "r")) == NULL ||
            (rotatefile_i = fopen(rotatefile_i_name, "w")) == NULL)
        return 0;
    while ((c = getc(rotatefile)) != EOF)
        putc(c, rotatefile_i);
    fclose(rotatefile);
    fclose(rotatefile_i);
    return 1;
}

static inline long fsize(FILE *fp)
{
    long cur, res;
    
    if (fp == NULL)
        return 0;
    cur = ftell(fp);
    fseek(fp, 0, SEEK_END);
    res = ftell(fp);
    fseek(fp, cur, SEEK_SET);
    return res;
}

static inline int fpattern(const char *name)
{
    struct stat stbuf;

    /* TODO: сделать в одну строку */
    if (stat(name, &stbuf) == -1 || S_ISREG(stbuf.st_mode) == 0)
        return 0;
    return 1;
}

static inline long filesize(const char *name)
{
    struct stat stbuf;

    if (stat(name, &stbuf) == -1)
        return -1;
    return stbuf.st_size;
}
