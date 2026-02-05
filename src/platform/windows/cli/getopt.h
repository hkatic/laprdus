/*
 * getopt.h - POSIX getopt() and getopt_long() for Windows
 *
 * This is a public domain implementation of getopt/getopt_long for systems
 * that lack them (primarily Windows). Based on various public domain
 * implementations.
 *
 * Public Domain - No copyright claimed
 */

#ifndef GETOPT_H
#define GETOPT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Standard getopt variables */
extern char *optarg;
extern int optind;
extern int opterr;
extern int optopt;

/* Argument requirement constants */
#define no_argument       0
#define required_argument 1
#define optional_argument 2

/* Long option structure */
struct option {
    const char *name;    /* Name of the option */
    int has_arg;         /* no_argument, required_argument, or optional_argument */
    int *flag;           /* If non-NULL, set *flag to val when option found */
    int val;             /* Value to return or set in *flag */
};

/**
 * Parse command-line options (POSIX getopt)
 *
 * @param argc Argument count
 * @param argv Argument vector
 * @param optstring String of valid option characters
 * @return Option character, '?' for unknown option, ':' for missing argument, -1 when done
 */
int getopt(int argc, char * const argv[], const char *optstring);

/**
 * Parse command-line options with long option support (GNU getopt_long)
 *
 * @param argc Argument count
 * @param argv Argument vector
 * @param optstring String of valid option characters
 * @param longopts Array of struct option defining long options
 * @param longindex If non-NULL, receives index of matched long option
 * @return Option character, '?' for unknown option, ':' for missing argument, -1 when done
 */
int getopt_long(int argc, char * const argv[], const char *optstring,
                const struct option *longopts, int *longindex);

/**
 * Reset getopt state for re-parsing
 */
void getopt_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* GETOPT_H */
