# EWM103-W15 (Inc)

用途
- EWM103-W15 WiFi AT 模组的 Pack/Unpack 门面，帮助应用构造 AT 指令并解析模组响应。
- 不操作 UART；与 `UartReceive` + 应用会话层（如 Agriculture `WifiLayer`）组合使用。

主要接口
- `EWM103_Pack(content, packet, size, &len)` — 按类型构造待发送字符串（以 CRLF 结尾）。
- `EWM103_Unpack(packet, expect, &data)` — 将接收缓冲解析为结构化结果。
- `EWM103_IsComplete(packet, expect)` — 判断是否接收到最终行（如 `OK` / `ERROR`）。
- `EWM103_TypeName(type)` — 类型名字符串。

BluFi 相关类型（当前 Agriculture 配网使用）
- `EWM103_TYPE_BLEINIT` → `AT+BLEINIT=<0|1>`（`content.mode`）
- `EWM103_TYPE_BLUFI` → `AT+BLUFI=<0|1>`（`content.mode`）
- 查询类：`content.query != 0` 时组 `AT+NAME?`（如 `CWJAP?`、`CWMODE?`）
- 说明：本库**不**提供 `BLEPAIR*`；Agriculture 走 ESP BluFi，见仓库  
  `Agriculture/Hardware/docs/WIFI_BLUFI_PROVISION.md`

使用建议
- Pack/Unpack 只负责文本的构建与解析；注意粘包/半包，由上层会话缓冲拼完整响应。
- 最终结果判定依赖 `AT/at_codec`：`AT_HasFinalResult` 优先整行匹配 `OK`/`ERROR`，并对独立 `OK` token 做兜底（避免子串误命中）。

参考
- EWM103-W15 AT 指令手册 V1.1
- `Inc/README.md` 中 EWM103 / AT codec 小节
