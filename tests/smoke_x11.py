#!/usr/bin/env python3
"""Exercise the real SDL game through XTest; no synthetic frames or game physics.

Requires Xvfb, libX11, libXtst and Pillow. Supply your own original assets.
Output must not already exist. Run each scenario in a fresh profile. This is a
smoke driver, not a Windows-binary equivalence test. Inspect the screenshots.
"""
from __future__ import annotations
import argparse
import ctypes as C
import hashlib
import json
import os
from pathlib import Path
import subprocess
import time
from PIL import Image, ImageGrab

class Driver:
    def __init__(self, display: str):
        self.x = C.CDLL("libX11.so.6")
        self.t = C.CDLL("libXtst.so.6")
        self.x.XOpenDisplay.argtypes = [C.c_char_p]
        self.x.XOpenDisplay.restype = C.c_void_p
        self.x.XStringToKeysym.argtypes = [C.c_char_p]
        self.x.XStringToKeysym.restype = C.c_ulong
        self.x.XKeysymToKeycode.argtypes = [C.c_void_p, C.c_ulong]
        self.x.XKeysymToKeycode.restype = C.c_uint
        self.x.XFlush.argtypes = [C.c_void_p]
        self.x.XCloseDisplay.argtypes = [C.c_void_p]
        self.t.XTestFakeKeyEvent.argtypes = [C.c_void_p, C.c_uint, C.c_int, C.c_ulong]
        self.t.XTestFakeMotionEvent.argtypes = [C.c_void_p, C.c_int, C.c_int, C.c_int, C.c_ulong]
        self.t.XTestFakeButtonEvent.argtypes = [C.c_void_p, C.c_uint, C.c_int, C.c_ulong]
        self.display = self.x.XOpenDisplay(display.encode())
        if not self.display:
            raise RuntimeError(f"Cannot open {display}")
    def key(self, name: str, down: bool):
        code = self.x.XKeysymToKeycode(self.display, self.x.XStringToKeysym(name.encode()))
        if code == 0:
            raise ValueError(f"Unknown keysym: {name}")
        self.t.XTestFakeKeyEvent(self.display, code, int(down), 0)
        self.x.XFlush(self.display)
    def tap(self, name: str, duration: float = 0.10):
        self.key(name, True)
        time.sleep(duration)
        self.key(name, False)
        time.sleep(0.25)
    def text(self, text: str):
        for char in text:
            self.tap(char)
    def click(self, x: int, y: int, button: int = 1):
        # Original 640x480 logical coordinates; test window is scaled by two.
        self.t.XTestFakeMotionEvent(self.display, 0, x * 2, y * 2, 0)
        self.x.XFlush(self.display)
        time.sleep(0.12)
        self.t.XTestFakeButtonEvent(self.display, button, 1, 0)
        self.x.XFlush(self.display)
        time.sleep(0.12)
        self.t.XTestFakeButtonEvent(self.display, button, 0, 0)
        self.x.XFlush(self.display)
        time.sleep(0.25)
    def close(self):
        if self.display:
            self.x.XCloseDisplay(self.display)
            self.display = None

