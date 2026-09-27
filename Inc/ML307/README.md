# ML307 (Inc)

`ml307_http.h` 提供 ML307C HTTP/HTTPS 缓存模式的有界命令构造与解析，
HTTPS 实例通过 `ML307_HttpBuildSsl` 绑定 SSL context。OTA 应按 HTTP Range
小片读取并流式写入 Flash；`ML307_HttpParseRead` 使用显式字节
长度解析，因此固件中的 NUL、逗号和换行不会被误认为协议分隔符。

`ml307_ssl.h` 提供 ML307C TLS context 的安全参数设置/查询、CA 绑定、
证书分片写入与定长读回，以及模块 `CCLK` 设置/查询。安全设置只生成
`auth=1`、TLS 1.2、`ignorestamp=0`、`ignoreverify=0`、`encoding=2`；
ML307C 不支持的 SNI 配置不在 API 中。证书读回以声明长度划分 payload，
payload 内的 `OK`、`ERROR` 或 URC 都不作为控制结果。解析器返回
`INCOMPLETE` 时调用方必须保留原缓冲并追加数据；只有唯一匹配响应且末尾
最终结果为 `OK` 才返回 `COMPLETE`。

`ML307_SslTransaction*` 在驱动内固定 CA 写入提示符/精确 RAW payload/最终
结果/完整读回，以及 TLS 参数 set/query、CA 引用、UTC `CCLK` 和 MQTT SSL
绑定 set/query 的顺序。事务本身不分配内存、不拥有 UART 或计时器；上层
通过 `GetAction`、`ActionSent`、`InspectResponse`/`ConsumeResponse` 提供
串行发送、累计接收和单调超时。`ML307_MqttParseSslQuery` 按手册接受查询
响应省略 connect id 的格式，但若存在命令回显则必须精确匹配请求 id。

这些接口只证明命令已被正确构造或模组响应已被严格解析，不证明服务器
证书 CN/SAN、TLS 握手、Broker 连接或现场链路已经验证，也不提供可永久
保持的 READY 状态。上层仍需按实际连接生命周期管理状态与失败恢复。
`CCLK` 接口同样只处理命令与当前响应，不声明该时钟设置可掉电保持。

用途
- ML307 是中移物联网的 4G 模组，仓库中实现了针对常用 AT 指令的 Pack/Unpack 门面，方便上层业务按类型构建与解析 AT 响应。

主要接口（在 `Inc/ML307` 下头文件）
- `ML307_Pack(content, packet, size, &len)` — 根据类型与内容生成发送包（应用侧把生成的 packet 通过 UART 发送）
- `ML307_Unpack(packet, expect, &data)` — 将接收到的包解析为结构化结果
- `ML307_IsComplete(packet, expect, id)` — 判断当前接收缓冲是否包含完整最终结果（例如 `OK` / `ERROR` 行）
- `ML307_TypeName(type)` — 类型名字符串，用于日志打印

典型使用流程
1. 应用调用 `ML307_Pack` 生成 AT 指令（或根据类型从配置中读取）并通过串口发送。
2. 使用 `UartReceive` 模块接收串口数据并入队处理。
3. 将接收到的数据交给 `ML307_Unpack`/`ML307_IsComplete` 以判断命令是否完成并解析结果。

注意事项
- ML307 模块的 Pack/Unpack **不直接操作 UART**，只负责数据结构的构建和解析；UART 发送/接收由应用或 `UartReceive` 完成。
- 多并发命令：应用需自己管理命令 ID 与超时，以便把收到的数据关联到正确的请求。

参考
- 手册：AT Commands Reference Guide 4G Series V2.0.5（仓库示例假设遵循该手册）
