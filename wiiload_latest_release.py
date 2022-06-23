#!/usr/bin/python
import json
import os
import sys
import zlib
import socket
import struct
import urllib.request


ip = "10.0.27.155"


def main():
    with urllib.request.urlopen("https://api.github.com/repos/wchill/WupNetworkInstaller/releases/latest") as response:
        body = json.loads(response.read().decode("utf-8"))
        rpx_url = body["assets"][0]["browser_download_url"]

    with urllib.request.urlopen(rpx_url) as response:
        rpx_data = response.read()

    wii_ip = (ip, 4299)
    
    WIILOAD_VERSION_MAJOR=0
    WIILOAD_VERSION_MINOR=5
    
    c_data = zlib.compress(rpx_data, 6)
    
    chunk_size = 1024*128
    chunks = [c_data[i:i+chunk_size] for i in range(0, len(c_data), chunk_size)]
    
    # args = [os.path.basename(filename)]+sys.argv[2:]
    # args = b"\x00".join(arg.encode("utf-8") for arg in args) + b"\x00"
    args = b'\x00'
    
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(wii_ip)
    
    s.send(b"HAXX")
    s.send(struct.pack("B", WIILOAD_VERSION_MAJOR)) # one byte, unsigned
    s.send(struct.pack("B", WIILOAD_VERSION_MINOR)) # one byte, unsigned
    s.send(struct.pack(">H",len(args))) # bigendian, 2 bytes, unsigned
    s.send(struct.pack(">L",len(c_data))) # bigendian, 4 bytes, unsigned
    s.send(struct.pack(">L",len(rpx_data))) # bigendian, 4 bytes, unsigned
    
    print(len(chunks),"chunks to send")
    for piece in chunks:
        s.send(piece)
        sys.stdout.write(".")
        sys.stdout.flush()
    sys.stdout.write("\n")
    
    s.send(args)
    
    s.close()
    print("done")

if __name__ == "__main__":
    main()
