# Shoe Tag LF Front-End — Atopile Project (Rev C)

This revision removes external dependencies to avoid `ato.yaml` schema issues.
Everything is defined locally (including JST-SH connectors).

## Build
```
ato build
```
(If your Atopile suggests a different command than `install`, skip it; this project has no external deps.)

## Notes
- J1: JST-SH 1x4 vertical (to XIAO): 1=3V3, 2=GND, 3=IN_ZONE, 4=ENV
- J2: JST-SH 1x2 vertical (to shoe spiral)
- Tune CRES (1210 C0G) after measuring your spiral inductance (target ~20 kHz).
