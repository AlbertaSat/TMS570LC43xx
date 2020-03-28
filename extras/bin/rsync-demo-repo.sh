# Use this shell script to rsync this repository with the local copy
# of the github dsmux-debugging-example.

export src=~/workspace-hjh-win10/TMS570LC43xx
export dst=~/Working/AlbertaSat/github-TMS570LC43xx/TMS570LC43xx

# for rsync live run
export opts=-avlz
# uncomment for rsync dry run
#export opts=-avzn

do_sync () {
    dir=$1
    dest=$dst/$dir
    source=$src/$dir
    echo
    echo Command: rsync $opts --delete $source/ $dest
    echo
    read -p "Proceed to update $dir from $src?" 
    [[ $REPLY =~ ^[Yy]$ ]] || (echo Aborted; false) || return 1
    rsync $opts --delete $source/ $dest
    # chown -R hoover:staff $dest
    # chmod -R g-w $dest
    # chmod -R o-rwx $dest
    # echo '***' done '***'
    echo
    return 0
}

cd $src
echo Working in `pwd`

# 2019-06-05 HJH get from same tree as other code
do_sync "base_r5.dil"
do_sync "base_r5.hcg"
do_sync "extras"
do_sync "include"
do_sync "source"
echo '***' All Done '***'
