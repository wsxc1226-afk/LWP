#include <net/lwp.h>

int ip_local_deliver(struct sk_buff *skb)
{
    struct iphdr *iph = ip_hdr(skb);

    if (iph->protocol == IPPROTO_LWP) {
        // LWP 패킷 처리
        return lwp_rcv(skb);
    }

    // 기존 프로토콜 처리
    return NF_HOOK(NFPROTO_IPV4, NF_INET_LOCAL_IN,
                   NULL, skb, skb->dev, NULL,
                   ip_local_deliver_finish);
}
