 git clone https://github.com/noiseoverip/contiki.git
 make TARGET=dc-dc_converter
contiki/examples/microgridNode

# 1. Static 2000::1/64 Ipv6 on wired ethernet interface
#    We have to fight ubuntu automatic machinery

cat   /etc/network/interfaces
auto lo
iface lo inet loopback

iface eth3 inet static
  address 192.168.2.170
  netmask 255.255.255.0
  network 192.168.2.0
  broadcast 255.255.255.255
  gateway 192.168.2.1
iface eth3 inet6 static
  pre-up modprobe ipv6
  address 2000::1
  netmask 64

# Manually addr setting
ip add add 2000::1/64 dev eth3 
ip link set up dev eth3 

# Turn on ip6 routing to enable address assignment
# Uncommnet write to /proc/sys/net/ipv6/conf/

net.ipv6.conf.all.forwarding=1
/etc/sysctl.conf

# We have to install and run radvd to assign addresses to IoT-controller
# units. 

# Ubuntu intsall
apt-get install radvd

# Config
/etc/radvd.conf
interface eth3
{
     AdvSendAdvert on;
     prefix 2000::4/64
     {
          AdvOnLink on;
          AdvAutonomous on;
          AdvRouterAddr on;
     };
};

/usr/sbin/radvd

#check neighbors
ip -6 n 

#Use firefox to discover it.

firefox coap://[2000::121f:e0ff:fe12:1d02]/

#Note brackets when using raw IP6 addr.

