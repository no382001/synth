wm title . "main"
wm protocol . WM_DELETE_WINDOW {
    destroy .
    exit
}

canvas .c -width 600 -height 500 -bg white
pack .c -expand true -fill both

set canvas_width 600
set canvas_height 500
set outer_padding 20
set inner_padding 10

set middle_x [expr {($canvas_width - 2 * $outer_padding - $inner_padding) / 2 + $outer_padding}]
set middle_y [expr {($canvas_height - 2 * $outer_padding - $inner_padding) / 2 + $outer_padding}]

.c create line [expr {$middle_x + $inner_padding / 2}] $outer_padding [expr {$middle_x + $inner_padding / 2}] [expr {$middle_y - $inner_padding / 2}] -width 2
.c create line $outer_padding [expr {$middle_y + $inner_padding / 2}] [expr {$canvas_width - $outer_padding}] [expr {$middle_y + $inner_padding / 2}] -width 2

set font {Courier 12}

# ---------------
# draw sections
# ---------------
set adsr_x1 $outer_padding
set adsr_y1 $outer_padding
set adsr_x2 [expr {$middle_x - $inner_padding / 2}]
set adsr_y2 [expr {$middle_y - $inner_padding / 2}]
.c create rectangle $adsr_x1 $adsr_y1 $adsr_x2 $adsr_y2 -outline black -width 2
.c create text [expr {$adsr_x1 + 25}] [expr {($adsr_y1 + 10)}] -text "adsr" -font $font

set waveform_x1 [expr {$middle_x + $inner_padding / 2}]
set waveform_y1 $outer_padding
set waveform_x2 [expr {$canvas_width - $outer_padding}]
set waveform_y2 [expr {$middle_y - $inner_padding / 2}]
.c create rectangle $waveform_x1 $waveform_y1 $waveform_x2 $waveform_y2 -outline black -width 2
.c create text [expr {$waveform_x1 + 45}] [expr {($waveform_y1 + 10)}] -text "waveform" -font $font

set bottom_x1 $outer_padding
set bottom_y1 [expr {$middle_y + $inner_padding / 2}]
set bottom_x2 [expr {$canvas_width - $outer_padding}]
set bottom_y2 [expr {$canvas_height - $outer_padding}]
.c create rectangle $bottom_x1 $bottom_y1 $bottom_x2 $bottom_y2 -outline black -width 2
.c create text [expr {$bottom_x1 + 25}] [expr {$bottom_y1 + 10}] -text "code" -font $font

button .return_button -text "ADSR Values" -command "return_adsr"
.c create window [expr {($bottom_x1 + $bottom_x2) / 2}] [expr {($bottom_y1 + $bottom_y2) / 2}] -window .return_button

# ---------------
# adsr viz
# ---------------
set attack_time 60
set decay_time 60
set sustain_time 60
set sustain_level 100
set release_time 60
set dragging_point ""

proc draw_adsr {} {
    global attack_time decay_time sustain_time sustain_level release_time
    global .c adsr_x1 adsr_y1 adsr_y2 adsr_x2
    set canvas_height [expr {$adsr_y2 - $adsr_y1}]
    set sustain_level_height 50
    set point_radius 5
    set line_width 2
    set left_offset [expr {10 + $adsr_x1}]

    .c delete adsr_elements

    set attack_x [expr {$attack_time + $left_offset}]
    set decay_x [expr {$attack_x + $decay_time}]
    set sustain_x [expr {$decay_x + $sustain_time}]
    set release_x [expr {$sustain_x + $release_time}]

    set sustain_y [expr {$adsr_y2 - $sustain_level}]

    .c create line $left_offset $adsr_y2 $attack_x $sustain_level_height -fill black -width $line_width -tags adsr_elements
    .c create line $attack_x $sustain_level_height $decay_x $sustain_y -fill black -width $line_width -tags adsr_elements
    .c create line $decay_x $sustain_y $sustain_x $sustain_y -fill black -width $line_width -tags adsr_elements
    .c create line $sustain_x $sustain_y $release_x $adsr_y2 -fill black -width $line_width -tags adsr_elements

    .c create oval [expr {$attack_x - $point_radius}] [expr { $sustain_level_height - $point_radius}]  [expr {$attack_x + $point_radius}] [expr {$sustain_level_height + $point_radius}] -fill red -tags "adsr_elements attack_point"
    .c create oval [expr {$decay_x - $point_radius}] [expr {$sustain_y - $point_radius}]  [expr {$decay_x + $point_radius}] [expr {$sustain_y + $point_radius}] -fill red -tags "adsr_elements decay_point"
    .c create oval [expr {$sustain_x - $point_radius}] [expr {$sustain_y - $point_radius}]  [expr {$sustain_x + $point_radius}] [expr {$sustain_y + $point_radius}]  -fill red -tags "adsr_elements sustain_point"
    .c create oval [expr {$release_x - $point_radius}] [expr {$adsr_y2 - $point_radius}]   [expr {$release_x + $point_radius}] [expr {$adsr_y2 + $point_radius}]   -fill red -tags "adsr_elements release_point"
}

