
namespace eval networking {
    variable server_address "localhost"
    variable server_port 5000
    variable sock ""

    # ----------------------------
    # connecting
    # ----------------------------
    
    proc try_connect {address port} {
        variable sock

        while {1} {
            if { [catch {socket $address $port} sock] } {
                puts "failed to connect to $address:$port, retrying in 2 seconds..."
                after 2000
            } else {
                puts "connected to $address:$port!"
                break
            }
        }

        return $sock
    }

    proc connect {} {
        variable server_address
        variable server_port
        variable sock

        set sock [try_connect $server_address $server_port]

        if {$sock ne ""} {
            fconfigure $sock -blocking 0
            puts "connection successful, socket configured as non-blocking."
        } else {
            puts "failed to connect."
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
        if {[eof $sock]} {
            close $sock
            puts "Connection closed by server"
            return
        }

        set data [read $sock]

        set myDataVariable $data
        puts "Received data: $data"
    }

}