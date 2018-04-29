export PRETEST="$PRETEST pretest"
find . -name "lib*" | while read lib
do
    export LIB_LIST="$LIB_LIST $(realpath $lib)"
    export MIDTEST="$MIDTEST midtest"
    echo $LIB_LIST
done
export POSTTEST="$POSTTEST posttest"
