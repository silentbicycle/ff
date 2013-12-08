/* 
 * Copyright (c) 2012 Scott Vokes <vokes.s@gmail.com>
 *  
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *  
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/dir.h>
#include <ctype.h>
#include <dirent.h>
#include <err.h>
#include <errno.h>

#define FF_VERSION "0.6.0"

static void bail(char *msg) {
    fprintf(stderr, "%s", msg);
    exit(EXIT_FAILURE);
}

static int dotfiles = 0;        /* show dotfiles? */
static int only_dirs = 0;       /* only show directories */
static char conseq_char = '=';  /* consecutive match toggle char */
static int nocase = 0;          /* case insensitive? */
static int links = 0;           /* follow links? */
static int recurse = 1;         /* search file tree recursively? */

static char rootbuf[FILENAME_MAX];
static char pathbuf[FILENAME_MAX];

static char *query = NULL;
static int query_len = 0;

static void usage() {
    fprintf(stderr,
        "fuzzy-finder v. %s, by Scott Vokes <vokes.s@gmail.com>\n"
        "usage: ff [-dhiltR] [-c char] [-n count] [-r root] query\n"
        "-c CHAR   char to toggle Consecutive match (default: '=')\n"
        "-d        show Dotfiles\n"
        "-D        only show directories\n"
        "-h        print this Help\n"
        "-i        case-Insensitive search\n"
        "-l        follow Links\n"
        "-t        run Tests and exit\n"
        "-r ROOT   set search Root (default: .)\n"
        "-R        don't recurse subdirectories\n", FF_VERSION);
    exit(EXIT_FAILURE);
}

/* Append a name element to the path buffer. */
static uint put_path(uint offset, const char *elt, int dir) {
    uint sz = FILENAME_MAX - offset;
    uint res = snprintf(pathbuf + offset, sz, "%s%s",
        elt, dir ? "/" : "");
    if (FILENAME_MAX < res) {
        fprintf(stderr, "snprintf error\n");
        exit(EXIT_FAILURE);
    }

    /* If it ends with "//" then drop the second '/'. */
    if (offset + res >= 2 && pathbuf[offset + res - 2] == '/') {
        res--;
        pathbuf[offset + res] = '\0';
    }

    return res;
}

#define IS_DIR(d) (d->d_type == DT_DIR)
#define IS_LINK(d) (d->d_type == DT_LNK)

/* Try to sequentially match the next characters of the query against
 * a filename, returning the endpoint in the query. If the query
 * contains the consecutive-match toggle character (def. '='), then
 * the following characters (until another '=') need to be matched
 * as a conecutive group.
 * For example, "aeiou" matches "abefijopuv", but "a=eio=u" does not.*/
static uint match_chars(const char *name, uint nlen, uint qo) {
    uint i = 0;
    int exact = 0;
    char c = '\0';

    while (i < nlen) {
        if (query[qo] != conseq_char) { /* look for indiv. chars */
            c = nocase ? tolower(name[i]) : name[i];
            i++;
            if (query[qo] == c) {
                qo++;
                if (qo == query_len) break;
            }
        } else {                /* look for consecutive chars */
            int old_qo = qo;
    advance:
            qo++;
            if (qo == query_len) return qo; /* done */
            if (query[qo] == conseq_char) { /* done w/ consecutive match */
                qo++;
                continue;
            }
            c = nocase ? tolower(name[i]) : name[i];
            i++;
            if (c != query[qo]) {
                qo = old_qo;
                continue;
            }
            goto advance;
        }
    }
    return qo;
}

/* Incrementally match the query string against the file tree.
 * Sections of the query surrounded by '/'s must all match within
 * the same path element: "d/ex/" matches "dev/example/foo", but
 * not "dev/eta/text". */
static void walk(const char *path, uint po,
                 uint qo) {
    DIR *dir = opendir(path);
    struct dirent *dp = NULL;
    int i = 0;
    int npo = 0, nqo = 0;       /* new path and query offsets */
    int expects_dir = 0;
    int is_dir = 0;

    if (dir == NULL) {
        perror(path);
        errno = 0;
        return;
    }

    /* If the rest of the query has any '/'s, then the preceding portion
     * must be completely matched by the next directory name. */
    for (i=qo; i<query_len; i++) {
        if (query[i] == '/') { expects_dir = 1; break; }
    }

    while ((dp = readdir(dir)) != NULL) {
        char *name = dp->d_name;
        uint nlen = strlen(name);

        /* Skip empty names, dotfiles, and links. */
        if (nlen == 0) continue;
        if (name[0] == '.') {
            if (!dotfiles) continue;
            if (name[1] == '\0' || 0 == strcmp(name, "..")) continue;
        }
        if (!links && IS_LINK(dp)) continue;

        nqo = match_chars(name, nlen, qo);
        is_dir = IS_DIR(dp);
        npo = put_path(po, name, is_dir) + po;

        /* If this is a directory that doesn't completely match
         * to the next '/', skip, unless at start of the query. */
        if (expects_dir && nqo > 0 && query[nqo] != '/' && is_dir)
            continue;

        /* Check for trailing / in pattern. Do this *before*
         * printing the path, in case the pattern ends in '/'. */
        if (is_dir && query[nqo] == '/') nqo++;

        /* Print complete matches. */
        if (nqo == query_len) {
            if (!only_dirs || is_dir) {
              printf("%s\n", pathbuf);
            }
        }

        /* Walk subdirectories, checking from new query offset. */
        if (is_dir && recurse) walk(pathbuf, npo, nqo);
    }

    if (closedir(dir) == -1) {
        perror("Closedir failure");
        exit(EXIT_FAILURE);
    }
}


