# Show Deck ESP-NOW protocol v1

Show Deck broadcasts a fixed-size packed message. Receivers ignore frames whose
32-bit magic value is not `0x54494B49` (`TIKI`).

| Field | Type | Meaning |
| --- | --- | --- |
| `magic` | `uint32_t` | Protocol marker |
| `sequence` | `uint16_t` | Incrementing message number |
| `source` | `char[16]` | Deck name |
| `target` | `char[24]` | User-configured group or receiver name |
| `payload` | `char[64]` | User-configured command |

Example button configuration:

```json
{
  "label": "Candlelight",
  "color": "#C87828",
  "type": "espnow",
  "target": "all_candles",
  "payload": "preset:3"
}
```

This first version uses broadcast messages without acknowledgement. Pairing,
delivery acknowledgement, retry and optional encryption belong to the next
reliability milestone.

ESP-NOW and infrastructure Wi-Fi share one radio. When the deck joins an access
point, receivers must use that access point's 2.4 GHz channel. In offline mode,
all devices use the fixed channel saved in the deck configuration.
