export project=$1
[ ! -d "$project" ] && exit
echo Archiving $project
export version=`date +%C%y%m%d-%H%M`
export archive=$project-archive
export filename=$project-$version.tgz
rm $archive/$filename
tar cvzf $archive/$filename $project
ls -l $archive
