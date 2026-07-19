#!/usr/bin/env python3
"""RadioMesh Zephyr serial helper: discover boards, identify roles, monitor.

Standard library only — no third-party dependencies (no pyserial), so it runs
under any system python3 without a venv.

The XIAO ESP32-S3 exposes the SoC's native USB. The running Zephyr firmware uses
a USB-Serial-JTAG console that (a) discards TX until a host attaches (~1.5 s after
power-up) and (b) drops the port across a reboot. So a naive `cat`/`screen` misses
boot output and dies on the post-flash replug. This tool reconnects across that
gap and can tell you which firmware role each connected board is running.

Commands:
  list                 print connected board serial ports, one per line
  resolve [--port P]   print exactly ONE port to use; error if zero or ambiguous
  boards               print each connected port and the firmware role it runs
  monitor [--port P]   reconnecting serial console (Ctrl-C to stop)

Role is read from the Zephyr LOG_MODULE name in the console stream:
  rm_tx -> tx     rm_rx -> rx     rm_m1 -> legacy-symmetric
"""
import argparse
import glob
import os
import platform
import select
import sys
import termios
import time

BAUD = 115200

# LOG_MODULE_REGISTER name -> human role. Extend as new example roles land.
ROLE_TAGS = {
    "rm_tx": "tx",
    "rm_rx": "rx",
    "rm_m1": "legacy-symmetric",
}


def port_glob():
    """Serial device glob for the board's native USB, per host OS."""
    if platform.system() == "Darwin":
        return "/dev/cu.usbmodem*"
    return "/dev/ttyACM*"  # Linux; ttyUSB* is for external UART bridges, not this


def list_ports():
    return sorted(glob.glob(port_glob()))


def resolve_port(explicit):
    """Return one port path, or None (with a message on stderr) if ambiguous."""
    if explicit and explicit != "auto":
        return explicit
    ports = list_ports()
    if len(ports) == 1:
        return ports[0]
    if not ports:
        sys.stderr.write("error: no boards connected (%s)\n" % port_glob())
    else:
        sys.stderr.write(
            "error: %d boards connected; pass PORT=<path> to pick one:\n  %s\n"
            % (len(ports), "\n  ".join(ports)))
    return None


def serial_open(port):
    """Open a serial port in raw 8N1 at BAUD using termios (stdlib only)."""
    fd = os.open(port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    attrs = termios.tcgetattr(fd)  # [iflag, oflag, cflag, lflag, ispeed, ospeed, cc]
    attrs[0] = 0  # iflag: no input processing
    attrs[1] = 0  # oflag: no output processing
    attrs[3] = 0  # lflag: non-canonical, no echo
    attrs[2] = ((attrs[2] & ~termios.CSIZE & ~termios.PARENB & ~termios.CSTOPB)
                | termios.CS8 | termios.CREAD | termios.CLOCAL)
    attrs[4] = attrs[5] = getattr(termios, "B%d" % BAUD)
    cc = list(attrs[6])
    cc[termios.VMIN] = 0
    cc[termios.VTIME] = 0
    attrs[6] = cc
    termios.tcsetattr(fd, termios.TCSANOW, attrs)
    return fd


def serial_read(fd, timeout=0.2):
    """Read available bytes; b'' on timeout. Raises OSError on disconnect."""
    ready, _, _ = select.select([fd], [], [], timeout)
    if not ready:
        return b""
    data = os.read(fd, 4096)
    if data == b"":
        raise OSError("device closed")  # readable but empty => port went away
    return data


def sample_role(port, seconds=8.0):
    """Read up to `seconds` of console output and classify the firmware role."""
    deadline = time.monotonic() + seconds
    buf = b""
    fd = None
    try:
        while time.monotonic() < deadline:
            if fd is None:
                try:
                    fd = serial_open(port)
                except OSError:
                    time.sleep(0.2)
                    continue
            try:
                buf += serial_read(fd)
            except OSError:
                os.close(fd)
                fd = None
                continue
            for tag, role in ROLE_TAGS.items():
                if tag.encode() in buf:
                    return role
    finally:
        if fd is not None:
            try:
                os.close(fd)
            except OSError:
                pass
    return "unknown (no role output in %gs)" % seconds


def cmd_list():
    ports = list_ports()
    if not ports:
        sys.stderr.write("no boards found (%s)\n" % port_glob())
        return 1
    print("\n".join(ports))
    return 0


def cmd_resolve(port_arg):
    port = resolve_port(port_arg)
    if not port:
        return 1
    print(port)
    return 0


def cmd_boards():
    ports = list_ports()
    if not ports:
        sys.stderr.write("no boards found (%s)\n" % port_glob())
        return 1
    sys.stderr.write("Sampling %d board(s), up to ~8 s each...\n" % len(ports))
    width = max(len(p) for p in ports)
    for port in ports:
        print("%-*s  %s" % (width, port, sample_role(port)))
    return 0


def cmd_monitor(port_arg):
    port = resolve_port(port_arg)
    if not port:
        return 1
    auto = port_arg in (None, "auto")
    sys.stderr.write("[rmon] monitoring %s @ %d (Ctrl-C to stop)\n" % (port, BAUD))
    fd = None
    try:
        while True:
            if fd is None:
                # The port can vanish across a reboot/replug; keep retrying, and
                # in auto mode re-resolve in case the device node was renumbered.
                if auto:
                    found = list_ports()
                    if len(found) == 1:
                        port = found[0]
                try:
                    fd = serial_open(port)
                    sys.stderr.write("[rmon] opened %s\n" % port)
                except OSError:
                    time.sleep(0.3)
                    continue
            try:
                data = serial_read(fd)
                if data:
                    sys.stdout.buffer.write(data)
                    sys.stdout.flush()
            except OSError:
                try:
                    os.close(fd)
                except OSError:
                    pass
                fd = None
                time.sleep(0.3)
    except KeyboardInterrupt:
        sys.stderr.write("\n[rmon] stopped\n")
    finally:
        if fd is not None:
            try:
                os.close(fd)
            except OSError:
                pass
    return 0


def main():
    ap = argparse.ArgumentParser(prog="rmon",
                                 description="RadioMesh Zephyr serial helper")
    sub = ap.add_subparsers(dest="cmd", required=True)
    sub.add_parser("list", help="print connected board ports")
    p_res = sub.add_parser("resolve", help="print one port to use (errors if ambiguous)")
    p_res.add_argument("--port", default="auto")
    sub.add_parser("boards", help="print each port and its firmware role")
    p_mon = sub.add_parser("monitor", help="reconnecting serial monitor")
    p_mon.add_argument("--port", default="auto")
    args = ap.parse_args()
    if args.cmd == "list":
        return cmd_list()
    if args.cmd == "resolve":
        return cmd_resolve(args.port)
    if args.cmd == "boards":
        return cmd_boards()
    return cmd_monitor(args.port)  # "monitor"


if __name__ == "__main__":
    sys.exit(main())
