# logmgrd 使用说明

## 简介

logmgrd 是一款为 Linux (aarch64) 平台设计的高鲁棒性日志管理守护进程。它能够通过 SOME/IP 协议远程接收日志上传请求，自动归档指定路径下所有日志文件，并上传至配置好的 FTP 服务器根目录，随后通过 SOME/IP 协议返回处理结果信号。logmgrd 以守护进程方式常驻后台运行，直到设备关机。

---

## 主要特性

- 常驻后台自动监听日志上传请求（通过 SOME/IP 协议，demo 版为 TCP 实现）。
- 自动归档指定目录下全部日志文件为 tar.gz 包。
- 自动将归档包上传至 FTP 服务器根目录。
- 上传完成后通过 SOME/IP 返回操作结果信号。
- 支持详细日志记录，异常自动处理，确保高鲁棒性。

---

## 编译方法

依赖：
- gcc
- pthread 库
- 系统需自带 tar、curl 工具

编译指令：

```sh
gcc -Wall -O2 -o logmgrd logmgrd.c -lpthread
```

---

## 配置文件

配置文件默认路径为 `/etc/logmgrd.conf`，格式如下：

```
LOG_PATH=/var/log/mylogs
FTP_HOST=192.168.1.100
FTP_USER=myuser
FTP_PASS=mypass
FTP_ROOT=/
FTP_PORT=21
TAR_PATH=/tmp
LOG_FILE=/var/log/logmgrd.log
```

各参数说明：

- `LOG_PATH`：需要归档的日志目录
- `FTP_HOST`：FTP 服务器地址
- `FTP_USER` / `FTP_PASS`：FTP 登录账号和密码
- `FTP_PORT`：FTP 端口（默认21）
- `FTP_ROOT`：FTP 根路径（一般为`/`，无需修改）
- `TAR_PATH`：归档包临时文件存放路径
- `LOG_FILE`：logmgrd 运行时日志文件路径

---

## 运行方法

后台运行（推荐）：

```sh
sudo ./logmgrd
```

前台调试运行：

```sh
./logmgrd -f
```

---

## 协议说明

- 监听端口：30501（可通过代码修改）
- 客户端通过 SOME/IP（示例实现为 TCP）发送 4 字节上传请求信号（0xAABBCCDD）。
- 归档与上传完成后，logmgrd 通过新建 TCP 连接回客户端，发送 4 字节处理结果信号。

详细信号规范请见[《logmgrd SOME/IP 信号规范》](doc/SOMEIP_SIGNAL_SPEC_CN.md)。

---

## 注意事项

- 部署环境需有 tar 和 curl 命令行工具。
- 建议通过防火墙限制端口访问范围，防止未授权触发。
- 日志文件较大时请预留 /tmp 空间。

---

## 故障排查

- 日志文件详见配置中的 LOG_FILE 路径。
- 归档或上传失败时会有详细错误日志。
- 配置错误或缺失时进程不会启动。

---

## 扩展说明

- 支持后续扩展更多信号类型或自定义归档目录。
- 若需支持完整 SOME/IP 协议，请集成专业 SOME/IP 库。

---