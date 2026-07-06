#!/usr/bin/env python3

# Simulación del particionamiento del bloque Standard Speed Data (28005 bytes)

SIZE_FOR_SPLIT = 4096  # Tamaño típico de partition
totalSize = 28005      # Tamaño del bloque de datos
offsetBase = 0x30 + 5  # Offset después del header del bloque 0x10 (5 bytes de ID + pause + length)

blocks = totalSize // SIZE_FOR_SPLIT
lastBlockSize = totalSize - (blocks * SIZE_FOR_SPLIT)

print(f"File Partition Simulation")
print(f"=" * 60)
print(f"Total data size: {totalSize} bytes")
print(f"Block size for split: {SIZE_FOR_SPLIT} bytes")
print(f"Number of full partitions: {blocks}")
print(f"Last block size: {lastBlockSize} bytes")
print(f"Base offset: 0x{offsetBase:06x}\n")

total_read = 0

# Primera partición
print(f"PARTITION 0 (BEGIN):")
offset = offsetBase + (SIZE_FOR_SPLIT * 0)
size = SIZE_FOR_SPLIT
print(f"  Offset: 0x{offset:06x}")
print(f"  Size: {size} bytes")
print(f"  Cumulative: {total_read} → {total_read + size}")
total_read += size

# Particiones intermedias
for n in range(1, blocks):
    print(f"\nPARTITION {n} (MIDDLE):")
    offset = offsetBase + (SIZE_FOR_SPLIT * n)
    size = SIZE_FOR_SPLIT
    print(f"  Offset: 0x{offset:06x}")
    print(f"  Size: {size} bytes")
    print(f"  Cumulative: {total_read} → {total_read + size}")
    total_read += size

# Última partición
print(f"\nPARTITION {blocks} (END):")
offset = offsetBase + (SIZE_FOR_SPLIT * blocks)
size = lastBlockSize
print(f"  Offset: 0x{offset:06x}")
print(f"  Size: {size} bytes")
print(f"  Cumulative: {total_read} → {total_read + size}")
total_read += size

print(f"\n" + "=" * 60)
print(f"Total bytes read: {total_read} (should be {totalSize})")
print(f"Match: {'✓ YES' if total_read == totalSize else '✗ NO - ERROR!'}")

# Check for potential issues
print(f"\nPotential Issues:")
if lastBlockSize == 0:
    print(f"  ⚠️  Last block size is 0 - could cause empty playDataEnd call")
if lastBlockSize < SIZE_FOR_SPLIT and lastBlockSize > 0:
    print(f"  ✓ Last block size is smaller than split - normal")
