"""Minimal assembler for vUnit bytecode.
Usage: python vu_asm.py input.asm output.bin
"""

from __future__ import annotations

import argparse
import re
import struct
import sys
from dataclasses import dataclass
from pathlib import Path

REGS_COUNT = 16
VAR_KEY_LEN = 6

OPCODES = {
    "HALT": 0,
    "NOP": 1,
    "JMP": 2,
    "JZ": 3,
    "JP": 4,
    "JN": 5,
    "CMPVV": 6,
    "CMPRR": 7,
    "CMPRV": 8,
    "CMPVR": 9,
    "ADD_R": 10,
    "SUB_R": 11,
    "MUL_R": 12,
    "DIV_R": 13,
    "MOD_R": 14,
    "NEG_R": 15,
    "ADD_V": 16,
    "SUB_V": 17,
    "MUL_V": 18,
    "DIV_V": 19,
    "MOD_V": 20,
    "NEG_V": 21,
    "ADD_VR": 22,
    "SUB_VR": 23,
    "MUL_VR": 24,
    "DIV_VR": 25,
    "MOD_VR": 26,
    "ADD_RV": 27,
    "SUB_RV": 28,
    "MUL_RV": 29,
    "DIV_RV": 30,
    "MOD_RV": 31,
    "AND": 32,
    "OR": 33,
    "XOR": 34,
    "NOT": 35,
    "SHL": 36,
    "SHR": 37,
    "LOAD8": 38,
    "LOAD16": 39,
    "LOAD32": 40,
    "STORE8": 41,
    "STORE16": 42,
    "STORE32": 43,
    "VTOR": 44,
    "RTOV": 45,
    "SETR": 46,
    "SETV": 47,
    "NEWVAR": 48,
    "LDWND": 49,
    "STWND": 50,
    "DBGVAR": 51,
    "DBGREG": 52,
    "DBGWND": 53,
    "DBGPRG": 54,
    "DBGDAT": 55,
    "DBGSYS": 56,
}

ZERO = {"HALT", "NOP", "DBGVAR", "DBGREG", "DBGWND", "DBGPRG", "DBGDAT", "DBGSYS"}
JUMPS = {"JMP", "JZ", "JP", "JN"}
CMP_RR = {"CMPRR"}
CMP_RV = {"CMPRV"}
CMP_VR = {"CMPVR"}
CMP_VV = {"CMPVV"}
RR_BINARY = {
    "ADD_R", "SUB_R", "MUL_R", "DIV_R", "MOD_R",
    "AND", "OR", "XOR", "SHL", "SHR",
}
R_UNARY = {"NEG_R", "NOT"}
VV_BINARY = {"ADD_V", "SUB_V", "MUL_V", "DIV_V", "MOD_V"}
VR_BINARY = {"ADD_VR", "SUB_VR", "MUL_VR", "DIV_VR", "MOD_VR"}
RV_BINARY = {"ADD_RV", "SUB_RV", "MUL_RV", "DIV_RV", "MOD_RV"}
V_UNARY = {"NEG_V"}


class AsmError(Exception):
    def __init__(self, line: int, message: str):
        super().__init__(message)
        self.line = line
        self.message = message


@dataclass
class SourceLine:
    number: int
    text: str


def strip_comments(text: str) -> str:
    for marker in (";", "#"):
        text = text.split(marker, 1)[0]
    return text.strip()


def split_operands(text: str, line: int) -> list[str]:
    if not text.strip():
        return []
    parts = [item.strip() for item in text.split(",")]
    if any(not item for item in parts):
        raise AsmError(line, "пустой операнд: проверь запятые")
    return parts


def parse_number(token: str, line: int) -> int:
    try:
        value = int(token, 0)
    except ValueError:
        raise AsmError(line, f"ожидалось число, получено '{token}'")
    if not -(1 << 31) <= value <= 0xFFFFFFFF:
        raise AsmError(line, f"число '{token}' не помещается в 32 бита")
    return value & 0xFFFFFFFF


