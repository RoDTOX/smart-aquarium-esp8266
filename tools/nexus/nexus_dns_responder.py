#!/usr/bin/env python3
"""
Nexus & Galaxy-A6 Local Name Responder (LLMNR + NetBIOS + mDNS)
Enables seamless resolution of:
  - http://galaxy-a6/ and http://galaxy-a6.local/
  - http://nexus/ and http://nexus.local/
  - http://cinderella/ and http://cinderella.local/
on Android, Windows, macOS, and Linux across the local Wi-Fi.
Target IP: 192.168.1.28
"""

import socket
import struct
import threading
import sys
import os

TARGET_NAMES = ["galaxy-a6", "nexus", "cinderella"]
TARGET_IP = "192.168.1.28"
IP_BYTES = socket.inet_aton(TARGET_IP)

def encode_nbname(name):
    padded = name.upper().ljust(16)[:16]
    return b"".join(bytes([(ord(c) >> 4) + ord('A'), (ord(c) & 0xF) + ord('A')]) for c in padded)

ENCODED_NBNAMES = {name: encode_nbname(name) for name in TARGET_NAMES}

def encode_dns_name(domain):
    parts = domain.strip('.').split('.')
    buf = bytearray()
    for p in parts:
        buf.append(len(p))
        buf.extend(p.encode('utf-8'))
    buf.append(0)
    return bytes(buf)

# 1. LLMNR Responder (UDP 5355, 224.0.0.252) - Used by Windows
def run_llmnr():
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        try:
            sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
        except AttributeError:
            pass
        sock.bind(('', 5355))
        mreq = struct.pack("4sl", socket.inet_aton("224.0.0.252"), socket.INADDR_ANY)
        sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)
        print(f"[LLMNR] Listening on 224.0.0.252:5355 for {TARGET_NAMES}...")

        while True:
            data, addr = sock.recvfrom(2048)
            if len(data) < 12:
                continue
            tx_id = data[:2]
            flags = struct.unpack("!H", data[2:4])[0]
            if flags & 0x8000: # response, ignore
                continue
            # Parse question
            idx = 12
            qname_parts = []
            while idx < len(data) and data[idx] != 0:
                length = data[idx]
                idx += 1
                qname_parts.append(data[idx:idx+length].decode('utf-8', errors='ignore'))
                idx += length
            idx += 1 # skip null terminator
            if idx + 4 > len(data):
                continue
            qtype, qclass = struct.unpack("!HH", data[idx:idx+4])
            qname = ".".join(qname_parts).lower()

            if qname in TARGET_NAMES and qtype in (1, 255): # A or ANY
                resp = bytearray()
                resp.extend(tx_id)
                resp.extend(b"\x80\x00") # Response, No error
                resp.extend(b"\x00\x01\x00\x01\x00\x00\x00\x00")
                resp.extend(data[12:idx+4]) # Copy question section
                resp.extend(b"\xc0\x0c\x00\x01\x00\x01\x00\x00\x00\x1e\x00\x04") # A IN TTL 30s Len 4
                resp.extend(IP_BYTES)
                sock.sendto(resp, addr)
                print(f"[LLMNR] Answered query '{qname}' from {addr[0]} -> {TARGET_IP}")
    except Exception as e:
        print(f"[LLMNR] Error: {e}", file=sys.stderr)

