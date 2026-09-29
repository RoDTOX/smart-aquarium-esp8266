#!/data/data/com.termux/files/usr/bin/bash
echo "[Nexus] Initializing iptables port 80 redirection..."
su -c '
    iptables -t nat -C PREROUTING -p tcp --dport 80 -j REDIRECT --to-ports 8088 2>/dev/null || iptables -t nat -A PREROUTING -p tcp --dport 80 -j REDIRECT --to-ports 8088
    iptables -t nat -C OUTPUT -p tcp -o lo --dport 80 -j REDIRECT --to-ports 8088 2>/dev/null || iptables -t nat -A OUTPUT -p tcp -o lo --dport 80 -j REDIRECT --to-ports 8088
    iptables -t nat -C OUTPUT -p tcp -d 100.83.135.74 --dport 80 -j REDIRECT --to-ports 8088 2>/dev/null || iptables -t nat -A OUTPUT -p tcp -d 100.83.135.74 --dport 80 -j REDIRECT --to-ports 8088
    iptables -t nat -C OUTPUT -p tcp -d 192.168.1.28 --dport 80 -j REDIRECT --to-ports 8088 2>/dev/null || iptables -t nat -A OUTPUT -p tcp -d 192.168.1.28 --dport 80 -j REDIRECT --to-ports 8088
'

echo "[Nexus] Starting Nginx Portal & Reverse Proxy..."
while true; do
    proot-distro login debian -- /usr/sbin/nginx -g "daemon off;"
    echo "[Nexus] Nginx stopped. Restarting in 3 seconds..."
    sleep 3
done