def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--data-dir", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--scenario", choices=["play", "demo", "replay", "external", "editor"], default="play")
    parser.add_argument("--import-state", type=Path, help="Use a copy of existing progress; no assets are changed")
    parser.add_argument("--no-audio", action="store_true")
    parser.add_argument("--seconds", type=float, default=3)
    parser.add_argument("--throttle-seconds", type=float, default=0.8)
    args = parser.parse_args()
    binary = args.binary.resolve(strict=True)
    data = args.data_dir.resolve(strict=True)
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    originals = {str(p.relative_to(data)): sha256(p) for p in data.rglob("*") if p.is_file()}
    game = None
    xvfb = None
    driver = None
    log_path = output / "game.log"
    shots = []
    try:
        read_fd, write_fd = os.pipe()
        xvfb_log = (output / "xvfb.log").open("wb")
        xvfb = subprocess.Popen(["Xvfb", "-displayfd", str(write_fd), "-screen", "0", "1280x960x24", "-nolisten", "tcp"], pass_fds=[write_fd], stdout=xvfb_log, stderr=subprocess.STDOUT)
        os.close(write_fd)
        with os.fdopen(read_fd) as stream:
            display = ":" + stream.readline().strip()
        if display == ":":
            raise RuntimeError("Xvfb did not allocate a display")
        env = dict(os.environ, DISPLAY=display, SDL_AUDIODRIVER="dummy", ASAN_OPTIONS="detect_leaks=0", UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
        capture = output / "current.bmp"
        command = [str(binary), "--data-dir", str(data), "--user-dir", str(output / "profile"), "--no-dialogs", "--no-vsync", "--capture", str(capture), "--quit-after-ms", "90000"]
        if args.no_audio:
            command.append("--no-audio")
        if args.import_state:
            command += ["--import-state", str(args.import_state.resolve(strict=True))]
        (output / "command.json").write_text(json.dumps(command, indent=2) + "\n")
        log = log_path.open("wb")
        game = subprocess.Popen(command, env=env, stdout=log, stderr=subprocess.STDOUT)
        driver = Driver(display)
        def alive():
            if game.poll() is not None:
                raise RuntimeError(f"Game exited with {game.returncode}: {log_path.read_text(errors='replace')}")
        def capture_frame(name: str):
            alive()
            if capture.exists():
                capture.unlink()
            driver.tap("F12", 0.40)
            deadline = time.monotonic() + 10
            while not capture.exists() or capture.stat().st_size < 1280 * 960 * 3:
                alive()
                if time.monotonic() >= deadline:
                    ImageGrab.grab(xdisplay=display).save(output / (name + "-timeout-x11.png"))
                    raise TimeoutError(f"No SDL capture for {name}")
                time.sleep(0.1)
            time.sleep(0.1)
            image = Image.open(capture)
            image.load()
            if image.size != (1280, 960) or len(image.getcolors(1280 * 960) or []) < (3 if args.scenario == "editor" else 8):
                raise RuntimeError(f"Blank/incorrect capture for {name}")
            path = output / (name + ".png")
            image.save(path)
            shots.append({"name": name, "sha256": sha256(path), "size": list(image.size), "method": "SDL_RenderReadPixels -> BMP -> lossless PNG"})
        time.sleep(2)
        capture_frame("intro")
        driver.tap("Return")
        if args.import_state is None:
            time.sleep(0.5)
            driver.text("sdltest")
            driver.tap("Return")
        else:
            # Let the original initialization/menu construction finish and
            # observe key-up before selecting the existing player.
            time.sleep(1.5)
            driver.tap("Return", 0.40)
        time.sleep(0.5)
        capture_frame("main-menu")
        if args.scenario == "play":
            driver.tap("Return")
            driver.tap("Return")
            time.sleep(0.7)
            capture_frame("warm-up-idle")
            driver.tap("space")
            driver.key("Up", True)
            time.sleep(args.throttle_seconds)
            driver.key("Up", False)
            time.sleep(0.6)
            capture_frame("warm-up-playing")
            driver.tap("Right", 0.08)
            driver.tap("Down", 0.25)
            time.sleep(args.seconds)
            capture_frame("after-controls")
            # Exercise page-size toggles and framebuffer reuse.
            driver.tap("minus")
            driver.tap("equal")
            driver.tap("Escape")
            time.sleep(0.6)
            capture_frame("after-play")
        elif args.scenario == "demo":
            driver.tap("Down")
            driver.tap("Down")
            driver.tap("Return")
            time.sleep(args.seconds)
            capture_frame("demo")
            driver.tap("Escape")
        elif args.scenario == "replay":
            driver.tap("Down")
            driver.tap("Return")
            driver.tap("Down")
            driver.tap("Return")
            time.sleep(args.seconds)
            capture_frame("replay")
            driver.tap("Escape")
        elif args.scenario == "external":
            driver.tap("Return")
            driver.tap("Up")
            driver.tap("Return")
            capture_frame("external-selector")
            driver.tap("Return")
            time.sleep(args.seconds)
            capture_frame("external-level")
            driver.tap("Escape")
        elif args.scenario == "editor":
            for _ in range(6):
                driver.tap("Down", 0.25)
            driver.tap("Return")
            time.sleep(args.seconds)
            capture_frame("editor")
            # Does not bypass the original progress gate. Imported profiles must
            # legitimately satisfy it to exercise editing rather than its dialog.
            driver.tap("Return", 0.30)
            time.sleep(0.8)
            capture_frame("editor-loaded")
            # Save a copy through the original Save As dialog, not in-place.
            driver.click(40, 102)
            time.sleep(0.5)
            capture_frame("editor-save-as")
            driver.text("sdltest1")
            # Exercise the full 8.3 filename and backspace at column zero.
            for _ in range(8):
                driver.tap("minus")
            driver.tap("BackSpace")
            driver.tap("Return", 0.30)
            time.sleep(0.8)
            capture_frame("editor-saved")
            saved_levels = [p for p in (output / "profile" / "lev").iterdir() if p.name.lower() == "sdltest1.lev"]
            if len(saved_levels) != 1 or saved_levels[0].stat().st_size < 100:
                raise RuntimeError("Editor Save As did not create the profile-local level")
            driver.click(40, 83)
            time.sleep(0.4)
            driver.tap("Return", 0.30)
            time.sleep(0.8)
            capture_frame("editor-reloaded")
        alive()
        # Use SDL's normal window-close path, not SIGKILL. A timeout is fallback.
        # Xvfb has no window manager; send WM_DELETE_WINDOW through Xlib instead.
        send_close(driver)
        try:
            code = game.wait(timeout=8)
        except subprocess.TimeoutExpired:
            raise RuntimeError("Window-close did not exit (possibly an unsaved editor dialog)")
        log.close()
        text = log_path.read_text(errors="replace")
        if code != 0 or "Clean exit:" not in text or "runtime error:" in text or "AddressSanitizer:" in text:
            raise RuntimeError(f"Failed/sanitizer run: {log_path}")
        after = {str(p.relative_to(data)): sha256(p) for p in data.rglob("*") if p.is_file()}
        if originals != after:
            raise RuntimeError("Original data changed during smoke test")
        result = {"scenario": args.scenario, "binary_sha256": sha256(binary), "exit_code": code, "original_data_unchanged": True, "audio_device": "dummy" if not args.no_audio else "disabled", "screenshots": shots, "claim": "Real-port UI/runtime smoke; not original-binary parity or audible-quality certification"}
        (output / "result.json").write_text(json.dumps(result, indent=2) + "\n")
        print(json.dumps(result, indent=2))
        return 0
    finally:
        if game is not None and game.poll() is None:
            game.terminate()
            try:
                game.wait(timeout=3)
            except subprocess.TimeoutExpired:
                game.kill()
                game.wait()
        if driver is not None:
            driver.close()
        if xvfb is not None:
            xvfb.terminate()
            xvfb.wait(timeout=5)

def send_close(driver: Driver):
    class Data(C.Union):
        _fields_ = [("b", C.c_char * 20), ("s", C.c_short * 10), ("l", C.c_long * 5)]
    class ClientMessage(C.Structure):
        _fields_ = [("type", C.c_int), ("serial", C.c_ulong), ("send_event", C.c_int), ("display", C.c_void_p), ("window", C.c_ulong), ("message_type", C.c_ulong), ("format", C.c_int), ("data", Data)]
    x = driver.x
    x.XGetInputFocus.argtypes = [C.c_void_p, C.POINTER(C.c_ulong), C.POINTER(C.c_int)]
    x.XInternAtom.argtypes = [C.c_void_p, C.c_char_p, C.c_int]
    x.XInternAtom.restype = C.c_ulong
    x.XSendEvent.argtypes = [C.c_void_p, C.c_ulong, C.c_int, C.c_long, C.c_void_p]
    window, revert = C.c_ulong(), C.c_int()
    x.XGetInputFocus(driver.display, C.byref(window), C.byref(revert))
    event = ClientMessage()
    event.type = 33
    event.display = driver.display
    event.window = window.value
    event.message_type = x.XInternAtom(driver.display, b"WM_PROTOCOLS", 0)
    event.format = 32
    event.data.l[0] = x.XInternAtom(driver.display, b"WM_DELETE_WINDOW", 0)
    event.data.l[1] = 0
    x.XSendEvent(driver.display, window, 0, 0, C.byref(event))
    x.XFlush(driver.display)

if __name__ == "__main__":
    raise SystemExit(main())
