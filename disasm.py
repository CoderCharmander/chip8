import argparse
import sys
import collections

parser = argparse.ArgumentParser(description='CHIP-8 disassembler')

parser.add_argument('rom', nargs=1, help='File to be disassembled')
parser.add_argument('-o', nargs=1, help='Output file', metavar='file')
args = parser.parse_args()
out = None
need_clean = False

if args.o == None:
    out = sys.stdout
else:
    out = open(args.o[0], 'w')
    need_clean = True

rom = open(args.rom[0], 'rb')

AddressInfo = collections.namedtuple('AddressInfo', ('analyzed', 'reachable'))


def op_0(x, y, nibble, address, byte):
    if address == 0x0E0:
        return "clr"
    elif address == 0x0EE:
        return "ret"
    else:
        return f"njp {address:03X} ; db 0{address:03X}"


def op_arithm(x, y, nibble, address, byte):
    return {
        0: f'mov V{x:01X}, V{y:01X}',
        1: f'or  V{x:01X}, V{y:01X}',
        2: f'and V{x:01X}, V{y:01X}',
        3: f'xor V{x:01X}, V{y:01X}',
        4: f'add V{x:01X}, V{y:01X}',
        5: f'sub V{x:01X}, V{y:01X}',
        6: f'shr V{x:01X}, V{y:01X}',
        7: f'rsu V{x:01X}, V{y:01X}',
        0xE: f'shl V{x:01X}, V{y:01X}',
    }.get(nibble, f"db  8{address:03X}")


def op_reg(x, y, nibble, address, byte):
    return {
        0x07: f'mov V{x:01X}, DL',
        0x0A: f'wky V{x:01X}',
        0x15: f'mov DL, V{x:01X}',
        0x18: f'mov SN, V{x:01X}',
        0x1E: f'mov I,  V{x:01X}',
        0x29: f'fnt I,  V{x:01X}',
        0x33: f'bcd V{x:01X}',
        0x55: f'dump V0..V{x:01X}',
        0x65: f'load V0..V{x:01X}'
    }.get(byte, f"db  F{address:03X}")


def disassemble(inst):
    x = inst >> 8 & 0xf
    y = inst >> 4 & 0xf
    nibble = inst & 0xf
    address = inst & 0xfff
    byte = inst & 0xff

    return {
        0: op_0,
        1: lambda x, y, nibble, address, byte: f"jmp {address:03X}",
        2: lambda x, y, nibble, address, byte: f"cll {address:03X}",
        3: lambda x, y, nibble, address, byte: f"seq V{x:01X}, {byte:02X}",
        4: lambda x, y, nibble, address, byte: f"sne V{x:01X}, {byte:02X}",
        5: lambda x, y, nibble, address, byte: f"seq V{x:01X}, V{y:01X}",
        6: lambda x, y, nibble, address, byte: f"mov V{x:01X}, {byte:02X}",
        7: lambda x, y, nibble, address, byte: f"add V{x:01X}, {byte:02X}",
        8: op_arithm,
        9: lambda x, y, nibble, address, byte: f"sne V{x:01X}, V{y:01X}",
        10: lambda x, y, nibble, address, byte: f"mov I, {address:03X}",
        11: lambda x, y, nibble, address, byte: f"jmp [{address:03X} + V0]",
        12: lambda x, y, nibble, address, byte: f"mov V{x:01X}, [random & {byte:02X}]",
        13: lambda x, y, nibble, address, byte: f"drw V{x:01X}, V{y:01X}, {nibble:01X}",
        14: lambda x, y, nibble, address, byte: {0x9E: f"skd {x:01X}", 0xA1: f"sku {x:01X}"}.get(byte, f"db  {inst:04X}"),
        15: op_reg
    }.get(inst >> 12 & 0xf, lambda *args: f"db  {inst:04X}")(x, y, nibble, address, byte)


instructions = []
reach_analysis = []
while inst := rom.read(2):
    instructions.append(int.from_bytes(inst, "big", signed=False))
    reach_analysis.append(None)


def analyze_instruction(instr, idx):
    hnibble = instr >> 12 & 0xf
    address = instr & 0xfff
    idx_a = (address - 0x200) // 2
    byte = instr & 0xff

    if hnibble == 1:  # jump
        return [idx_a]
    elif hnibble == 2:  # call
        return [idx_a, idx + 1]
    elif hnibble in (3, 4, 5, 9):  # skip instructions
        return [idx + 1, idx + 2]
    elif hnibble == 0xB:  # jump to address + v0
        return list(range(idx_a, idx_a + 128))
    # skip if key is pressed or not
    elif hnibble == 0xE and byte in (0x9E, 0xA1):
        return [idx + 1, idx + 2]
    else:
        return [idx + 1]


def analyze_at(idx):
    if idx >= len(instructions) or idx < 0:
        return
    if reach_analysis[idx] != None:
        return

    reachable = analyze_instruction(instructions[idx], idx)
    reach_analysis[idx] = True
    for i in reachable:
        analyze_at(i)


print('Analysing reachability...', file=sys.stderr)
analyze_at(0)
print('ANALYSIS DONE! Result:', file=sys.stderr)
for (idx, reach) in enumerate(reach_analysis):
    print(
        f'{idx * 2 + 0x200: 03X}: {instructions[idx]:04X} {"✅" if reach else "❌"}', file=sys.stderr)

counter = 0x200
for (idx, inst) in enumerate(instructions):
    if reach_analysis[idx]:
        print(f'{counter:03X}:', disassemble(inst), file=out)
    else:
        print(f'{counter:03X}: db  {inst:04X}', file=out)
    counter += 2

if need_clean:
    out.close()
