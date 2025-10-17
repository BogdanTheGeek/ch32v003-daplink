# DAPLink
DAPLink (CMSIS-DAP) V1 ported to CH32V003 using [ch32fun](https://github.com/cnlohr/ch32fun) and [rv003usb](https://github.com/cnlohr/rv003usb) for USB support.

> [!WARNING]
> Only SWD is supported

### Pin map
|  FUNC    | Pin   |
|  :----   | :---- |
| SWD_CLK  | PC.2  |
| SWD_DIO  | PC.1  |

## Speed Test

**test result**
| probe           | pyocd print                                                                                                   |
| ----            | ----                                                                                                          |
| CH32V003 HID    | Erased 8192 bytes (2 sectors), programmed 5888 bytes (46 pages), skipped 0 bytes (0 pages) at 0.47 kB/s       |

# openOCD
> [!NOTE]
> Make sure you build openOCD from source and have hidapi support enabled:

```sh
CMSIS-DAP v1 compliant dongle (HID)              yes (auto)
```

```sh
openocd -f ./v003-dap.cfg -f target/stm32f0x.cfg
```

# Resources
- [DAPLink Firmware](https://github.com/XIVN1987/DAPLink)
- [Making my own Programmer/Debugger using ARM SWD](https://qcentlabs.com/posts/swd_banger/)
