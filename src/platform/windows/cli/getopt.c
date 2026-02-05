/*
 * getopt.c - POSIX getopt() and getopt_long() for Windows
 *
 * This is a public domain implementation of getopt/getopt_long for systems
 * that lack them (primarily Windows). Based on various public domain
 * implementations.
 *
 * Public Domain - No copyright claimed
 */

#include "getopt.h"
#include <string.h>
#include <stdio.h>

/* Global variables */
char *optarg = NULL;
int optind = 1;
int opterr = 1;
int optopt = '?';

/* Internal state */
static char *nextchar = NULL;

/**
 * Reset getopt state
 */
void getopt_reset(void)
{
    optarg = NULL;
    optind = 1;
    opterr = 1;
    optopt = '?';
    nextchar = NULL;
}

/**
 * Parse short options
 */
int getopt(int argc, char * const argv[], const char *optstring)
{
    const char *oli;  /* Option letter index */

    if (optind == 0) {
        /* Reset state */
        optind = 1;
        nextchar = NULL;
    }

    optarg = NULL;

    if (nextchar == NULL || *nextchar == '\0') {
        /* Start processing new argument */
        if (optind >= argc) {
            return -1;
        }

        if (argv[optind][0] != '-' || argv[optind][1] == '\0') {
            /* Not an option */
            return -1;
        }

        if (argv[optind][1] == '-' && argv[optind][2] == '\0') {
            /* "--" marks end of options */
            optind++;
            return -1;
        }

        nextchar = &argv[optind][1];
    }

    /* Get option character */
    optopt = *nextchar++;

    /* Check if option is valid */
    oli = strchr(optstring, optopt);
    if (oli == NULL) {
        if (opterr) {
            fprintf(stderr, "%s: invalid option -- '%c'\n", argv[0], optopt);
        }
        if (*nextchar == '\0') {
            optind++;
        }
        return '?';
    }

    /* Check for argument */
    if (oli[1] == ':') {
        if (*nextchar != '\0') {
            /* Argument is attached to option */
            optarg = nextchar;
            nextchar = NULL;
            optind++;
        } else if (optind + 1 < argc) {
            /* Argument is next argv element */
            optind++;
            optarg = argv[optind];
            optind++;
        } else {
            /* Missing argument */
            if (opterr) {
                fprintf(stderr, "%s: option requires an argument -- '%c'\n", argv[0], optopt);
            }
            if (*nextchar == '\0') {
                optind++;
            }
            return (optstring[0] == ':') ? ':' : '?';
        }
    } else {
        if (*nextchar == '\0') {
            optind++;
        }
    }

    return optopt;
}

/**
 * Parse long options
 */
int getopt_long(int argc, char * const argv[], const char *optstring,
                const struct option *longopts, int *longindex)
{
    const char *arg;
    const struct option *opt;
    size_t arglen;
    int i;

    if (optind == 0) {
        optind = 1;
        nextchar = NULL;
    }

    optarg = NULL;

    /* Check if we're in the middle of parsing short options */
    if (nextchar != NULL && *nextchar != '\0') {
        return getopt(argc, argv, optstring);
    }

    if (optind >= argc) {
        return -1;
    }

    arg = argv[optind];

    /* Check for long option (--name) */
    if (arg[0] == '-' && arg[1] == '-') {
        if (arg[2] == '\0') {
            /* "--" marks end of options */
            optind++;
            return -1;
        }

        arg += 2;  /* Skip "--" */

        /* Check for "=" in argument */
        const char *eq = strchr(arg, '=');
        if (eq != NULL) {
            arglen = (size_t)(eq - arg);
        } else {
            arglen = strlen(arg);
        }

        /* Search for matching long option */
        for (i = 0; longopts[i].name != NULL; i++) {
            opt = &longopts[i];

            if (strncmp(arg, opt->name, arglen) == 0 && opt->name[arglen] == '\0') {
                /* Found exact match */
                optind++;

                if (longindex != NULL) {
                    *longindex = i;
                }

                /* Handle argument */
                if (opt->has_arg != no_argument) {
                    if (eq != NULL) {
                        /* Argument attached with "=" */
                        optarg = (char *)(eq + 1);
                    } else if (opt->has_arg == required_argument) {
                        if (optind < argc) {
                            optarg = argv[optind];
                            optind++;
                        } else {
                            if (opterr) {
                                fprintf(stderr, "%s: option '--%s' requires an argument\n",
                                        argv[0], opt->name);
                            }
                            return '?';
                        }
                    }
                }

                /* Handle flag/val */
                if (opt->flag != NULL) {
                    *opt->flag = opt->val;
                    return 0;
                } else {
                    return opt->val;
                }
            }
        }

        /* No match found */
        if (opterr) {
            fprintf(stderr, "%s: unrecognized option '--%.*s'\n", argv[0], (int)arglen, arg);
        }
        optind++;
        return '?';
    }

    /* Not a long option, try short option */
    if (arg[0] == '-' && arg[1] != '\0') {
        return getopt(argc, argv, optstring);
    }

    /* Not an option */
    return -1;
}
