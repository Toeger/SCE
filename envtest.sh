export PRETEST="$PRETEST pretest"
find . -name "lib*" | while read lib
do
    export LIB_LIST="$LIB_LIST $(realpath $lib)"
    echo $(realpath $lib)
    echo $LIB_LIST
    echo "$LIB_LIST $(realpath $lib)"
done