def parse_reg(token: str, line: int) -> int:
    match = re.fullmatch(r"[rR](\d+)", token)
    if not match:
        raise AsmError(line, f"ожидался регистр r0..r{REGS_COUNT - 1}, получено '{token}'")
    reg = int(match.group(1), 10)
    if not 0 <= reg < REGS_COUNT:
        raise AsmError(line, f"регистр '{token}' вне диапазона r0..r{REGS_COUNT - 1}")
    return reg


def parse_var(token: str, line: int) -> bytes:
    if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", token):
        raise AsmError(line, f"недопустимое имя переменной '{token}'")
    raw = token.encode("ascii")
    if len(raw) > VAR_KEY_LEN:
        raise AsmError(line, f"имя переменной '{token}' длиннее {VAR_KEY_LEN} байт")
    return raw.ljust(VAR_KEY_LEN, b"\0")


def require_count(op: str, operands: list[str], count: int, line: int) -> None:
    if len(operands) != count:
        suffix = "операнд" if count == 1 else "операнда"
        raise AsmError(line, f"{op} ожидает {count} {suffix}, получено {len(operands)}")


def parse_lines(path: Path) -> tuple[list[SourceLine], dict[str, int]]:
    parsed: list[SourceLine] = []
    labels: dict[str, int] = {}
    pc = 0

    try:
        lines = path.read_text(encoding="utf-8").splitlines()
    except UnicodeDecodeError:
        raise AsmError(0, "файл должен быть UTF-8 текстом")

    for number, raw in enumerate(lines, 1):
        text = strip_comments(raw)
        if not text:
            continue

        while True:
            match = re.match(r"^([A-Za-z_][A-Za-z0-9_]*):", text)
            if not match:
                break
            label = match.group(1)
            if label in labels:
                raise AsmError(number, f"метка '{label}' объявлена повторно")
            labels[label] = pc
            text = text[match.end():].strip()
            if not text:
                break

        if not text:
            continue

        size = instruction_size(text, number)
        parsed.append(SourceLine(number, text))
        pc += size

    return parsed, labels


def instruction_size(text: str, line: int) -> int:
    op = text.split(None, 1)[0].upper()
    rest = text[len(text.split(None, 1)[0]):].strip()
    operands = split_operands(rest, line)

    if op not in OPCODES:
        raise AsmError(line, f"неизвестный opcode '{op}'")
    if op in ZERO:
        require_count(op, operands, 0, line)
        return 1
    if op in JUMPS:
        require_count(op, operands, 1, line)
        return 5
    if op in R_UNARY or op in V_UNARY or op == "NEWVAR":
        require_count(op, operands, 1, line)
        return 2 if op in R_UNARY else 7
    if op in RR_BINARY or op in CMP_RR:
        require_count(op, operands, 2, line)
        return 3
    if op in VV_BINARY or op in CMP_VV:
        require_count(op, operands, 2, line)
        return 13
    if op in VR_BINARY or op in CMP_VR:
        require_count(op, operands, 2, line)
        return 8
    if op in RV_BINARY or op in CMP_RV:
        require_count(op, operands, 2, line)
        return 8
    if op == "SETR":
        require_count(op, operands, 2, line)
        return 6
    if op == "SETV":
        require_count(op, operands, 2, line)
        return 11
    if op in {"VTOR", "RTOV"}:
        require_count(op, operands, 2, line)
        return 8
    if op in {"LOAD8", "LOAD16", "LOAD32"}:
        require_count(op, operands, 2, line)
        return 6
    if op in {"STORE8", "STORE16", "STORE32"}:
        require_count(op, operands, 2, line)
        return 6
    if op in {"LDWND", "STWND"}:
        require_count(op, operands, 1, line)
        return 5

    raise AsmError(line, f"для '{op}' не задан формат")


