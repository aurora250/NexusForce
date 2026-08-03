#!/usr/bin/env python3
"""NexusForce API 索引生成器

从 Doxygen XML 输出生成紧凑的 JSONL 索引文件，每行一个 JSON 对象，
描述一个公开 API 类型或自由函数。AI Agent 通过 Grep 查找所需 API，
再打开对应的头文件获取完整签名。

用法:
    python generate_api_index.py <xml_dir> <output_file>
"""

import json
import os
import sys
import xml.etree.ElementTree as ET

import builtins


def build_namespace_map(xml_dir: str) -> dict:
    """从 namespace*.xml 构建 refid → 命名空间映射"""
    mapping = {}
    for fn in os.listdir(xml_dir):
        if not fn.startswith("namespace") or not fn.endswith(".xml"):
            continue
        try:
            tree = ET.parse(os.path.join(xml_dir, fn))
            cd = tree.getroot().find("compounddef")
            if cd is None:
                continue
            ns_name = cd.findtext("compoundname", "")
            if not ns_name or ns_name == "std" or ns_name.startswith("inner"):
                continue
            for ic in cd.findall("innerclass"):
                refid = ic.get("refid", "")
                if refid:
                    mapping[refid] = ns_name
        except ET.ParseError:
            continue
    return mapping


def header_from_location(loc_elem) -> str | None:
    """从 <location> 提取 include 路径

    将 ``D:/.../include/NeForce/core/foo.hpp`` 转换为
    ``NeForce/core/foo.hpp``。
    """
    fp = loc_elem.get("file", "")
    for sep in ("/include/", "\\include\\"):
        idx = fp.find(sep)
        if idx != -1:
            return fp[idx + builtins.len(sep):].replace("\\", "/")
    return None


def extract_brief(elem) -> str:
    """从 <briefdescription> 提取中文简述"""
    brief = elem.find("briefdescription")
    if brief is None:
        return ""
    para = brief.find("para")
    text = "".join((para if para is not None else brief).itertext()).strip()
    return " ".join(text.split())


def normalize_name(raw_name: str, ns: str) -> str:
    """去掉 compoundname 中的命名空间前缀"""
    prefix = ns + "::"
    while raw_name.startswith(prefix):
        raw_name = raw_name[builtins.len(prefix):]
    return raw_name


def ns_from_qualified_name(qname: str) -> str:
    """从 qualifiedname 提取命名空间"""
    if "::" not in qname:
        return ""
    return qname.rsplit("::", 1)[0]


def ns_from_header(header: str) -> str:
    """从 include 路径推断命名空间"""
    if header.startswith("NeForce/"):
        return "neforce"
    return ""


def is_valid_ns(ns: str) -> bool:
    """检查命名空间是否应该包含在索引中"""
    return ns != "std" and not ns.startswith("inner")


def is_public_name(name: str) -> bool:
    """检查名称是否为公开 API（排除 __ / _ 前缀的内部符号）"""
    return not name.startswith("_")


# ── 入口 ──

VALID_COMPOUND_KINDS = {"class", "struct", "concept", "union"}
VALID_MEMBERDEF_KINDS = {"function", "enum", "typedef", "define"}

# Doxygen kind → JSONL 输出类型名
KIND_MAP = {"define": "macro"}


def extract_from_compound(xml_dir: str, ns_map: dict, entries: list, seen: set):
    """从 index.xml 中的顶层 compound 提取类型"""
    index_path = os.path.join(xml_dir, "index.xml")
    tree = ET.parse(index_path)
    root = tree.getroot()

    for compound in root.findall("compound"):
        kind = compound.get("kind", "")
        if kind not in VALID_COMPOUND_KINDS and kind != "group":
            continue

        refid = compound.get("refid", "")
        if not refid or refid in seen:
            continue

        xml_path = os.path.join(xml_dir, f"{refid}.xml")
        if not os.path.isfile(xml_path):
            continue

        try:
            ct = ET.parse(xml_path)
            cd = ct.getroot().find("compounddef")
        except ET.ParseError:
            continue

        if cd is None:
            continue

        if kind in VALID_COMPOUND_KINDS:
            _extract_top_level(cd, refid, kind, compound.findtext("name", ""),
                               ns_map, entries, seen)
        elif kind == "group":
            _extract_from_group(cd, ns_map, entries, seen)


def _extract_top_level(cd, refid, kind, name, ns_map, entries, seen):
    """提取顶层 class / struct / concept / union"""
    if not name or refid in seen:
        return
    seen.add(refid)

    if cd.get("prot", "public") != "public":
        return

    loc = cd.find("location")
    if loc is None:
        return
    header = header_from_location(loc)
    if not header:
        return

    ns = ns_map.get(refid, "")
    if not ns:
        ns = ns_from_header(header)
    if not is_valid_ns(ns):
        return

    short_name = normalize_name(name, ns)

    # 跳过多层嵌套类型（如 basic_string::storage::long_pointer）——内部实现细节
    if short_name.count("::") >= 2:
        return

    if not is_public_name(short_name):
        return

    brief = extract_brief(cd)
    entries.append({
        "name":   short_name,
        "kind":   kind,
        "ns":     ns,
        "header": header,
        "brief":  brief,
    })


def _extract_from_group(cd, ns_map, entries, seen):
    """从 group compound 中提取自由函数 / 枚举 / typedef"""
    for memberdef in cd.findall(".//memberdef"):
        kind = memberdef.get("kind", "")
        if kind not in VALID_MEMBERDEF_KINDS:
            continue

        if memberdef.get("prot", "public") != "public":
            continue

        name = memberdef.findtext("name", "")
        member_id = memberdef.get("id", "")
        if not name or not member_id or member_id in seen:
            continue

        # 跳过类的成员函数 / 嵌套类型
        if "::" in name:
            continue

        # 跳过内部符号（__ / _ 前缀）
        if not is_public_name(name):
            continue

        loc = memberdef.find("location")
        if loc is None:
            continue
        header = header_from_location(loc)
        if not header:
            continue

        # ── 命名空间 ──
        # 优先从 qualifiedname 提取，define 无此字段则兜底
        qname = memberdef.findtext("qualifiedname", "")
        ns = ns_from_qualified_name(qname) if qname else ""
        if not ns:
            ns = ns_from_header(header)
        if not is_valid_ns(ns):
            continue

        seen.add(member_id)

        brief = extract_brief(memberdef)
        entries.append({
            "name":   name,
            "kind":   KIND_MAP.get(kind, kind),
            "ns":     ns,
            "header": header,
            "brief":  brief,
        })


# ── Main ──

def main() -> int:
    if builtins.len(sys.argv) != 3:
        builtins.print("用法: generate_api_index.py <xml_dir> <output_file>",
                        file=sys.stderr)
        sys.exit(1)

    xml_dir = sys.argv[1]
    output_file = sys.argv[2]

    # ── 构建命名空间映射 ──
    ns_map = build_namespace_map(xml_dir)

    # ── 提取所有 API ──
    entries = []
    seen = builtins.set()
    extract_from_compound(xml_dir, ns_map, entries, seen)

    # ── 排序并写出 ──
    entries.sort(key=lambda e: e["name"])
    os.makedirs(os.path.dirname(output_file) or ".", exist_ok=True)

    with builtins.open(output_file, "w", encoding="utf-8") as f:
        for entry in entries:
            f.write(json.dumps(entry, ensure_ascii=False) + "\n")

    builtins.print(f"{builtins.len(entries)} 条记录已写入 {output_file}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
