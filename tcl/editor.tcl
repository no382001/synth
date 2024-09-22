package provide editor 0.1

set BUFFER {}
set currentLine 0
set currentCol 0
set searchpattern ""
set cursorPosition [list 0 0]
set previousCursorPosition [list 0 0]

set monoFont {Courier 12}

wm title . "s"

frame .content
pack .content -side top -fill both -expand true

canvas .content.canvas -width 500 -height 300
pack .content.canvas -side left -fill both -expand true

frame .content.frame
set frame_id [.content.canvas create window 0 0 -anchor nw -window .content.frame]

proc update_line {row} {
    global BUFFER cursorPosition monoFont

    set line [lindex $BUFFER $row]
    set cursorPos [lindex $cursorPosition 1]

    if {$row == [lindex $cursorPosition 0]} {
        set beforeCursor [string range $line 0 [expr {$cursorPos - 1}]]
        set cursorChar [string index $line $cursorPos]
        set afterCursor [string range $line [expr {$cursorPos + 1}] end]

        set lineWithCursor "$beforeCursor\[$cursorChar\]$afterCursor"
        .content.frame.line$row configure -text $lineWithCursor
    } else {
        .content.frame.line$row configure -text $line
    }
}

proc update_display {} {
    global BUFFER cursorPosition monoFont

    foreach widget [winfo children .content.frame] {
        destroy $widget
    }

    set row 0
    foreach line $BUFFER {
        if {$row == [lindex $cursorPosition 0]} {
            set cursorPos [lindex $cursorPosition 1]

            set beforeCursor [string range $line 0 [expr {$cursorPos - 1}]]
            set cursorChar [string index $line $cursorPos]
            set afterCursor [string range $line [expr {$cursorPos + 1}] end]

            set lineWithCursor "$beforeCursor\[$cursorChar\]$afterCursor"
            label .content.frame.line$row -text $lineWithCursor -anchor w -font $monoFont
        } else {
            label .content.frame.line$row -text "$line" -anchor w -font $monoFont
        }
        pack .content.frame.line$row -side top -fill x
        incr row
    }
}

proc move_cursor {direction} {
    global BUFFER cursorPosition previousCursorPosition

    set previousCursorPosition $cursorPosition

    switch -- $direction {
        "up" {
            if {[lindex $cursorPosition 0] > 0} {
                set cursorPosition [list [expr {[lindex $cursorPosition 0] - 1}] [lindex $cursorPosition 1]]
                # bound check
                set lineLen [string length [lindex $BUFFER [lindex $cursorPosition 0]]]
                if {[lindex $cursorPosition 1] > $lineLen} {
                    set cursorPosition [list [lindex $cursorPosition 0] $lineLen]
                }
            }
        }
        "down" {
            if {[lindex $cursorPosition 0] < [expr {[llength $BUFFER] - 1}]} {
                set cursorPosition [list [expr {[lindex $cursorPosition 0] + 1}] [lindex $cursorPosition 1]]
                set lineLen [string length [lindex $BUFFER [lindex $cursorPosition 0]]]
                if {[lindex $cursorPosition 1] > $lineLen} {
                    set cursorPosition [list [lindex $cursorPosition 0] $lineLen]
                }
            }
        }
        "left" {
            if {[lindex $cursorPosition 1] > 0} {
                set cursorPosition [list [lindex $cursorPosition 0] [expr {[lindex $cursorPosition 1] - 1}]]
            }
        }
        "right" {
            if {[lindex $cursorPosition 1] < [string length [lindex $BUFFER [lindex $cursorPosition 0]]]} {
                set cursorPosition [list [lindex $cursorPosition 0] [expr {[lindex $cursorPosition 1] + 1}]]
            }
        }
    }
    
    # update only changes
    update_line [lindex $previousCursorPosition 0]
    update_line [lindex $cursorPosition 0]
}

bind . <Key-Up> {move_cursor up}
bind . <Key-Down> {move_cursor down}
bind . <Key-Left> {move_cursor left}
bind . <Key-Right> {move_cursor right}

set BUFFER {
    "This is line 1."
    "This is line 2."
    "This is line 3."
    "This is line 4."
    "This is line 5."
}

update_display

bind . <KeyPress> {handle_key_press %K}

proc handle_key_press {k} {
    global cursorPosition
    set col [lindex $cursorPosition 0]
    set lin [lindex $cursorPosition 1]
    puts "ins $col $lin $k"
}