# 2. NetBIOS Name Service Responder (UDP 137) - Legacy Windows resolution
def run_nbns():
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        sock.bind(('', 137))
        print(f"[NBNS] Listening on UDP 137 for {[n.upper() for n in TARGET_NAMES]}...")

        while True:
            data, addr = sock.recvfrom(2048)
            if len(data) < 12:
                continue
            tx_id = data[:2]
            flags = struct.unpack("!H", data[2:4])[0]
            if flags & 0x8000:
                continue
            idx = 12
            if idx >= len(data) or data[idx] != 0x20:
                continue
            idx += 1
            if idx + 32 > len(data):
                continue
            raw_nbname = data[idx:idx+32]
            
            matched = False
            for name, enc in ENCODED_NBNAMES.items():
                if raw_nbname == enc:
                    matched = True
                    break
            
            if matched:
                resp = bytearray()
                resp.extend(tx_id)
                resp.extend(b"\x85\x00") # Response, Authoritative, Recursion Available
                resp.extend(b"\x00\x00\x00\x01\x00\x00\x00\x00") # 0 questions, 1 answer
                resp.append(0x20)
                resp.extend(raw_nbname)
                resp.append(0x00)
                resp.extend(b"\x00\x20\x00\x01\x00\x00\x01\x2c\x00\x06") # NB IN TTL 300s Len 6
                resp.extend(b"\x00\x00") # Flags unique B-node
                resp.extend(IP_BYTES)
                sock.sendto(resp, addr)
                print(f"[NBNS] Answered query from {addr[0]} -> {TARGET_IP}")
    except Exception as e:
        print(f"[NBNS] Error: {e}", file=sys.stderr)

# 3. mDNS Responder (UDP 5353, 224.0.0.251) - .local for Android/iOS/Mac/Linux
def run_mdns():
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        try:
            sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
        except AttributeError:
            pass
        sock.bind(('', 5353))
        mreq = struct.pack("4sl", socket.inet_aton("224.0.0.251"), socket.INADDR_ANY)
        sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)
        print(f"[mDNS] Listening on 224.0.0.251:5353 for {[n + '.local' for n in TARGET_NAMES]}...")

        valid_mdns = set()
        for name in TARGET_NAMES:
            valid_mdns.add(name)
            valid_mdns.add(name + ".local")

        while True:
            data, addr = sock.recvfrom(2048)
            if len(data) < 12:
                continue
            flags = struct.unpack("!H", data[2:4])[0]
            if flags & 0x8000:
                continue
            idx = 12
            qname_parts = []
            while idx < len(data) and data[idx] != 0:
                length = data[idx]
                idx += 1
                qname_parts.append(data[idx:idx+length].decode('utf-8', errors='ignore'))
                idx += length
            qname = ".".join(qname_parts).lower()

            if qname in valid_mdns:
                resp_domain = qname if '.' in qname else qname + '.local'
                encoded_name = encode_dns_name(resp_domain)
                
                resp = bytearray()
                resp.extend(data[:2]) # Transaction ID
                resp.extend(b"\x84\x00") # Authoritative answer, no error
                resp.extend(b"\x00\x00\x00\x01\x00\x00\x00\x00") # 1 answer
                resp.extend(encoded_name)
                # Type A (1), Class IN with flush bit (0x8001), TTL 120s, Len 4, IP
                resp.extend(b"\x00\x01\x80\x01\x00\x00\x00\x78\x00\x04")
                resp.extend(IP_BYTES)
                
                # Send to multicast group
                sock.sendto(resp, ("224.0.0.251", 5353))
                # Also send direct unicast to querying device
                try:
                    sock.sendto(resp, addr)
                except Exception:
                    pass
                print(f"[mDNS] Answered query '{qname}' from {addr[0]} -> {TARGET_IP}")
    except Exception as e:
        print(f"[mDNS] Error: {e}", file=sys.stderr)

if __name__ == "__main__":
    if hasattr(sys.stdout, 'reconfigure'):
        sys.stdout.reconfigure(line_buffering=True)
    t1 = threading.Thread(target=run_llmnr, daemon=True)
    t2 = threading.Thread(target=run_nbns, daemon=True)
    t3 = threading.Thread(target=run_mdns, daemon=True)
    t1.start()
    t2.start()
    t3.start()
    print(f"[Nexus & Galaxy-A6 Name Server] Active on {TARGET_IP} for: {TARGET_NAMES}")
    t1.join()
