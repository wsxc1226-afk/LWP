#ifndef _LINUX_IN_H
#define _LINUX_IN_H

#include <linux/errno.h>
#include <uapi/linux/in.h>

#define IPPROTO_LWP 253 // 실험 프로토콜 번호

static inline int proto_ports_offset(int proto)
{
	switch (proto) {
	case IPPROTO_TCP:
	case IPPROTO_UDP:
	case IPPROTO_DCCP:
	case IPPROTO_ESP:	/* SPI */
	case IPPROTO_SCTP:
	case IPPROTO_UDPLITE:
	case IPPROTO_LWP:
		return 0;
	case IPPROTO_AH:	/* SPI */
		return 4;
	default:
		return -EINVAL;
	}
}
