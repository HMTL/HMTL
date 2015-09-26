#!/bin/bash -xu

exec 2>&1

ARDUINO_LIB_DIR=${1:-"/Users/amp/Dropbox/Arduino/libraries"}

LIBRARY_DIR=${2:-"Libraries"}

BACKUP_DIR="${ARDUINO_LIB_DIR}/../libraries.backup.$$"
mkdir -p $BACKUP_DIR
LOG_FILE=$BACKUP_DIR/log

log() {
    msg=$1
    echo "$msg"
    echo "$msg" >> $LOG_FILE
}

log "*** Running setup: $LIBRARY_DIR > $ARDUINO_LIB_DIR"
log "*** Moving libraries from $LIBRARY_DIR to $ARDUINO_LIB_DIR"
log "*** Existing libraries will be backed up in $BACKUP_DIR"

pushd $LIBRARY_DIR
DIR=`pwd`

for lib in `find . -type d -depth 1`; do
    base=`basename $lib`

    cmd="mv $ARDUINO_LIB_DIR/$base $BACKUP_DIR/"
    log "executing: $cmd"
    $cmd | tee -a $LOG_FILE

    cmd="ln -s $DIR/$base $ARDUINO_LIB_DIR/"
    log "executing: $cmd"
    $cmd | tee -a $LOG_FILE
done

popd
