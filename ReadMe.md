# IBSSChat
## ©️ 2018 Claes M Nyberg, cmn@fuzzpoint.com

---
This is the implementation that was part of my thesis at NPS.  
It is a fully working implementation of a stand alone multi-hop capable server-less ad-hoc chat protocol.   
It runs as root from a single binary installed on a Android device.  

One interesting approach from my point of view, is that I run the daemon to listen on localhost and provide  
a protocol that allows a regular user to connect and execute commands as root.  
With this setupt, I simple ran system calls as root to avoid getting blocked by SELinux on the devices I was testing

