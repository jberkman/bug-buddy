#!/bin/sh

xgettext --default-domain=bug-buddy --directory=.. \
  --add-comments --keyword=_ --keyword=N_ \
  --files-from=./POTFILES.in \
&& test ! -f bug-buddy.po \
   || ( rm -f ./bug-buddy.pot \
    && mv bug-buddy.po ./bug-buddy.pot )