proc drag {x y} {
    global dragging_point attack_time decay_time sustain_time sustain_level release_time adsr_x1 adsr_x2 adsr_y1 adsr_y2

    set max_x [expr {$adsr_x2 - $adsr_x1 - 10}]
    set max_y [expr {$adsr_y2 - $adsr_y1}]

    set endpt [expr {$max_x - $attack_time - $decay_time - $sustain_time}]

    if {$dragging_point == "attack_point"} {
        set attack_time [expr {max(0, min($x - $adsr_x1, $endpt))}]
    } elseif {$dragging_point == "decay_point"} {
        set decay_time [expr {max(0, min($x - $adsr_x1 - $attack_time, $endpt))}]
        set sustain_level [expr {max(0, min($max_y - $y + $adsr_y1, $max_y))}]
    } elseif {$dragging_point == "sustain_point"} {
        set sustain_time [expr {max(0, min($x - $adsr_x1 - $attack_time - $decay_time, $endpt))}]
    } elseif {$dragging_point == "release_point"} {
        set release_time [expr {max(0, min($x - $adsr_x1 - $attack_time - $decay_time - $sustain_time, $endpt))}]
    }

    draw_adsr
}

proc start_drag {tag x y} {
    global dragging_point
    set dragging_point $tag
}

proc stop_drag {} {
    global dragging_point
    set dragging_point ""
}

proc return_adsr {} {
    global attack_time decay_time sustain_time sustain_level release_time
    puts "$attack_time $decay_time $sustain_time $sustain_level $release_time"
}

bind .c <B1-Motion> {drag %x %y}
bind .c <Button-1> {
    set x %x
    set y %y

    set clicked_point [.c find closest $x $y]
    set tags [.c gettags $clicked_point]

    foreach tag $tags {
        if {$tag == "attack_point" || $tag == "decay_point" || $tag == "sustain_point" || $tag == "release_point"} {
            start_drag $tag $x $y
            break
        }
    }
}
bind .c <ButtonRelease-1> {stop_drag}

# ---------------
# waveform viz
# ---------------

proc draw_waveform {} {
    global waveform_x1 waveform_y1 waveform_x2 waveform_y2 .c
    global signal_data
    global log

    set previousX $waveform_x1
    set previousY [expr {($waveform_y1 + $waveform_y2) / 2}]

    .c delete waveform_elements

    set signal_length [llength $signal_data]

    if {$signal_length == 0} {
        return
    }

    set step [expr {double($waveform_x2 - $waveform_x1) / double($signal_length)}]

    if {$step <= 0} {
        return
    }

    for {set i 0} {$i < $signal_length} {incr i} {
        set x [expr {$waveform_x1 + $i * $step}]

        set y_value [lindex $signal_data $i]

        set y_value [expr {($y_value / 127.0) * 3}]

        set prev_value $y_value
        set next_value $y_value

        if {$i > 0} {
            set prev_value [lindex $signal_data [expr {$i - 1}]]
            set prev_value [expr {($prev_value / 127.0) * 3}]
        }
        if {$i < $signal_length - 1} {
            set next_value [lindex $signal_data [expr {$i + 1}]]
            set next_value [expr {($next_value / 127.0) * 3}]
        }

        set y_value [expr {($prev_value + $y_value + $next_value) / 3.0}]

        if {$y_value < -3} { set y_value -3 }
        if {$y_value > 3} { set y_value 3 }

        set y [expr {($waveform_y1 + $waveform_y2) / 2 - 15 * $y_value}]

        .c create line $previousX $previousY $x $y -fill black -tags waveform_elements

        set previousX $x
        set previousY $y
    }
}


proc update_waveform {} {
    draw_waveform
}