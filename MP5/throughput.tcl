#!/usr/bin/tclsh

# Check command-line arguments
if { $argc != 2 } {
    puts "Usage: ns2.tcl <TCP_flavor> <case_number>"
    exit 1
}

#get user tcp flavor
set input_tcp_flavor [string tolower [lindex $argv 0]]
#based on MP5 problem
set case_number [lindex $argv 1]

# Check if the case number is valid
if { $case_number > 3 || $case_number < 1 } {
    #Todo:exit only, can't run any test--Done
    puts "Invalid case number entered: $case_number"
    exit 1
}

# Map input TCP flavor to valid NS2 flavor
set ns2_tcp_flavor ""

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

#delay for MP5
global link_delay
set link_delay 0

# Choose the input delay based on the case number

switch -exact -- $case_number {
    #from the MP5
    global link_delay
    1 {set link_delay "12.5ms"}
    2 {set link_delay "20.0ms"}
    3 {set link_delay "27.5ms"}
    default {
        puts "Invalid case number entered: $case_number"
        exit 1
    }
}

# Create a new NS2 simulator instance
set ns [new Simulator]

# Store data in output files
set output_file1 [open output1.tr w]
# 2 needed
set output_file2 [open output2.tr w]

# Outfile nmaing
set output_file "output_${ns2_tcp_flavor}_${case_number}"

# Trace the packets
set tracefile [open out.tr w]
$ns trace-all $tracefile

# Open a NAM trace file for visual
set namfile [open out.nam w]
#attack to simulator
$ns namtrace-all $namfile

#Counter variable
set event_counter 0

# Initialize throughputs to 0
set throughput1 0
set throughput2 0

# Create source,router and receiver
#2 dources
set source_node1 [$ns node]
set source_node2 [$ns node]
#2 routers
set router_node1 [$ns node]
set router_node2 [$ns node]
#2 Receivers
set receiver_node1 [$ns node]
set receiver_node2 [$ns node]

# Assign data flow colors for the directions for NAM
$ns color 1 Green
$ns color 2 Red

# Define TCP Agents
set tcp_agent1 [new Agent/TCP/$ns2_tcp_flavor]
set tcp_agent2 [new Agent/TCP/$ns2_tcp_flavor]
#Attach to the nodes
$ns attach-agent $source_node1 $tcp_agent1
$ns attach-agent $source_node2 $tcp_agent2
#Assign classes
$tcp_agent1 set class_ 1
$tcp_agent2 set class_ 2

# Define TCP sink 
set sink_agent1 [new Agent/TCPSink]
set sink_agent2 [new Agent/TCPSink]
#Attach to receiver
$ns attach-agent $receiver_node1 $sink_agent1
$ns attach-agent $receiver_node2 $sink_agent2
#Connect them
$ns connect $tcp_agent1 $sink_agent1
$ns connect $tcp_agent2 $sink_agent2

# Define Links
$ns duplex-link $router_node1 $router_node2 1.0Mb 5ms DropTail
#delay is sane in cases, add here only
$ns duplex-link $source_node1 $router_node1 10.0Mb 5ms DropTail
$ns duplex-link $receiver_node1 $router_node2 10.0Mb 5ms DropTail
$ns duplex-link $source_node2 $router_node1 10.0Mb $link_delay DropTail
$ns duplex-link $receiver_node2 $router_node2 10.0Mb $link_delay DropTail

# Define Nam interfaces
$ns duplex-link-op $router_node1 $router_node2 orient right
$ns duplex-link-op $source_node1 $router_node1 orient right-down
#todo:orient properly--Done
$ns duplex-link-op $source_node2 $router_node1 orient right-up
$ns duplex-link-op $router_node2 $receiver_node1 orient right-up
$ns duplex-link-op $router_node2 $receiver_node2 orient right-down

# Create FTP traffic on top of TCP connection
set ftp_application1 [new Application/FTP]
set ftp_application2 [new Application/FTP]
#attach tcp agents to the ftp
$ftp_application1 attach-agent $tcp_agent1
$ftp_application2 attach-agent $tcp_agent2

# Procedure to record simulation data
proc record_bandwidth {} {
    #redeclare global as in man pages of tcl
    global sink_agent1 sink_agent2 output_file1 output_file2 throughput1 throughput2 event_counter

    #todo:check if can be combined
    set ns [Simulator instance]

    #cuurent time
    set curr_time [$ns now]
    #time granulity
    set time_interval 0.5


    # Get the number of bytes received by sink_agent1 and sink_agent2
    set data_rate1 [$sink_agent1 set bytes_]
    set data_rate2 [$sink_agent2 set bytes_]

    # Record the bandwidth in Mbps and write to output files
    puts $output_file1 "$curr_time [expr $data_rate1/$time_interval*8/1000000]"

    puts $output_file2 "$curr_time [expr $data_rate2/$time_interval*8/1000000]"

    # Calculate throughput after 100 seconds from MP5
    if { $curr_time >= 100 } {
        #todo:check if putting in file can be combined with calculation
        set throughput1 [expr $throughput1 + $data_rate1/$time_interval*8/1000000 ]
        #todo:its the same time interval also 2nd half same--combine?
        set throughput2 [expr $throughput2 + $data_rate2/$time_interval*8/1000000 ]
    }
    
    
    #Increment as cyche over
    set event_counter [expr $event_counter + 1]

    #set to 0 fornXt cycle 
    $sink_agent1 set bytes_ 0
    $sink_agent2 set bytes_ 0

    #call again after 0.5
    $ns at [expr $curr_time + $time_interval] "record_bandwidth"
}

# Procedure to finish the simulation
proc finish_simulation {} {
    #redclare
    global ns tracefile namfile output_file throughput1 throughput2 event_counter
    
    # Flush the trace file
    $ns flush-trace

    # Calculate and print average throughput for Src1 and Src2
    puts "Average throughput for Source 1 = [expr $throughput1 / ($event_counter - 200)] Mbits/sec"
    #removing 200 as 0.5 granulity
    puts "Average throughput for Source 2 = [expr $throughput2 / ($event_counter - 200)] Mbits/sec"
    # Ratio for MP5
    puts "The Ratio of Source 1/ Source 2 = [expr $throughput1/($throughput2)]"

    #Close and exit 
    close $tracefile
    #CLose nam before opening
    close $namfile
    #Open nam output window
    exec nam out.nam &

    exit 0
}

# Schedule recording of bandwidth and finishing the simulation
$ns at 0 "record_bandwidth"

$ns at 0 "$source_node1 label Src1"

#Adding labels
$ns at 0 "$source_node2 label Src2"

$ns at 0 "$router_node1 label Router1"

$ns at 0 "$router_node2 label Router2"
#TODO: better names
$ns at 0 "$receiver_node1 label Rcv1"

$ns at 0 "$receiver_node2 label Rcv2"
#Start ftp transfer
$ns at 0 "$ftp_application1 start"

$ns at 0 "$ftp_application2 start"
#ENd ftp transfer
$ns at 400 "$ftp_application1 stop"

$ns at 400 "$ftp_application2 stop"
#End simulation and cleanup
$ns at 400 "finish_simulation"

#Todo: is it needed
$ns color 1 Green

$ns color 2 Red
#Trace File Setup for Source Node 1
set tf1 [open "$output_file-S1.tr" w]
#todo: combine both 
$ns trace-queue  $source_node1  $router_node1  $tf1

#Trace File Setup for Source Node 2
set tf2 [open "$output_file-S2.tr" w]
$ns trace-queue  $source_node2  $router_node1  $tf2
#Run the file
$ns run

