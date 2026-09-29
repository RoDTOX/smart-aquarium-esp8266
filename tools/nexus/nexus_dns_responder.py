#!/usr/bin/env python3
"""
Nexus Local Name Responder (LLMNR + NetBIOS + mDNS)
Enables seamless resolution of 'http://nexus/' and 'http://nexus.local/'
on Windows, macOS, Linux, and Android devices across the local Wi-Fi.
Target IP: 192.168.1.28
"""

import socket
import struct
import threading
import sys

TARGET_NAME = "nexus"
TARGET_IP = "192.168.1.28"
IP_BYTES = socket.inet_aton(TARGET_IP)

def encode_nbname(name):
    padded = name.upper().ljust(16)[:16]
    return b"".join(bytes([(ord(c) >> 4) + ord('A'), (ord(c) & 0xF) + ord('A')]) for c in padded)

ENCODED_NBNAME = encode_nbname(TARGET_NAME)

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
        print("[LLMNR] Listening on 224.0.0.252:5355 for 'nexus'...")

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

            if qname == TARGET_NAME and qtype in (1, 255): # A or ANY
                # Build LLMNR response
                # Transaction ID (2) + Flags 0x8000 (2) + QDCOUNT 1 (2) + ANCOUNT 1 (2) + NSCOUNT 0 (2) + ARCOUNT 0 (2)
                resp = bytearray()
                resp.extend(tx_id)
                resp.extend(b"\x80\x00") # Response, No error
                resp.extend(b"\x00\x01\x00\x01\x00\x00\x00\x00")
                # Question section copy
                resp.extend(data[12:idx+4])
                # Answer: name pointer 0xc00c, type A (1), class IN (1), TTL (30s), len 4, IP
                resp.extend(b"\xc0\x0c\x00\x01\x00\x01\x00\x00\x00\x1e\x00\x04")
                resp.extend(IP_BYTES)
                sock.sendto(resp, addr)
                print(f"[LLMNR] Answered query from {addr[0]} -> {TARGET_IP}")
    except Exception as e:
        print(f"[LLMNR] Error: {e}", file=sys.stderr)

# 2. NetBIOS Name Service Responder (UDP 137) - Legacy Windows resolution
def run_nbns():
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        sock.bind(('', 137))
        print("[NBNS] Listening on UDP 137 for 'NEXUS'...")

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
            if raw_nbname == ENCODED_NBNAME:
                # Build NBNS positive response
                resp = bytearray()
                resp.extend(tx_id)
                resp.extend(b"\x85\x00") # Response, Authoritative, Recursion Available
                resp.extend(b"\x00\x00\x00\x01\x00\x00\x00\x00") # 0 questions, 1 answer
                # Answer Name
                resp.append(0x20)
                resp.extend(raw_nbname)
                resp.append(0x00)
                # Type NB (0x0020), Class IN (0x0001), TTL 300s (0x0000012c), Data len 6
                resp.extend(b"\x00\x20\x00\x01\x00\x00\x01\x2c\x00\x06")
                # Flags (0x0000 - unique name, B-node) + IP (4)
                resp.extend(b"\x00\x00")
                resp.extend(IP_BYTES)
                sock.sendto(resp, addr)
                print(f"[NBNS] Answered query from {addr[0]} -> {TARGET_IP}")
    except Exception as e:
        print(f"[NBNS] Error: {e}", file=sys.stderr)

# 3. mDNS Responder (UDP 5353, 224.0.0.251) - nexus.local for Mac/iOS/Android
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
        print("[mDNS] Listening on 224.0.0.251:5353 for 'nexus.local'...")

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

            if qname in ("nexus.local", "nexus"):
                # Build mDNS response packet
                resp = bytearray()
                resp.extend(data[:2]) # Transaction ID
                resp.extend(b"\x84\x00") # Authoritative answer, no error
                resp.extend(b"\x00\x00\x00\x01\x00\x00\x00\x00") # 1 answer
                # Answer Name: \x05nexus\x05local\x00
                resp.extend(b"\x05nexus\x05local\x00")
                # Type A (1), Class IN with flush bit (0x8001), TTL 120s, Len 4, IP
                resp.extend(b"\x00\x01\x80\x01\x00\x00\x00\x78\x00\x04")
                resp.extend(IP_BYTES)
                sock.sendto(resp, ("224.0.0.251", 5353))
                print(f"[mDNS] Answered query from {addr[0]} -> {TARGET_IP}")
    except Exception as e:
        print(f"[mDNS] Error: {e}", file=sys.stderr)

if __name__ == "__main__":
    t1 = threading.Thread(target=run_llmnr, daemon=True)
    t2 = threading.Thread(target=run_nbns, daemon=True)
    t3 = threading.Thread(target=run_mdns, daemon=True)
    t1.start()
    t2.start()
    t3.start()
    print("[Nexus Name Server] All responders active (LLMNR, NBNS, mDNS).")
    t1.join()
