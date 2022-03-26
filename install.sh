#!/bin/bash

if [[ $1 == '' ]]; then
  echo "Usage: $0 <install_location>"
  exit 1
fi

DEST_DIR=$1
mkdir -p $DEST_DIR
find bin/default/ -name *.so -exec \
  cp -vuni '{}' "$DEST_DIR" ";"

if [ ! -d $DEST_DIR/includes ]; then
  mkdir $DEST_DIR/includes
fi
cp -r bin/default/include/public/* $DEST_DIR/includes
