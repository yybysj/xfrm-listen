/*
    gcc xfrm_listen.c `pkg-config --cflags --libs libnl-3.0 libnl-xfrm-3.0`
*/
#include <arpa/inet.h>
#include <linux/netlink.h>
#include <linux/xfrm.h>
#include <netlink/types.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <stdio.h>
#include <string.h>

#define PFUNC()  printf("[+%s]\n", __FUNCTION__)
#define NLMSG_TYPE(type)    printf(#type " : %d\n", type)

int parse_nlmsg(struct nl_msg *, void *);
void nlmsg_type_map();
void parse_sa(struct nlmsghdr *nlh);
void parse_sp(struct nlmsghdr *nlh);

int main()
{
	struct nl_sock *sock;
	sock = nl_socket_alloc();

/* broadcast group
#define XFRMGRP_ACQUIRE         1
#define XFRMGRP_EXPIRE          2
#define XFRMGRP_SA              4
#define XFRMGRP_POLICY          8
#define XFRMGRP_REPORT          0x20
*/
	nl_join_groups(sock, XFRMGRP_SA | XFRMGRP_POLICY);

	nl_connect(sock, NETLINK_XFRM);

	nl_socket_modify_cb(sock,
			    NL_CB_MSG_IN, NL_CB_CUSTOM, parse_nlmsg, NULL);

	while (1)
		nl_recvmsgs_default(sock);
	return 0;
}

void nlmsg_type_map()
{
	/*
	   xfrm message type list
	   libnl/include/linux-private/linux/xfrm.h :152
	 */
	printf("+----------<XFRM_MSG_TYPE : ID>--------+\n");
	NLMSG_TYPE(XFRM_MSG_NEWSA);
	NLMSG_TYPE(XFRM_MSG_DELSA);
	NLMSG_TYPE(XFRM_MSG_GETSA);
	NLMSG_TYPE(XFRM_MSG_NEWPOLICY);
	NLMSG_TYPE(XFRM_MSG_DELPOLICY);
	NLMSG_TYPE(XFRM_MSG_GETPOLICY);
	NLMSG_TYPE(XFRM_MSG_FLUSHSA);
	NLMSG_TYPE(XFRM_MSG_FLUSHPOLICY);
	printf("+--------------------------------------+\n");
}

int parse_nlmsg(struct nl_msg *nlmsg, void *arg)
{
	PFUNC();
	//nlmsg_type_map();
	//nl_msg_dump(nlmsg, stdout);

	struct nlmsghdr *nlhdr;
	int len;
	nlhdr = nlmsg_hdr(nlmsg);
	len = nlhdr->nlmsg_len;

	for (nlhdr; NLMSG_OK(nlhdr, len); nlhdr = NLMSG_NEXT(nlhdr, len)) {
		switch (nlhdr->nlmsg_type) {
		case XFRM_MSG_NEWSA:
		case XFRM_MSG_DELSA:
		case XFRM_MSG_GETSA:
			parse_sa(nlhdr);
			break;
		case XFRM_MSG_NEWPOLICY:
		case XFRM_MSG_DELPOLICY:
		case XFRM_MSG_GETPOLICY:
			parse_sp(nlhdr);
			break;
		}
	}
	return 0;
}

void dump_hex(char *buf, int len)
{
	int i;
	printf("0x");
	for (i = 0; i < len; i++) {
		printf("%02x", buf[i] & 0xff);
	}
	printf("\n");
}

void parse_sa(struct nlmsghdr *nlh)
{
	PFUNC();
	/*
	   libnl/include/netlink/xfrm/sa.h
	 */
	struct xfrmnl_sa *sa = xfrmnl_sa_alloc();
	xfrmnl_sa_parse(nlh, &sa);

	struct xfrmnl_sel *sel = xfrmnl_sa_get_sel(sa);
	struct nl_addr *sel_src = xfrmnl_sel_get_saddr(sel);
	struct nl_addr *sel_dst = xfrmnl_sel_get_daddr(sel);
	char src[16];
	char dst[16];
	nl_addr2str(sel_src, src, 16);
	nl_addr2str(sel_dst, dst, 16);

	struct nl_addr *nlsaddr = xfrmnl_sa_get_saddr(sa);
	struct nl_addr *nldaddr = xfrmnl_sa_get_daddr(sa);
	char saddr[16];
	char daddr[16];
	nl_addr2str(nlsaddr, saddr, 16);
	nl_addr2str(nldaddr, daddr, 16);

/*
    root@ubuntu:~/libnl# cat /etc/protocols |grep -i ipsec
    esp 50  IPSEC-ESP   # Encap Security Payload [RFC2406]
    ah  51  IPSEC-AH    # Authentication Header [RFC2402]
*/
	uint8_t proto = (uint8_t) xfrmnl_sa_get_proto(sa);
	uint32_t spi = (uint32_t) xfrmnl_sa_get_spi(sa);
	uint32_t reqid = xfrmnl_sa_get_reqid(sa);
	int mode = xfrmnl_sa_get_mode(sa);
	char s_mode[32];
	xfrmnl_sa_mode2str(mode, s_mode, 32);
	uint8_t replay_win = xfrmnl_sa_get_replay_window(sa);

	char enc_alg[64];
	char enc_key[1024];
	unsigned int enc_key_len;
	xfrmnl_sa_get_crypto_params(sa, enc_alg, &enc_key_len, enc_key);

	char auth_alg[64];
	char auth_key[1024];
	unsigned int auth_key_len;
	unsigned int auth_trunc_len;
	xfrmnl_sa_get_auth_params(sa, auth_alg, &auth_key_len, &auth_trunc_len,
				  auth_key);
/*
    dump to stdout
*/
	printf(" src : %s\t\t dst : %s\n", saddr, daddr);
	printf(" proto : %d(esp:50 ah:51)\t\tspi : 0x%x \n", proto, spi);
	printf(" repid : %u \t\tmode : %s\n", reqid, s_mode);
	printf(" replay window : %d\n", replay_win);
	printf(" %s \t", auth_alg);
	dump_hex(auth_key, auth_key_len / 8);
	printf(" %s \t", enc_alg);
	dump_hex(enc_key, enc_key_len / 8);
	printf(" sel src : %s\t dst : %s\n", src, dst);

	xfrmnl_sa_put(sa);
}

void parse_sp(struct nlmsghdr *nlh)
{
	PFUNC();
	/*
	   libnl/include/netlink/xfrm/sp.h
	 */
}