/**********************
 * Arguments and main *
 **********************/

static void run_tests();

static void set_root(const char *path) {
    if (path == NULL) bail("Bad root path\n");
    if (path[0] == '~') {       /* properly expand ~ */
        const char *home = getenv("HOME");
        if (home == NULL) bail("Failed to get $HOME\n");

        if (FILENAME_MAX < snprintf(rootbuf, FILENAME_MAX,
                                    "%s/%s", home, path + 1)) {
            bail("error: path longer than FILENAME_MAX\n");
        }
    } else {
        strncpy(rootbuf, path, FILENAME_MAX);
        rootbuf[FILENAME_MAX-1] = '\0';
    }
}

/* Process args, return root path (or NULL). */
static void proc_args(int argc, char **argv) {
    uint i = 0;
    int a = 0;

    while ((a = getopt(argc, argv, "c:dDhilr:tR")) != -1) {
        switch (a) {
        case 'c':               /* set consecutive match char */
            conseq_char = optarg[0]; break;
        case 'd':               /* show dotfiles */
            dotfiles = 1; break;
        case 'D':               /* only print directories */
            only_dirs = 1; break;
        case 'h':               /* help */
            usage(); break;
        case 'i':               /* case-insensitive */
            nocase = 1; break;
        case 'l':               /* follow links */
            links = 1; break;
        case 'r':               /* set search root */
            set_root(optarg);
            break;
        case 't':               /* run tests and exit */
            run_tests(); break;
        case 'R':
            recurse = 0; break;
        default:
            fprintf(stderr, "ff: illegal option: -- %c\n", a);
            usage();
        }
    }

    argc -= optind;
    argv += optind;
    if (argc < 1) usage();
    query = argv[0];
}

int main(int argc, char **argv) {
    char *root = rootbuf;

    proc_args(argc, argv);
    if (rootbuf[0] == '\0') {
        root = getcwd(rootbuf, FILENAME_MAX);
        if (root == NULL) bail("Could not get current working directory.\n");
    }

    if (query == NULL) bail("Bad query\n");

    query_len = strlen(query);

    if (nocase) {
        int i;
        for (i=0; i<query_len; i++) query[i] = tolower(query[i]);
    }

    setlinebuf(stdout);
    
    walk(root, put_path(0, root, 1), 0);
    return EXIT_SUCCESS;
}


/*********
 * Tests *
 *********/

static int tests_run = 0;

static int testcase(char *tquery, const char *path, int expected) {
    int res = 0;
    tests_run++;
    query = tquery;
    query_len = strlen(query);
    res = match_chars(path, strlen(path), 0);
    if (res != expected) {
        printf("\ntest %d -- query: \"%s\", path: \"%s\", expected %d, got %d\n",
            tests_run, tquery, path, expected, res);
        return 1;
    }
    printf(".");
    return 0;
}

static void run_tests() {
    int fail = 0;

#define T fail += testcase
    T("foo", "afbocod", 3);
    nocase = 1;
    T("foo", "aFbOcOd", 3);
    nocase = 0;
    T("=foo", "foo", 4);        /* leading = */
    T("=foo=a", "foobar", 6);
    T("=foo=a", "oobar", 0);    /* should stick at unmatched =foo= block */
    T("f=oob=r", "foobar", 7);
    T("f=oob=rx", "foobar", 7);
    T("=", "foo", 1);           /* arguably malformed */
    T("==", "foo", 2);
    T("f=", "foo", 2);          /* trailing =s */
    T("f==", "foo", 3);         /* trailing =s */
    T("==f", "foo", 3);
    T("z==", "foo", 0);
    T("aeiou", "abefijopuv", 5);
    T("a=eio=u", "abefijopuv", 1); /* stick at a, don't match =eio= */
    T("a=cdef=hj", "abcdefghijk", 9);
    T("a=cdef=hj", "abcefghijk", 1);
#undef T

    printf("%d tests, %d failed\n", tests_run, fail);
    exit(fail == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}
