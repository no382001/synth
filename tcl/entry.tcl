package require logger 0.3

set log [logger::init tcl]
set signal_data {}

source tcl/networking.tcl
source tcl/gui.tcl

networking::connect