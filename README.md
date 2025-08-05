LWP: Lightweight Transport Protocol for the Linux Kernel
LWP (Lightweight Protocol) is an experimental transport-layer protocol implemented as a Linux kernel module. It defines a custom packet header and handles packet delivery directly at the IP layer using protocol number IPPROTO_LWP (253).

✨ Features
Kernel-space implementation of a lightweight transport protocol

Custom packet structure with SYN, ACK, FIN flags

Basic connection management (SYN, SYN-ACK, ACK, FIN, FIN-ACK)

Simple RTT-based congestion control mechanism

Integrated into the Linux IP stack via ip_local_deliver()

📁 Project Structure
include/net/lwp.h # Defines LWP header and lwp_sock structure
include/linux/in.h # Defines IPPROTO_LWP and port offset logic
net/ipv4/ip_input.c # Hooks into the IP layer to dispatch LWP packets
net/lwp_module.c # Core LWP logic: packet parsing, handlers, RTT control

📦 LWP Packet Header
struct lwp_header {
  __u16 src_port;
  __u16 dst_port;
  __u32 seq;
  __u32 ack;
  __u16 packet_size;
  __u8 flags;
};

Flags
LWP_FLAG_SYN (0x02)

LWP_FLAG_ACK (0x10)

LWP_FLAG_FIN (0x01)

🚀 RTT-Based Congestion Control
RTT is measured using jiffies, and the pacing rate is adjusted as follows:

If RTT > 100 ms → halve the sending rate

If RTT ≤ 100 ms → increase the rate gradually (up to 100)

🛠️ Build and Load
Build the kernel module
make -C /lib/modules/$(uname -r)/build M=$(pwd) modules

Insert the module
sudo insmod lwp.ko

View kernel messages
dmesg | grep lwp

Remove the module
sudo rmmod lwp

🧪 Example Log Output
lwp: protocol registered
lwp packet received: src_port=1234, dst_port=5678, seq=1, ack=0, flags=2
lwp SYN packet received: src_port=1234, dst_port=5678
lwp RTT: 18 ms
