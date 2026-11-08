#!/usr/bin/env python3
"""
APX Anonymizer: Generates sanitized/synthetic APX definition files from proprietary nodes.

Preserves:
- Full AST and structural fidelity (type primitives, arrays, records, enums, limits)
- Memory layouts, binary signal offsets, and alignments
- Cross-node provide/require port connectivity and port signatures

Sanitizes:
- Node names -> SyntheticNodeA, SyntheticNodeB, etc.
- Port names -> Sig_0001, Sig_0002, etc. (shared between matching provide/require ports)
- Type names -> Type_0001_T, Type_0002_T, etc.
- Record fields -> f0, f1, f2, etc.
- Enum labels -> E0, E1, E2, etc.
"""

import argparse
import os
from pathlib import Path
import re
import sys

def tokenize_record_fields(record_str):
    """
    Given a record body like '"Field1"C(0,3)"Field2"S', replaces field names
    with "f0", "f1", ... while preserving field type expressions.
    """
    # Record fields are sequences of "name"<type_expr>
    # We parse quoted strings and replace them with f0, f1, ...
    result = []
    field_idx = 0
    i = 0
    n = len(record_str)
    while i < n:
        if record_str[i] == '"':
            # start of field name
            j = record_str.find('"', i + 1)
            if j == -1:
                result.append(record_str[i:])
                break
            result.append(f'"f{field_idx}"')
            field_idx += 1
            i = j + 1
        else:
            result.append(record_str[i])
            i += 1
    return "".join(result)

def anonymize_vt(vt_content):
    """
    Given VT content like '("Val1","Val2","Val3")', replaces labels with 'E0','E1','E2'...
    """
    # Matches strings in quotes
    idx = 0
    def repl(m):
        nonlocal idx
        name = f'"E{idx}"'
        idx += 1
        return name
    return re.sub(r'"[^"]*"', repl, vt_content)

