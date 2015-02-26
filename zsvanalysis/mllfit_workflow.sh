#!/bin/bash

function exec_checked() {
    $* || { echo "Error executing $*"; exit 1; }
}


exec_checked python create-mllfit-rootfile.py

# ee:
exec_checked sed -i 's/^channel.*$/channel="ee"/' mllfit_include.py
exec_checked ../../theta/utils2/theta-auto.py mllfit.py

# mm:
exec_checked sed -i 's/^channel.*$/channel="mm"/' mllfit_include.py
exec_checked ../../theta/utils2/theta-auto.py mllfit.py

# plot all:
workdir=`cat .mllfit_workdir`
mkdir -p ${workdir}/plots
exec_checked ./plot_zpurity_fits

