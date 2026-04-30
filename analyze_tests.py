#!/usr/bin/env python3
import struct

def read_byte(f, offset):
    f.seek(offset)
    return struct.unpack('B', f.read(1))[0]

def read_word(f, offset):
    f.seek(offset)
    data = f.read(2)
    return struct.unpack('<H', data)[0]

def read_3bytes(f, offset):
    f.seek(offset)
    data = f.read(3)
    return data[0] | (data[1] << 8) | (data[2] << 16)

def analyze_tzx(filename):
    print(f"\n{'='*70}")
    print(f"Analizando: {filename}")
    print('='*70)
    
    with open(filename, 'rb') as f:
        # Verificar header
        header = f.read(10)
        if header[:7] != b'ZXTape!':
            print("❌ No es un archivo TZX válido")
            return
        
        print("✓ Header TZX válido")
        
        # Leer bloques
        offset = 10  # Después del header
        block_num = 0
        
        while offset < 100000:  # Límite de seguridad
            f.seek(offset)
            
            block_id = read_byte(f, offset)
            if block_id == 0:
                print("\nFin del fichero")
                break
            
            block_num += 1
            
            if block_id == 0x10:  # Standard Speed Data Block
                print(f"\n[BLOQUE {block_num}] ID 0x10 - Standard Speed Data Block")
                pause = read_word(f, offset + 1)
                length = read_word(f, offset + 3)
                print(f"  Pause: {pause}ms")
                print(f"  Data length: {length} bytes")
                offset += 5 + length
                
            elif block_id == 0x11:  # Turbo Speed Data Block
                print(f"\n[BLOQUE {block_num}] ID 0x11 - Turbo Speed Data Block")
                pilot_len = read_word(f, offset + 1)
                sync1 = read_word(f, offset + 3)
                sync2 = read_word(f, offset + 5)
                bit0 = read_word(f, offset + 7)
                bit1 = read_word(f, offset + 9)
                pilot_pulses = read_word(f, offset + 11)
                mask_last_byte = read_byte(f, offset + 13)
                pause = read_word(f, offset + 14)
                length = read_3bytes(f, offset + 16)
                print(f"  Pilot length: {pilot_len}T")
                print(f"  Sync1: {sync1}T, Sync2: {sync2}T")
                print(f"  Bit0: {bit0}T, Bit1: {bit1}T")
                print(f"  Pilot pulses: {pilot_pulses}")
                print(f"  Mask last byte: {mask_last_byte:08b} ({mask_last_byte})")
                print(f"  Pause: {pause}ms")
                print(f"  Data length: {length} bytes")
                offset += 19 + length
                
            elif block_id == 0x15:  # Direct Recording Block
                print(f"\n[BLOQUE {block_num}] ID 0x15 - Direct Recording Block")
                sr = read_3bytes(f, offset + 1)
                pause = read_word(f, offset + 4)
                mask_last_byte = read_byte(f, offset + 6)
                length = read_3bytes(f, offset + 7)
                print(f"  Sampling rate: {sr} Hz")
                print(f"  Pause: {pause}ms")
                print(f"  Mask last byte: {mask_last_byte:08b} ({mask_last_byte})")
                print(f"  Data length: {length} bytes")
                offset += 10 + length
                
            elif block_id == 0x20:  # Pause or Stop the Tape
                print(f"\n[BLOQUE {block_num}] ID 0x20 - Pause or Stop")
                pause = read_word(f, offset + 1)
                print(f"  Pause: {pause}ms")
                offset += 3
                
            else:
                print(f"\n[BLOQUE {block_num}] ID 0x{block_id:02X} - Desconocido")
                # Intentar saltar
                offset += 1

# Analizar los tres tests
analyze_tzx('c:/Users/atama/Documents/200.SPECTRUM/500. Proyectos/PowaDCR - General/powadcr_recorder/Xevious_test1.tzx')
analyze_tzx('c:/Users/atama/Documents/200.SPECTRUM/500. Proyectos/PowaDCR - General/powadcr_recorder/Xevious_test2.tzx')
analyze_tzx('c:/Users/atama/Documents/200.SPECTRUM/500. Proyectos/PowaDCR - General/powadcr_recorder/Xevious_test3.tzx')
