# MultiUser-secondFileSystem
- 同济大学操作系统课程设计（邓蓉）
- 基于Unix v6++源码移植的多用户二级文件系统
- 带有缓存层的文件系统
- 支持多用户同时使用的文件系统
- Linux操作系统
## 使用教程
1. 运行server
- 删除 c.img 文件（可选）
- 在 MultiUser-secondFileSytem 目录中运行 `make`
- 在 MultiUser-secondFileSytem 目录中运行 `./secondFileSystem`
2. 运行client
- 在client目录中运行 `make`
- 在client目录中运行 `./clinet 127.0.0.1 1235`
- 输入用户名
