#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/net.h>
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <net/protocol.h>
#include <net/sock.h>
#include <net/inet_common.h>
#include <net/inet_sock.h>
#include <net/ip.h>
#include <net/lwp.h>

static void handle_lwp_syn(struct sk_buff *skb, struct lwp_header *lh);
static void handle_lwp_ack(struct sk_buff *skb, struct lwp_header *lh);
static void handle_lwp_syn_ack(struct sk_buff *skb, struct lwp_header *lh);
static void handle_lwp_fin(struct sk_buff *skb, struct lwp_header *lh);
static void handle_lwp_fin_ack(struct sk_buff *skb, struct lwp_header *lh);

static int lwp_rcv(struct sk_buff *skb)
{
    struct lwp_header *lh;

    if (!pskb_may_pull(skb, sizeof(struct lwp_header)))
        return -1;

    lh = (struct lwp_header *)skb_transport_header(skb);

    printk(KERN_INFO "lwp packet received: src_port=%u, dst_port=%u, seq=%u, ack=%u, packet_size=%u, flags=%u\n",
           ntohs(lh->src_port), ntohs(lh->dst_port), ntohl(lh->seq), ntohl(lh->ack), ntohs(lh->packet_size), lh->flags);

    // 패킷 지연 기반 혼잡 제어를 위해 RTT 측정
    struct sock *sk = skb->sk;
    if (sk) {
        u32 rtt = jiffies_to_msecs(jiffies) - sk->sk_stamp.tv64;
        printk(KERN_INFO "lwp RTT: %u ms\n", rtt);

        // 혼잡 제어 로직
        if (rtt > 100) { // RTT가 100ms 이상이면 송신 속도를 줄임
            sk->sk_pacing_rate = max(sk->sk_pacing_rate / 2, 1UL);
        } else { // RTT가 일정하면 송신 속도를 점진적으로 증가
            sk->sk_pacing_rate = min(sk->sk_pacing_rate + 1, 100UL);
        }
    }

    // flags를 기반으로 패킷 처리
    if (lh->flags & LWP_FLAG_SYN) {
        if (lh->flags & LWP_FLAG_ACK) {
            handle_lwp_syn_ack(skb, lh);
        } else {
            handle_lwp_syn(skb, lh);
        }
    } else if (lh->flags & LWP_FLAG_ACK) {
        handle_lwp_ack(skb, lh);
    } else if (lh->flags & LWP_FLAG_FIN) {
        if (lh->flags & LWP_FLAG_ACK) {
            handle_lwp_fin_ack(skb, lh);
        } else {
            handle_lwp_fin(skb, lh);
        }
    } else {
        printk(KERN_WARNING "lwp: unknown flags: %u\n", lh->flags);
    }

    return 0;
}

static void handle_lwp_syn(struct sk_buff *skb, struct lwp_header *lh)
{
    printk(KERN_INFO "lwp SYN packet received: src_port=%u, dst_port=%u\n",
           ntohs(lh->src_port), ntohs(lh->dst_port));

    // SYN 패킷 처리 로직
    // 연결 설정을 위한 초기화 작업
    struct sock *sk = skb->sk;
    if (!sk) return;

    sk->sk_state = TCP_SYN_RECV;
    sk->sk_ack_backlog = 0;
    sk->sk_rcv_saddr = ip_hdr(skb)->daddr;
    sk->sk_daddr = ip_hdr(skb)->saddr;
    sk->sk_dport = lh->src_port;
    sk->sk_rcv_nxt = ntohl(lh->seq) + 1;
    sk->sk_snd_nxt = ntohl(lh->ack);
}

static void handle_lwp_ack(struct sk_buff *skb, struct lwp_header *lh)
{
    printk(KERN_INFO "lwp ACK packet received: src_port=%u, dst_port=%u\n",
           ntohs(lh->src_port), ntohs(lh->dst_port));

    // ACK 패킷 처리 로직
    // 데이터 수신 확인 작업
    struct sock *sk = skb->sk;
    if (!sk) return;

    sk->sk_ack_backlog--;
    sk->sk_rcv_nxt = ntohl(lh->seq) + 1;
}

static void handle_lwp_syn_ack(struct sk_buff *skb, struct lwp_header *lh)
{
    printk(KERN_INFO "lwp SYN-ACK packet received: src_port=%u, dst_port=%u\n",
           ntohs(lh->src_port), ntohs(lh->dst_port));

    // SYN-ACK 패킷 처리 로직
    // 연결 설정을 위한 응답 처리
    struct sock *sk = skb->sk;
    if (!sk) return;

    sk->sk_state = TCP_ESTABLISHED;
    sk->sk_ack_backlog = 0;
    sk->sk_rcv_nxt = ntohl(lh->seq) + 1;
    sk->sk_snd_nxt = ntohl(lh->ack);
}

static void handle_lwp_fin(struct sk_buff *skb, struct lwp_header *lh)
{
    printk(KERN_INFO "lwp FIN packet received: src_port=%u, dst_port=%u\n",
           ntohs(lh->src_port), ntohs(lh->dst_port));

    // FIN 패킷 처리 로직
    // 연결 해제 요청 처리
    struct sock *sk = skb->sk;
    if (!sk) return;

    sk->sk_state = TCP_CLOSE_WAIT;
    sk->sk_rcv_nxt = ntohl(lh->seq) + 1;
}

static void handle_lwp_fin_ack(struct sk_buff *skb, struct lwp_header *lh)
{
    printk(KERN_INFO "lwp FIN-ACK packet received: src_port=%u, dst_port=%u\n",
           ntohs(lh->src_port), ntohs(lh->dst_port));

    // FIN-ACK 패킷 처리 로직
    // 연결 해제 요청 응답 처리
    struct sock *sk = skb->sk;
    if (!sk) return;

    sk->sk_state = TCP_CLOSE;
    sk->sk_rcv_nxt = ntohl(lh->seq) + 1;
    sk->sk_snd_nxt = ntohl(lh->ack);
}

static struct net_protocol lwp_protocol = {
    .handler = lwp_rcv,
    .no_policy = 1,
};

static int __init lwp_init(void)
{
    if (inet_add_protocol(&lwp_protocol, IPPROTO_LWP) < 0) {
        printk(KERN_ERR "lwp: unable to register protocol\n");
        return -1;
    }

    printk(KERN_INFO "lwp: protocol registered\n");
    return 0;
}

static void __exit lwp_exit(void)
{
    inet_del_protocol(&lwp_protocol, IPPROTO_LWP);
    printk(KERN_INFO "lwp: protocol unregistered\n");
}

module_init(lwp_init);
module_exit(lwp_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("LWP Protocol");
MODULE_AUTHOR("Your Name");
