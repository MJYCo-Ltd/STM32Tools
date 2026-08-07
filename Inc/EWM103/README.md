# EWM103-W15 (Inc)

用途
- EWM103-W15 是一种常见的 WiFi AT 模组，本目录下提供 Pack/Unpack 门面，帮助应用构造 AT 指令并解析模组响应。

主要接口
- `EWM103_Pack(content, packet, size, &len)` — 按类型构造待发送字符串（以 CRLF 结尾）。
- `EWM103_Unpack(packet, expect, &data)` — 将接收缓冲解析为结构化结果。
- `EWM103_IsComplete(packet, expect)` — 判断是否接收到最终行（如 `OK` / `ERROR`）。

使用建议
- Pack/Unpack 只负责文本的构建与解析，不直接操作 UART；将其与 `UartReceive` 组合使用以保证接收可靠性。
- 由于模组按行返回，解析逻辑以换行分割为主，注意处理粘包/半包场景。

参考
- EWM103-W15 AT 指令手册 V1.1