def encode_instruction(source: SourceLine, labels: dict[str, int]) -> bytes:
    text = source.text
    head = text.split(None, 1)[0]
    op = head.upper()
    rest = text[len(head):].strip()
    operands = split_operands(rest, source.number)
    out = bytearray([OPCODES[op]])

    if op in ZERO:
        return bytes(out)

    if op in JUMPS:
        target = operands[0]
        value = labels[target] if target in labels else parse_number(target, source.number)
        out += struct.pack("<I", value)
        return bytes(out)

    if op in R_UNARY:
        out.append(parse_reg(operands[0], source.number))
        return bytes(out)

    if op in V_UNARY or op == "NEWVAR":
        out += parse_var(operands[0], source.number)
        return bytes(out)

    if op in RR_BINARY or op in CMP_RR:
        out.append(parse_reg(operands[0], source.number))
        out.append(parse_reg(operands[1], source.number))
        return bytes(out)

    if op in VV_BINARY or op in CMP_VV:
        out += parse_var(operands[0], source.number)
        out += parse_var(operands[1], source.number)
        return bytes(out)

    if op in VR_BINARY or op in CMP_VR:
        out += parse_var(operands[0], source.number)
        out.append(parse_reg(operands[1], source.number))
        return bytes(out)

    if op in RV_BINARY or op in CMP_RV:
        out.append(parse_reg(operands[0], source.number))
        out += parse_var(operands[1], source.number)
        return bytes(out)

    if op == "SETR":
        out.append(parse_reg(operands[0], source.number))
        out += struct.pack("<I", parse_number(operands[1], source.number))
        return bytes(out)

    if op == "SETV":
        out += parse_var(operands[0], source.number)
        out += struct.pack("<I", parse_number(operands[1], source.number))
        return bytes(out)

    # VTOR r0, a
    # r0 = variable[a]
    # bytecode: opcode, reg, var[6]
    if op == "VTOR":
        out.append(parse_reg(operands[0], source.number))
        out += parse_var(operands[1], source.number)
        return bytes(out)

    # RTOV a, r0
    # variable[a] = r0
    # bytecode: opcode, var[6], reg
    if op == "RTOV":
        out += parse_var(operands[0], source.number)
        out.append(parse_reg(operands[1], source.number))
        return bytes(out)

    if op in {"LOAD8", "LOAD16", "LOAD32"}:
        out.append(parse_reg(operands[0], source.number))
        out += struct.pack("<I", parse_number(operands[1], source.number))
        return bytes(out)

    if op in {"STORE8", "STORE16", "STORE32"}:
        out += struct.pack("<I", parse_number(operands[0], source.number))
        out.append(parse_reg(operands[1], source.number))
        return bytes(out)

    if op in {"LDWND", "STWND"}:
        out += struct.pack("<I", parse_number(operands[0], source.number))
        return bytes(out)

    raise AsmError(source.number, f"для '{op}' не задан encoder")


def assemble(input_path: Path, output_path: Path) -> int:
    source, labels = parse_lines(input_path)
    output = bytearray()
    for item in source:
        output += encode_instruction(item, labels)
    output_path.write_bytes(output)
    return len(output)


def main() -> int:
    parser = argparse.ArgumentParser(
        prog="vu_asm.py",
        description="Assembler: file.asm -> file.bin для vUnit.",
    )
    parser.add_argument("input", type=Path, help="входной .asm файл")
    parser.add_argument("output", type=Path, help="выходной .bin файл")
    args = parser.parse_args()

    if not args.input.is_file():
        parser.error(f"входной файл не существует или не является файлом: {args.input}")
    if args.input.resolve() == args.output.resolve():
        parser.error("входной и выходной файлы не должны совпадать")

    try:
        size = assemble(args.input, args.output)
    except AsmError as error:
        prefix = f"строка {error.line}: " if error.line else ""
        print(f"Ошибка: {prefix}{error.message}", file=sys.stderr)
        return 1
    except OSError as error:
        print(f"Ошибка I/O: {error}", file=sys.stderr)
        return 1

    print(f"OK: {args.input} -> {args.output}; {size} bytes")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())