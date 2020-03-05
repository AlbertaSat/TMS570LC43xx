do_sync () {
    base=$1
    dir=$2
    dest=iprsm/$dir
    source=$mirror/$base/$dir
    echo
    echo Command: rsync $opts --delete $source/ $dest
    echo
    read -p "Proceed to update $dir from $base?" 
    [[ $REPLY =~ ^[Yy]$ ]] || (echo Aborted; false) || return 1
    rsync $opts --delete $source/ $dest
    chown -R hoover:staff $dest
    chmod -R g-w $dest
    chmod -R o-rwx $dest
    echo '***' done '***'
    echo
    return 0
}
