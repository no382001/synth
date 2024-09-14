
namespace eval networking {
    variable server_address "localhost"
    variable server_port 5000
    variable sock ""

    # ----------------------------
    # connecting
    # ----------------------------
    
    proc try_connect {address port} {
        variable sock
        global log

        while {1} {
            if { [catch {socket $address $port} sock] } {
                ${log}::notice "failed to connect to $address:$port, retrying in 2 seconds..."
                after 2000
            } else {
                ${log}::notice "connected to $address:$port!"
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
            ${log}::notice "connection successful, socket configured as non-blocking."
        } else {
            ${log}::error "failed to connect."
        }
        
        # its like select() in c
        fileevent $sock readable receive_data
    }

    # ----------------------------
    # sending
    # ----------------------------

    proc send_keypress {key} {
        variable sock

        puts $sock $key
        flush $sock
    }

    bind . <KeyPress> {
        send_keypress %A
    }

    # ----------------------------
    # receiving
    # ----------------------------

    proc receive_data {} {
        variable sock
        global log
        if {[eof $sock]} {
            close $sock
            ${log}::notice "connection closed by server"
            return
        }

        set data [read $sock]

        set myDataVariable $data
        ${log}::debug "received data: $data"
    }

}