def parse_and_anonymize(file_paths, output_dir, node_name_prefix="SyntheticNode"):
    os.makedirs(output_dir, exist_ok=True)

    # Step 1: Collect all port names across all files and find matching provide/require connections
    provide_ports_by_file = {}
    require_ports_by_file = {}
    all_port_names = set()

    for p in file_paths:
        with open(p, "r", encoding="utf-8") as f:
            lines = [l.strip() for l in f if l.strip()]
        p_names = set()
        r_names = set()
        for line in lines:
            if line.startswith("P\""):
                m = re.match(r'^P"([^"]+)"', line)
                if m:
                    p_names.add(m.group(1))
                    all_port_names.add(m.group(1))
            elif line.startswith("R\""):
                m = re.match(r'^R"([^"]+)"', line)
                if m:
                    r_names.add(m.group(1))
                    all_port_names.add(m.group(1))
        provide_ports_by_file[p] = p_names
        require_ports_by_file[p] = r_names

    # Determine priority for shared (connected) ports so they get low IDs first
    shared_ports = set()
    for i, p1 in enumerate(file_paths):
        for j, p2 in enumerate(file_paths):
            if i != j:
                shared_ports.update(provide_ports_by_file[p1].intersection(require_ports_by_file[p2]))

    unshared_ports = sorted(list(all_port_names - shared_ports))
    sorted_shared_ports = sorted(list(shared_ports))

    # Port name mapping: shared ports first, then unshared ports
    port_map = {}
    port_counter = 1
    for p_name in sorted_shared_ports:
        port_map[p_name] = f"Sig_{port_counter:04d}"
        port_counter += 1
    for p_name in unshared_ports:
        port_map[p_name] = f"Sig_{port_counter:04d}"
        port_counter += 1

    # Step 2: Collect all type names across all files
    all_type_names = set()
    for p in file_paths:
        with open(p, "r", encoding="utf-8") as f:
            for line in f:
                line = line.strip()
                if line.startswith("T\""):
                    m = re.match(r'^T"([^"]+)"', line)
                    if m:
                        all_type_names.add(m.group(1))

    type_map = {}
    for idx, t_name in enumerate(sorted(list(all_type_names)), 1):
        type_map[t_name] = f"Type_{idx:04d}_T"

    # Step 3: Anonymize each file
    generated_files = []
    for file_idx, p in enumerate(file_paths, 1):
        with open(p, "r", encoding="utf-8") as f:
            lines = [l.strip() for l in f if l.strip()]

        out_lines = []
        new_node_name = f"{node_name_prefix}_{chr(ord('A') + file_idx - 1)}"
        out_filename = os.path.join(output_dir, f"{new_node_name}.apx")

        for line in lines:
            if line.startswith("APX/"):
                out_lines.append(line)
            elif line.startswith("N\""):
                out_lines.append(f'N"{new_node_name}"')
            elif line.startswith("T\""):
                # Type definition: T"<TypeName>"<TypeExpr>
                m = re.match(r'^T"([^"]+)"(.*)$', line)
                if not m:
                    out_lines.append(line)
                    continue
                old_tname, rest = m.group(1), m.group(2)
                new_tname = type_map.get(old_tname, old_tname)

                # Process VT if present
                if ":VT(" in rest:
                    prefix, vt_part = rest.split(":VT(", 1)
                    vt_part = anonymize_vt("(" + vt_part)
                    rest = prefix + ":VT" + vt_part

                # Process records if present
                if "{" in rest and "}" in rest:
                    def repl_rec(rm):
                        body = rm.group(1)
                        return "{" + tokenize_record_fields(body) + "}"
                    rest = re.sub(r'\{([^}]+)\}', repl_rec, rest)

                # Replace any inner T["..."] references
                for old_t, new_t in type_map.items():
                    target = f'T["{old_t}"]'
                    if target in rest:
                        rest = rest.replace(target, f'T["{new_t}"]')

                out_lines.append(f'T"{new_tname}"{rest}')

            elif line.startswith("P\"") or line.startswith("R\""):
                prefix = line[0] # 'P' or 'R'
                # Format: P"<PortName>"<TypeExpr>[:=<Init>]
                # Match port name
                m = re.match(r'^[PR]"([^"]+)"(.*)$', line)
                if not m:
                    out_lines.append(line)
                    continue
                old_pname, rest = m.group(1), m.group(2)
                new_pname = port_map.get(old_pname, old_pname)

                # Split init if present
                if ":=" in rest:
                    type_part, init_part = rest.split(":=", 1)
                    init_str = ":=" + init_part
                else:
                    type_part = rest
                    init_str = ""

                # Replace T["..."] in type_part
                for old_t, new_t in type_map.items():
                    target = f'T["{old_t}"]'
                    if target in type_part:
                        type_part = type_part.replace(target, f'T["{new_t}"]')

                # In case port has inline record
                if "{" in type_part and "}" in type_part:
                    def repl_rec(rm):
                        body = rm.group(1)
                        return "{" + tokenize_record_fields(body) + "}"
                    type_part = re.sub(r'\{([^}]+)\}', repl_rec, type_part)

                out_lines.append(f'{prefix}"{new_pname}"{type_part}{init_str}')

        with open(out_filename, "w", encoding="utf-8", newline="\n") as out_f:
            out_f.write("\n".join(out_lines) + "\n")

        print(f"Generated {out_filename} ({len(out_lines)} lines, {os.path.getsize(out_filename)} bytes)")
        generated_files.append(out_filename)

    return generated_files

def main():
    parser = argparse.ArgumentParser(
        description="Sanitize APX definition files into synthetic nodes while preserving AST, layout, and port connectivity."
    )
    parser.add_argument(
        "files",
        nargs="+",
        metavar="FILE",
        help="One or more APX definition files (.apx) to anonymize jointly",
    )
    parser.add_argument(
        "--output-dir",
        "-d",
        default=".",
        help="Directory to write anonymized files into (default: current directory '.')",
    )
    parser.add_argument(
        "--prefix",
        default="SyntheticNode",
        help="Prefix for anonymized node names (default: 'SyntheticNode')",
    )

    args = parser.parse_args()
    parse_and_anonymize(args.files, args.output_dir, node_name_prefix=args.prefix)


if __name__ == "__main__":
    main()

