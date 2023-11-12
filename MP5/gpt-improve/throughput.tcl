#!/usr/bin/tclsh

# Check command-line arguments
proc check_arguments {} {
    global argc argv

    if { $argc != 2 } {
        puts "Usage: ns2.tcl <TCP_flavor> <case_number>"
        exit 1
    }
}

# Get user input
proc get_user_input {} {
    global argv input_tcp_flavor case_number

    # Get user TCP flavor
    set input_tcp_flavor [string tolower [lindex $argv 0]]
    # Based on MP5 problem
    set case_number [lindex $argv 1]
}

# Map input TCP flavor to valid NS2 flavor
proc map_tcp_flavor {} {
    global input_tcp_flavor ns2_tcp_flavor

    switch -exact -- $input_tcp_flavor {
        "sack" {
            set ns2_tcp_flavor "Sack1"
        }
        "vegas" {
            set ns2_tcp_flavor "Vegas"
        }
        default {
            puts "Invalid TCP flavor entered: $input_tcp_flavor"
            exit 1
        }
    }
}

# Choose the input delay based on the case number
proc choose_link_delay {} {
    global case_number link_delay

    switch -exact -- $case_number {
        1 {set link_delay "12.5ms"}
        2 {set link_delay "20.0ms"}
        3 {set link_delay "27.5ms"}
        default {
            puts "Invalid case number entered: $case_number"
            exit 1
        }
    }
}

# Set up simulation
proc setup_simulation {simulator ns2_tcp_flavor link_delay output_file1 output_file2} {
    global link_delay

    set output_file "output_${ns2_tcp_flavor}_${case_number}"

    # ... (rest of the setup code)

    # Create FTP traffic on top of TCP connection
    set ftp_application1 [new Application/FTP]
    set ftp_application2 [new Application/FTP]
    
    # ... (rest of the setup code)
}

# Record bandwidth procedure
proc record_bandwidth {simulator sink_agent1 sink_agent2 output_file1 output_file2 throughput1 throughput2 event_counter} {
    global link_delay

    # ... (rest of the recording code)
}

# Finish simulation procedure
proc finish_simulation {simulator tracefile namfile output_file throughput1 throughput2 event_counter} {
    global link_delay

    # ... (rest of the finish_simulation code)
}

# Main function
proc main {} {
    global link_delay

    check_arguments
    get_user_input
    map_tcp_flavor
    choose_link_delay

    # Store data in output files
    set output_file1 [open output1.tr w]
    set output_file2 [open output2.tr w]

    # Create a new NS2 simulator instance
    set simulator [new Simulator]

    # ... (call setup_simulation and other procedures)

    # Schedule recording of bandwidth and finishing the simulation
    $simulator at 0 "record_bandwidth"

    # ... (rest of the code)
}

# Call the main function
main
