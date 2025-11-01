
Shoe Tag LF Field Front-End (Rev A)
-----------------------------------
KiCad 6/7 project files: schematic and PCB for a single-sided SMT front-end.
U1 MCP6001 precision rectifier (gain ~20)
U2 TLV3691 comparator with hysteresis
D1 BAT54S dual Schottky for rectifier
Envelope: 10k/10uF -> comparator
Reference: ~0.25–0.35 V via divider; 1M hysteresis
Connectors: J2 (JST-SH 1x2) to spiral coil; J1 (JST-SH 1x4) to Seeed XIAO
CRES footprint for resonating the shoe spiral at ~20 kHz (value depends on measured L)

Suggested J1 pinout (to XIAO):
1: 3V3
2: GND
3: IN_ZONE (comparator OUT) -> GPIO
4: ENV (optional analog envelope) -> ADC

Tune VREF and RHYS to get clean in/out. No ground pour under the spiral.
