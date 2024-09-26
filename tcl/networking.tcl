namespace eval networking {
    
    variable server_address "localhost"
    variable server_port 5000
    variable sock ""
    array set key_states {}

    set debounce_time 100

    # ----------------------------
    # connecting
    # ----------------------------
    proc try_connect {address port} {
        variable sock
        global log

        while {1} {
            if { [catch {socket $address $port} sock] } {
                ${log}::notice "Failed to connect to $address:$port, retrying in 2 seconds..."
                after 2000
            } else {
                ${log}::notice "Connected to $address:$port!"
                break
            }
        }
        return $sock
    }

    proc connect {} {
        variable server_address
        variable server_port
        variable sock
        global log

        set sock [try_connect $server_address $server_port]

        if {$sock ne ""} {
            fconfigure $sock -blocking 0
            ${log}::notice "Connection successful, socket configured as non-blocking."
        } else {
            ${log}::error "Failed to connect."
        }

        fileevent $sock readable networking::receive_data
    }

    # ----------------------------
    # sending key press/release
    # ----------------------------
    proc send_key {key action} {
        variable sock
        global log
        if {$sock ne ""} {
            set msg "$action $key"
            ${log}::notice "sent: $msg"
            puts -nonewline $sock $msg
            flush $sock
        } else {
            ${log}::error "socket is not available."
        }
    }

    # ----------------------------
    # key press
    # ----------------------------
    proc handle_keypress {key} {
        variable key_states
        global log
        variable debounce_time

        if {[info exists key_states(${key}_timeout)]} {
            after cancel $key_states(${key}_timeout)
            unset key_states(${key}_timeout)
        }

        # if key is not currently pressed, mark it as pressed and send "set"
        if {![info exists key_states($key)] || $key_states($key) == "released"} {
            set key_states($key) "pressed"
            networking::send_key $key "set"
        }
    }

    # ----------------------------
    # handle autorepeat
    # ----------------------------
    proc handle_keyrelease {key} {
        variable key_states
        global log
        variable debounce_time

        # only process the release after a debounce time, and cancel if pressed again before the timeout
        set key_states(${key}_timeout) [after $debounce_time [list networking::process_keyrelease $key]]
    }

    # ----------------------------
    # key release
    # ----------------------------
    proc process_keyrelease {key} {
        variable key_states
        global log

        # only process release if key is still pressed
        if {[info exists key_states($key)] && $key_states($key) == "pressed"} {
            set key_states($key) "released"
            unset key_states(${key}_timeout)
            networking::send_key $key "res"
        }
    }

    # ----------------------------
    # receiving data
    # ----------------------------
    proc receive_data {} {
        variable sock
        global log
        if {[eof $sock]} {
            close $sock
            ${log}::notice "Connection closed by server."
            networking::connect
        }

        set data [read $sock]
        ${log}::debug "received data: $data"
    }

    # ----------------------------
    # bindings
    # ----------------------------
    bind . <KeyPress> {
        networking::handle_keypress %K
    }

    bind . <KeyRelease> {
        networking::handle_keyrelease %K
    }
}
