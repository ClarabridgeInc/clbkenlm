#! /bin/sh
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:build/libs/clbkenlm/shared

# TODO: check var availability first
export GRADLE_USER_HOME=~/.gradle

export GRA_CACHE_MODULES=$GRADLE_USER_HOME/caches/modules-2
export CUR_SUFFIX=clarabridge/libclbkenlm-linux

rm -rf $GRA_CACHE_MODULES/metadata-2.23/descriptors/$CUR_SUFFIX
rm -rf $GRA_CACHE_MODULES/files-2.1/$CUR_SUFFIX
