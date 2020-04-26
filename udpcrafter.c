#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h> /* the L2 protocols */
#include <netinet/ip.h> 
#include <netinet/udp.h> 
#include <stdio.h>
#include <arpa/inet.h> /* htons */
#include <sys/ioctl.h> 
#include <net/if.h> /* ifreq */
#include <string.h> 
#include <stdlib.h>

#define BUF_SIZE 2048 


void usage(void)
{
    fprintf(stderr, "usage: craft srcip dstip srcmac dstmac srcport dstport ethif data\n");
    exit(EXIT_FAILURE);
}

void parsemac(char *macin, char *macout){

	char * token = strtok(macin, ":");
	int j= 0;
   	while( token != NULL ) {
		int num = (int)strtol(token, NULL, 16);  // number base 16
		macout[j++] = (char)num;
      		token = strtok(NULL, ":");
   	} 
	token = strtok(macin, ":");
}

//http://www.microhowto.info/howto/calculate_an_internet_protocol_checksum_in_c.html#idp22656
uint16_t ip_checksum(void* vdata,size_t length) {
    // Cast the data pointer to one that can be indexed.
    char* data=(char*)vdata;

    // Initialise the accumulator.
    uint32_t acc=0xffff;

    // Handle complete 16-bit blocks.
    for (size_t i=0;i+1<length;i+=2) {
        uint16_t word;
        memcpy(&word,data+i,2);
        acc+=ntohs(word);
        if (acc>0xffff) {
            acc-=0xffff;
        }
    }

    // Handle any partial block at the end of the data.
    if (length&1) {
        uint16_t word=0;
        memcpy(&word,data+length-1,1);
        acc+=ntohs(word);
        if (acc>0xffff) {
            acc-=0xffff;
        }
    }

    // Return the checksum in network byte order.
    return htons(~acc);
}

int main(int argc, char *argv[]){

	int sockfd, ifindex, tx_len=sizeof(struct ether_header)+sizeof(struct iphdr)+sizeof(struct udphdr);
	struct ifreq ifr;
	size_t if_name_len;
	char packet[BUF_SIZE];
	struct ether_header *eh;
	struct iphdr *iph;
	struct udphdr *udph; 
	unsigned char *data;
	u_int16_t src_port, dst_port;
	struct sockaddr_ll dst_addr;
	char dmac[6];
	char smac[6];
	
	if(argc != 9){
		usage();
	}

	parsemac(argv[4], dmac);
        parsemac(argv[3], smac); 

	memset(packet, 0, sizeof(packet));
	eh = (struct ether_header *) packet;
	iph = (struct iphdr *) (packet + sizeof(struct ether_header));
       	udph = (struct udphdr *) (packet + sizeof(struct ether_header) + sizeof(struct iphdr));
	data = (char *)packet + sizeof(struct ether_header) + sizeof(struct iphdr) + sizeof(struct udphdr);

	// create raw socket to send/receive ethernet frames that can transport all protocols
	if ((sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) == -1) {
	    perror("socket");
	}

	// get interface name length
	if_name_len = strlen(argv[7]);
	if(if_name_len < IF_NAMESIZE) {
		strncpy(ifr.ifr_name, argv[7], strlen(argv[7]));
		ifr.ifr_name[if_name_len]=0;
	}

	// get the interface index number
	if(ioctl(sockfd, SIOCGIFINDEX, &ifr) == -1){
		perror("ioctl");
	}
        ifindex = ifr.ifr_ifindex;

	// build ethernet header	
        memcpy(eh->ether_dhost, dmac, ETHER_ADDR_LEN);  
        memcpy(eh->ether_shost, smac, ETHER_ADDR_LEN);  
	eh->ether_type = htons(ETH_P_IP);

	// allocate enough memory to store the string 
	data = (char*)malloc(sizeof(char)*strlen(argv[8])+1);
	memcpy(data, argv[8], strlen(argv[8]));
	data[strlen(argv[8])] = '\0';

	int i;
	for(i=0;i<strlen(argv[8])+1;i++){
		packet[tx_len++] = data[i];
                printf(" %x\n",data[i]);
	}

	// build ip header
	iph->ihl = 5;
	iph->version = 4;
	iph->tos = 0;
	iph->tot_len = htons(sizeof(struct iphdr) + sizeof(struct udphdr) + (sizeof(char)*strlen(argv[8])+1));
	iph->id = htons(54321);
	iph->frag_off = 0x00;
	iph->ttl = 0xFF;
	iph->protocol = IPPROTO_UDP;
	iph->check = 0;
	iph->saddr = inet_addr(argv[1]);
	iph->daddr = inet_addr(argv[2]);
	iph->check = ip_checksum(iph, iph->ihl << 2); 

	// build udp header
	udph->source = htons(atoi(argv[5]));
	udph->dest = htons(atoi(argv[6]));
	udph->len = htons(sizeof(struct udphdr) + (sizeof(char)*strlen(argv[8])+1) );
	udph->check = 0;

	memset(&dst_addr, 0, sizeof(struct sockaddr_ll));	
	dst_addr.sll_ifindex = ifr.ifr_ifindex;	
	dst_addr.sll_halen = ETH_ALEN;
	memcpy(dst_addr.sll_addr, dmac, ETH_ALEN);
	
        if (sendto(sockfd, packet, tx_len, 0, (struct sockaddr*)&dst_addr, sizeof(struct sockaddr_ll)) < 0)
	    printf("Send failed\n");
	return 0;
}

