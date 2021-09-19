#!/bin/sh
#set -x

# Work directory of this repository.
if [ "$1" != "" ]; then
	workdir=$1
else
	workdir=.
fi

cd $workdir
if [ ! -f appveyor.yml ]; then
	echo "Wrong directory."
	exit 1
fi

git checkout master
git pull --no-edit

if [ ! -d ag/src ]; then
	git submodule init
fi
git submodule update

# Get the latest ag source code
cd ag
git checkout master
git pull --no-edit
agver=$(git describe --tags --always)
cd ..
aglog=$(git submodule summary | grep '^  > ')

# Check if it is updated
if git diff --quiet; then
	echo "No changes found."
	exit 0
fi

# Commit the change and push it
# replace newline by \n
echo "$aglog" | \
	sed -e 's/\([][_*^<`\\]\)/\\\1/g' \
	    -e 's/^  >/*/' \
	    -e 's!#\([0-9][0-9]*\)![#\1](https://github.com/ggreer/the_silver_searcher/issues/\1)!' | \
	perl -pe 's/\n/\\n/g' > gitlog.txt
aglog=$(echo "$aglog" | sed -e 's/^  >/*/')
git commit -a -m "ag: Update to $agver" -m "$aglog"
git tag $(date --rfc-3339=date)/$agver
git push origin master --tags
