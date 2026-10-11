## Reading the pulse times on the OPTICAL rotary

```
223 << Dialed 1
2192 <- Dial returning
255 63 << Dialed 2
1541 <- Dial returning
281 63 64  << Dialed 3
1905 <- Dial returning
230 62 63 63 << Dialed 4
3563 <- Dial returning
235 63 64 63 64 << Dialed 5
1742 <- Dial returning
222 63 63 65 64 64 << Dialed 6
1257 <- Dial returning
164 65 64 63 65 64 65 << Dialed 7
1256 <- Dial returning
150 64 66 65 65 66 65 65 << Dialed 8
1081 <- Dial returning
140 65 65 67 65 65 66 65 65 << Dialed 9
933 <- Dial returning
141 62 65 65 66 66 64 65 65 64 << Dialed 0
```

These time measurements were taken with the `pulse_timing.ino` code.

Note that the first pulse has a longer interval, between 223 and 141ms. Filtering the pulses with 60 alone is not enough: the value has to be between 60 and 225 to be detected, because the interval between the dead zone and the first pulse is 223 in the shortest-distance scenario. Since the mechanical system is spring-based, it is natural that the greater the distance travelled by the disc, the more torque it has on the return, and therefore the less time it takes.
