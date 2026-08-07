# ML307 (Inc)

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
