# fuzzy-find: fuzzy completion for finding files #

`ff` searches a directory tree with basic [fuzzy-completion][fc]. I wrote it
because `find -name "blah"` only scans filename (not their parent 
directories), and regular expressions for fuzzy completion are
cumbersome.

[fc]: http://common-lisp.net/project/slime/doc/html/Fuzzy-Completion.html

Searching for "aeiou" will print any paths that match the RE
`.*a.*e.*i.*o.*u.*`. 

By default, `ff` searches recursively from the current directory, but its
search root can be set with the `-r` option.

`ff` query strings are not regular expressions - characters such as
'.' and '-' match literally. Any sections enclosed in '/'s
will be required to match within the same path element, and the
consecutive match character (default '=') toggles exact matching.
(I chose '=' because it doesn't mean anything in basic REs and it's
unshifted on most keyboards.)

See also: [compound-completion][cc]

[cc]: http://common-lisp.net/project/slime/doc/html/Compound-Completion.html


## Installation ##

Just run make. I mean, it's one C file. The makefile is just `ff: ff.c`.
Copy ff somewhere in your path.


## Example ##

`ff aeiou` matches both `~/and/the/first/one/used.txt`
and `~/after_the_furious_ultimatum.txt`, because the characters 'a', 'e',
'i', 'o', and 'u' appear sequentially.

`ff a/e/i/o/u` only matches `~/and/the/first/one/used.txt`, since the `/`s force
each vowel to appear in its own directory element.

`ff ae=iou=` would only match `~/after_the_furious_ultimatum.txt`, since
it matches an 'a', then an 'e', then the `=`s specify a *consecutive* "iou" string.


## Usage ##

    ff [-dilt] [-c char] [-r root] query
    -c CHAR   char to toggle Consecutive match (default: '=')
    -d        show Dotfiles
    -h        print this Help
    -i        case-Insensitive search
    -l        follow Links (warning: no cycle detection)
    -t        run Tests and exit
    -r ROOT   set search Root (default: .)
