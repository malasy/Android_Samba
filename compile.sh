#!/bin/bash
CWD=$(pwd)

WAF_MAKE=1 python $CWD/buildtools/bin/waf build --targets=smbclient $*
