#!/usr/bin/env python3
"""Generate NeForce theme palettes from dsh-TUI's src/theme.ts.

Usage:
    python3 scripts/gen_theme_palettes.py [path/to/dsh-TUI/src/theme.ts] \
        [--out src/tui/dom/theme_palettes.cpp] [--header include/NeForce/tui/dom/theme_roles.inc]

The source file is MIT licensed; only color values and role names are extracted.
"""
import json
import re
import sys

DEFAULT_SRC = "/media/huenqi/Programming/github clonelib/dsh-TUI/src/theme.ts"

PALETTES = ["darkTheme", "lightTheme", "darkAnsiTheme",
            "pinkNightTheme", "pinkDayTheme", "pinkAnsiTheme"]
PALETTE_ENUM = ["dark", "light", "dark_ansi", "pink_night", "pink_day", "pink_ansi"]


def strip_comments(text):
    out = []
    i = 0
    n = len(text)
    while i < n:
        if text.startswith("//", i):
            while i < n and text[i] != "\n":
                i += 1
        elif text.startswith("/*", i):
            i = text.find("*/", i)
            i = n if i < 0 else i + 2
        else:
            out.append(text[i])
            i += 1
    return "".join(out)


def find_object(text, start):
    """Return (body, end_index) for the object literal starting at start."""
    depth = 0
    i = start
    while i < len(text):
        c = text[i]
        if c == "{":
            depth += 1
        elif c == "}":
            depth -= 1
            if depth == 0:
                return text[start + 1:i], i + 1
        elif c == "'":
            i += 1
            while i < len(text) and text[i] != "'":
                if text[i] == "\\":
                    i += 1
                i += 1
        i += 1
    raise ValueError("unbalanced object")


def parse_entries(body):
    entries = []
    depth = 0
    current = []
    for c in body:
        if c in "{[(":
            depth += 1
        elif c in "}])":
            depth -= 1
        if c == "," and depth == 0:
            entries.append("".join(current).strip())
            current = []
        else:
            current.append(c)
    if current:
        entries.append("".join(current).strip())
    out = []
    for entry in entries:
        if not entry:
            continue
        if entry.startswith("..."):
            out.append(("spread", entry[3:].strip()))
            continue
        key, _, value = entry.partition(":")
        key = key.strip()
        value = value.strip()
        if key.startswith('"') or key.startswith("'"):
            key = key[1:-1]
        out.append((key, value))
    return out


ANSI16 = {
    "black": "#000000", "red": "#800000", "green": "#008000", "yellow": "#808000",
    "blue": "#000080", "magenta": "#800080", "cyan": "#008080", "white": "#c0c0c0",
    "blackBright": "#808080", "redBright": "#ff0000", "greenBright": "#00ff00",
    "yellowBright": "#ffff00", "blueBright": "#0000ff", "magentaBright": "#ff00ff",
    "cyanBright": "#00ffff", "whiteBright": "#ffffff",
}


def parse_color(value):
    value = value.strip()
    if value in ("''", '""'):
        return None
    m = re.match(r"rgb\(\s*'(#[0-9A-Fa-f]{3,8})'\s*\)", value)
    if m:
        return m.group(1)
    m = re.match(r"'(#[0-9A-Fa-f]{3,8})'", value)
    if m:
        return m.group(1)
    m = re.match(r"'ansi:([A-Za-z]+)'", value)
    if m:
        return ANSI16[m.group(1)]
    m = re.match(r"'ansi256\((\d+)\)'", value)
    if m:
        return "ansi256:" + m.group(1)
    raise ValueError("unsupported color expression: " + value)


def camel_to_snake(name):
    out = []
    for i, c in enumerate(name):
        if c.isupper() and i > 0 and not name[i - 1].isupper():
            out.append("_")
        out.append(c.lower())
    return "".join(out)


def main():
    src = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_SRC
    text = strip_comments(open(src, encoding="utf-8").read())
    objects = {}
    for m in re.finditer(r"const\s+(\w+)\s*:\s*[\w<>]+\s*=\s*\{", text):
        name = m.group(1)
        body, _ = find_object(text, m.end() - 1)
        objects[name] = parse_entries(body)

    resolved = {}

    def resolve(name, stack=()):
        if name in resolved:
            return resolved[name]
        if name not in objects:
            raise KeyError(name)
        if name in stack:
            raise ValueError("cycle " + name)
        result = {}
        for key, value in objects[name]:
            if key == "spread":
                result.update(resolve(value, stack + (name,)))
            else:
                result[key] = parse_color(value)
        resolved[name] = result
        return result

    palettes = {}
    for var in PALETTES:
        palettes[var] = resolve(var)

    roles = []
    for var in PALETTES:
        for key in palettes[var]:
            if key not in roles:
                roles.append(key)

    data = {"roles": roles, "palettes": palettes}
    json.dump(data, open("/tmp/theme_palettes.json", "w"), indent=1)
    print("roles:", len(roles))
    print("palettes:", {k: len(v) for k, v in palettes.items()})
    missing = [r for r in roles if any(r not in palettes[p] for p in PALETTES)]
    print("missing somewhere:", len(missing))


if __name__ == "__main__":
    main()
