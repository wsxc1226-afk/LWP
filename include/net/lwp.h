#ifndef _LINUX_LWP_H
#define _LINUX_LWP_H

#include <linux/types.h>
#include <linux/net.h>

struct lwp_header {
    __u16 src_port;
    __u16 dst_port;
    __u32 seq;
    __u32 ack;
    __u16 packet_size;
    __u8 flags;
};

#define LWP_FLAG_ACK 0x10
#define LWP_FLAG_SYN 0x02
#define LWP_FLAG_FIN 0x01

// LWP 소켓 구조체 정의
struct lwp_sock {
    struct sock sk;
    __u32 snd_nxt;
    __u32 rcv_nxt;
    __u32 snd_una;
    __u32 rtt;
    unsigned long pacing_rate;
};

#define lwp_sk(sk) ((struct lwp_sock *)(sk))

#endif /* _LINUX_LWP_H */
