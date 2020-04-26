# udpcrafter
This program crafts an Ethernet frame which transport an UDP datagram and then sends it through a chosen Ethernet interface.
It allows to spoof both IP src address and src MAC address.

## PREREQUISITES
An ethernet link. To configure it:

1 - Directly connect 2 computers through an ethernet cable

2 - Assign to both computers ethernet interfaces an IP address in the same subnet

`ifconfig eth0 10.0.0.1 netmask 255.255.255.0 up.`

`ifconfig enp0s3 10.0.0.2 netmask 255.255.255.0 up.`

## USAGE 
`usage: craft srcip dstip srcmac dstmac srcport dstport ethif data`

### EXAMPLE
On one computer listen udp connections on `5555` port

`nc -ulvvp 5555`

On the other 

`gcc udpcrafter.c -o udpcrafter`

`./udpcrafter 10.0.0.8 10.0.0.2 aa:bb:cc:dd:ee:ff a2:bd:ee:a9:d7:ff 4444 5555 eth0 hello`

You can check the content of the fields using Wireshark 


![UI](https://github.com/midist0xf/udpcrafter/blob/master/wireshark.png)
