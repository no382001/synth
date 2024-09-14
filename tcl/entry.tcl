package require logger 0.3
set log [logger::init tcl]

source tcl/networking.tcl
source tcl/gui.tcl

networking::connect

draw_adsr
update_waveform