export project=TMS570LC43xx
find $project/{source,include} -type f | xargs dos2unix
