
# Manually addr setting
ip addr add 2000::1/64 dev eth3 
sleep 2
ip link set up dev eth3 

# start daemon
/usr/sbin/radvd

#check ip6 neighbors and find our board
ip -6 n 


