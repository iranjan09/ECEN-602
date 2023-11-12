# Create a new simulation instance
set ns [new Simulator]

# Define the link characteristics
set link_bw 1Mbps
set link_delay 5ms
set access_link_bw 10Mbps

# Create nodes
set R1 $ns node
set R2 $ns node
set src1 $ns node
set src2 $ns node
set rcv1 $ns node
set rcv2 $ns node

# Create links
$ns duplex-link $R1 $R2 $link_bw $link_delay DropTail
$ns duplex-link $src1 $R1 $access_link_bw $link_delay DropTail
$ns duplex-link $src2 $R1 $access_link_bw $link_delay DropTail
$ns duplex-link $R2 $rcv1 $access_link_bw $link_delay DropTail
$ns duplex-link $R2 $rcv2 $access_link_bw $link_delay DropTail

# Set TCP versions for src1 and src2
set tcp1 [new Agent/TCP]
$src1 attach $tcp1
$tcp1 set class_ 2
set tcp2 [new Agent/TCP]
$src2 attach $tcp2
$tcp2 set class_ 2

# Set FTP application
set ftp1 [new Application/FTP]
$ftp1 attach-agent $tcp1
$ns at 0.0 "$ftp1 start"

set ftp2 [new Application/FTP]
$ftp2 attach-agent $tcp2
$ns at 0.0 "$ftp2 start"

# Set end-to-end delays for each case
# Case 1
$ns queue-limit $R1 $R2 50
$ns queue-limit $R2 $rcv1 50
$ns queue-limit $R2 $rcv2 50
$ns at 0.0 "$ns link $R1 $R2 set delay_ 5ms"
$ns at 0.0 "$ns link $src1 $R1 set delay_ 5ms"
$ns at 0.0 "$ns link $src2 $R1 set delay_ 12.5ms"

# Case 2
# Adjust queue limits and delays accordingly

# Case 3
# Adjust queue limits and delays accordingly

# Run the simulation for 400 seconds
$ns run 400s

# Print the simulation statistics
puts "Simulation finished"
$ns flush-trace
