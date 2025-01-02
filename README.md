# xf_ymodem: 基于 XFusion 的 ymodem 模块

xf_ymodem 是基于 XFusion 接口的 ymodem 模块，已在 esp32 和 ws63 上测试，连续传输超过 20MB 的大文件均正常收发。

## 支持情况

支持:

- 自定义 IO 接口。
- 更小的 RAM 占用。
- 接收文件时自定义文件解析，发送文件时自定义填充文件信息。
- 取消传输。
- 错误重传。
- 非标包长 2K, 4K, 8K.
- 已测试超过 20MB 的大文件，传输正常，MD5 校验正确。

不支持：

- 一次连续传输多个文件。

## 使用方法

打开 `xf_ymodem/example`, 导出 XFusion 环境，使用 `xf menuconfig` 配置：

1.  使用发送端或接收端实例；
1.  串口号、波特率、引脚号。

之后使用 `xf build` 编译并烧录。

在 pc 上打开支持 Ymodem 协议的上位机(xshell)，根据前面的配置连接传输串口（不是调试串口）

其他详情见：[快速入门 | XFusion](https://www.coral-zone.cc/#/document?path=/document/)

### 关于 xshell 的注意事项

1.  连接传输串口（不是调试串口）。
    esp32 上默认 TX_PIN: 17, RX_PIN: 16.
    ws63 上默认 TX_PIN: 15, RX_PIN: 16.
    波特率默认 912600.
1.  串口不要开流控。
1.  会话属性 -> 文件传输 -> X/YMODEM
    1.  分组大小选 1024 bytes.
    1.  上传命令里的内容全部删除。
1.  接收或发送文件：
    1.  文件 -> 传输 -> YMODEM -> 用 YMODEM 发送。
    1.  文件 -> 传输 -> YMODEM -> 用 YMODEM 接收。
1.  对于 xshell 发送文件，单片机接收文件时，在接收完毕文件后单片机需要多发一个 'O', xshell 才会认为传输结束。

## 示例简介

### xf_ymodem_example_receiver

使用 xf_ymodem 接收文件并计算 MD5, 由于不保存数据，所以上位机发多长的数据都可以.

日志参考（开了调试）：

```
				C
SOH 00 FF Data[128] CRC CRC
				ACK
I (10383)-receiver: file_name:    1-1234567890_(2).md
I (10386)-receiver: file_len:     2050
				C
STX 01 FE Data[1024] CRC CRC
I (10408)-receiver: progress:     1024/2050
				ACK
STX 02 FD Data[1024] CRC CRC
I (10423)-receiver: progress:     2048/2050
				ACK
STX 03 FC Data[1024] CRC CRC
I (10444)-receiver: progress:     2050/2050
				ACK
EOT
				NAK
EOT
				ACK
				C
SOH 00 FF Data[128] CRC CRC
				ACK
				O
I (10493)-receiver: All received.
I (10496)-receiver: Computed MD5 hash: e41d09e17d5c8b4c746ddc5ed85525d1
```

### xf_ymodem_example_sender

使用 xf_ymodem 发送名为 `1-1234567890.md` 的文件并计算 MD5. 示例使用了 `app_cyclic_fill_buffer()` 循环读取 `sc_file_data`, 因此可以发送超过 `sc_file_data` 长度的数据。

日志参考（开了调试）：

```
xf_ymodem[xf_ymodem.c:622(xf_ymodem_send_handshake)]: no data
				C
SOH 00 FF Data[128] CRC CRC
				ACK
				C
I (19848)-sender: progress:     1024/2050
STX 01 FE Data[1024] CRC CRC
				ACK
I (19877)-sender: progress:     2048/2050
STX 02 FD Data[1024] CRC CRC
				ACK
I (19905)-sender: progress:     2050/2050
SOH 03 FC Data[128] CRC CRC
				ACK
EOT
				NAK
EOT
				ACK
				C
SOH 00 FF Data[128] CRC CRC
				ACK
I (19990)-sender: All have been sent.
I (19990)-sender: Computed MD5 hash: e41d09e17d5c8b4c746ddc5ed85525d1
```

## 对接示例

见 `xf_ymodem/example/main/xf_ymodem_example_receiver.c` 和 `xf_ymodem/example/main/xf_ymodem_example_sender.c`.

不使用 XFusion 的配置菜单时，注意添加 `xf_ymodem_config.h` 文件，以供 `xf_ymodem_config_internel.h` 使用。
如 `xf_ymodem/config/xf_ymodem_config.h`。

## 参考资料

[Ymodem 传输协议 - 树·哥 - 博客园](https://www.cnblogs.com/zzssdd2/p/15418778.html)
[Ymodem 协议说明 - 一月一星辰 - 博客园](https://www.cnblogs.com/tangwc/p/18276156)
[YModem: YModem 协议原版、中文译制版 与 C# 编写的 YModem 通用读写程序](https://gitee.com/miuser00/ymodemwin)
[STM32 Ymodem 协议及代码解析-CSDN 博客](https://blog.csdn.net/u012993936/article/details/125102816)
[纯 C 实现的 ymodem 库，无额外依赖-CSDN 博客](https://blog.csdn.net/qq_40824852/article/details/139661772)
