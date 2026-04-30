import struct

with open('xevious.tzx', 'rb') as f:
    data = f.read()

# Bloque Turbo Speed en offset 10
block_offset = 10
print('Turbo Speed Block (0x11) - xevious.tzx')
print('=' * 60)

# Estructura del bloque 0x11:
# offset+0:  ID (0x11)
# offset+1-2: pilot_len
# offset+3-4: sync_1
# offset+5-6: sync_2
# offset+7-8: bit_0
# offset+9-10: bit_1
# offset+11-12: pilot_num_pulses
# offset+13: maskLastByte
# offset+14-15: pauseAfterThisBlock
# offset+16-18: lengthOfData
# offset+19+: datos

fields = [
    ('ID', block_offset, 1, 'hex'),
    ('pilot_len', block_offset + 1, 2, 'int_le'),
    ('sync_1', block_offset + 3, 2, 'int_le'),
    ('sync_2', block_offset + 5, 2, 'int_le'),
    ('bit_0', block_offset + 7, 2, 'int_le'),
    ('bit_1', block_offset + 9, 2, 'int_le'),
    ('pilot_num_pulses', block_offset + 11, 2, 'int_le'),
    ('maskLastByte', block_offset + 13, 1, 'hex'),
    ('pauseAfterThisBlock', block_offset + 14, 2, 'int_le'),
    ('lengthOfData', block_offset + 16, 3, 'int_le3'),
]

for name, offset, size, fmt in fields:
    if fmt == 'hex':
        val = f'0x{data[offset]:02X}'
    elif fmt == 'int_le':
        val = struct.unpack('<H', data[offset:offset+2])[0]
    elif fmt == 'int_le3':
        val = data[offset] | (data[offset+1] << 8) | (data[offset+2] << 16)
    print(f'{name:20s}: offset +{offset - block_offset:2d} = {val}')

print()
print(f'Data starts at offset {block_offset + 19}')
print(f'First 20 data bytes: {" ".join(f"{b:02X}" for b in data[block_offset+19:block_offset+39])}')
