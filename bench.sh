if [ -z $1 ] || [ -z $2 ]; then
  echo "usage: bench MODE FOLDER"
  return
fi

mode=$1
folder=$2

for f in $(ls $folder); do
  fullf=$folder$f
  dom=$fullf/task-domain.maepl
  rm -rf $fullf/result.txt
  for p in $fullf/*-problem-*.maepl; do
    echo "$f $p"
    /usr/bin/time -v ./planner $mode individual none $dom $p >> $fullf/result.txt 2>&1
  done
done
