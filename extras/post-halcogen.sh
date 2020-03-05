export project='.'
find $project/{source,include} -type f | xargs chmod a-x
find $project/{source,include} -type f | xargs dos2unix
