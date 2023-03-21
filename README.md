# TcpForward 流量转发工具

<br>

<div align=center>

![image](https://user-images.githubusercontent.com/52789403/226499917-86beab49-131b-4d78-805f-07e9c40a6364.png)

[![Build status](https://cdn.lyshark.com/archive/LyScript/build.svg)](https://github.com/lyshark/LyMemory) [![Crowdin](https://cdn.lyshark.com/archive/LyScript/email.svg)](mailto:me@lyshark.com)  [![OSCS Status](https://cdn.lyshark.com/archive/LyScript/OSCS.svg)](https://www.lyshark.com)

</div>

<br>

一款基于命令行实现的功能强大的TCP流量转发工具，可用于后渗透过程中横向移动。它能够定向转发数据包，打破内外网的限制，使用户能够攻击内网中的特定主机或将数据包中转到外网。此工具仅占用19KB的空间，非常小巧且没有任何明显的特征，是一款非常有用的渗透测试辅助工具。

### 工具简介

工具提供三种转发模式,`正向隧道转发模式`,`反向纯流量隧道模式`,`双向隧道转发模式`，这三种转发模式通常是在网络架构设计和部署中使用的，它们的使用场景各不相同，可以根据实际需要选择适合的转发模式。下面是对三种转发模式的详细描述：

 - 正向隧道转发模式

正向隧道转发模式是一种将私有网络中的数据通过公网隧道转发到另一个私有网络的方式。在这种模式下，发送方将数据发送到本地隧道接口，该接口将数据封装为隧道协议，并将其发送到公网上的远程隧道接口，然后远程隧道接口将数据解封装并转发到目标私有网络中。这种模式通常用于远程访问私有网络资源，如 VPN 连接。

 - 反向纯流量隧道模式
 
 反向纯流量隧道模式是一种将公网上的数据通过隧道转发到私有网络的方式。在这种模式下，接收方先将要接收的数据发送到公网上的远程隧道接口，接着本地隧道接口接收到数据后进行解封装并转发到目标私有网络中。这种模式通常用于提供公网上的服务，例如 Web 服务器、邮件服务器等。
 
 - 双向隧道转发模式
 
 双向隧道转发模式是一种可以同时支持正向和反向转发的方式。在这种模式下，本地和远程隧道接口都可以发送和接收数据，因此可以同时进行双向通信。这种模式通常用于需要同时支持远程访问和公网服务的场景，例如企业网络和云服务之间的连接。
 
命令行帮助文档如下：

| 参数名 | 描述 |
| --- | --- |
| ListenPort | 指定转发服务器侦听端口 |
| RemoteAddress | 指定对端IP地址 |
| RemotePort | 指定对端端口号 |
| LocalPort | 指定本端端口号 |
| ServerAddress | 指定服务端IP地址 |
| ServerPort | 指定服务器Port端口号 |
| ConnectAddress | 指定连接到对端IP地址 |
| ConnectPort | 指定对端连接Port端口号 |

总之，选择适当的转发模式取决于应用场景和需求。正向隧道转发模式用于访问远程私有网络资源，反向纯流量隧道模式用于提供公网服务，而双向隧道转发模式则可以同时支持这两种用途。

### 使用方式

 - 正向隧道转发模式
 
当处于正向隧道模式下，用户访问本机的`8888`端口则流量将被转发到远端的`8.141.58.64:22`地址上，此时如果使用`SSH`连接本机的`8888`端口相当于连接了`8.141.58.64`的`22`号端口。
 ```C
Shell> TcpForward.exe Forward --ListenPort 8888 --RemoteAddress 8.141.58.64 --RemotePort 22

[*] 正向隧道模式
[+] 本机侦听端口: 8888
[+] 流量转发地址 8.141.58.64:22

Shell> ssh root@localhost -p 8888
root@localhost's password:
 ```
 
  - 反向纯流量隧道模式

当处于反向纯流量隧道模式下用户需要做两件事，服务端需要在本机运行，客户端需要在内网中的一台主机上运行。

服务端运行侦听命令，执行后本地将侦听`9999`端口等待客户端连接。
```C
Shell> TcpForward.exe ReverseServer --ListenPort 9999 --LocalPort 8888

[*] 反向纯流量隧道模式 (服务端)
[+] 侦听端口: 9999
[+] 本机连接地址: localhost:8888
```
客户端运行反弹命令，其中`ServerAddress:ServerPort`用于指定服务端地址以及端口号，其中`ConnectAddress:ConnectPort`则是内网中其他主机的IP地址。
```C
Shell> TcpForward.exe ReverseClient --ServerAddress 127.0.0.1 --ServerPort 9999 \
--ConnectAddress 8.141.58.64 --ConnectPort 22

[*] 反向纯流量隧道模式 (客户端)
[+] 服务端地址 127.0.0.1:9999
[+] 连接内网地址 8.141.58.64:22
```
如果你有幸获得了内网一台主机的控制权，而你想攻击其他主机，当你建立了如上隧道，攻击本机的`127.0.0.1:8888`则相当于在攻击内网中的`8.141.58.64:22`这个地址，其实是在变相的攻击，如上客户端执行后，服务端连接本地`8888`端口，则实际连接到了`8.141.58.64:22`端口上。
```C
Shell> ssh root@localhost -p 8888
root@localhost's password:
```

 - 双向隧道转发模式

当处于双向隧道转发模式下用户需要做两件事，服务端需要在本机运行，客户端需要在内网中的一台主机上运行。

不同于`反向纯流量隧道模式`此模式主要用于连接带有页面的服务，例如连接远程的`3389`远程桌面，这类流量需要更加精细的控制，所以需要使用本隧道完成。

服务端侦听地址。
```C
Shell> TcpForward.exe TwoForwardServer --ListenPort 9999 --LocalPort 8888

[*] 双向隧道转发模式 (服务端)
[+] 侦听端口: 9999
[+] 本机连接地址: localhost:8888
```

客户端执行如下命令,主动连接服务端`127.0.0.1:9999`端口，连接成功后转发`8.141.58.64:3389`的流量到服务端。
```C
Shell> TcpForward.exe TwoForwardClient --ServerAddress 127.0.0.1 --ServerPort 9999 \
--ConnectAddress 8.141.58.64 --ConnectPort 3389

[*] 双向隧道转发模式 (客户端)
[+] 服务端地址 127.0.0.1:9999
[+] 连接内网地址 8.141.58.64:3389
```
此时通过远程协助连接本机的`localhost:8888`端口则相当于连接了内网主机`8.141.58.64:3389`端口，实现直接访问。

### 项目地址

https://github.com/lyshark/TcpForward
