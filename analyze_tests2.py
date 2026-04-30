#!/usr/bin/env python3
import struct
import os

def analyze_tzx(filename):
    print(f"\n{'='*70}")
    print(f"Analizando: {os.path.basename(filename)}")
    print('='*70)
    
    try:
        with open(filename, 'rb') as f:
            # Verificar header
            header = f.read(10)
            if header[:7] != b'ZXTape!':
                print("❌ No es un archivo TZX válido")
                return
            
            print("✓ Header TZX válido\n")
            
            # Leer bloques
            block_num = 0
            
            while True:
                # Leer ID del bloque
                block_id_data = f.read(1)
                if not block_id_data:
                    print(f"\n✓ Final del fichero - Total de bloques: {block_num}")
                    break
                
                block_id = struct.unpack('B', block_id_data)[0]
                block_num += 1
                
                try:
                    if block_id == 0x10:  # Standard Speed Data Block
                        print(f"[BLOQUE {block_num}] ID 0x10 - Standard Speed Data Block")
                        pause = struct.unpack('<H', f.read(2))[0]
                        length = struct.unpack('<H', f.read(2))[0]
                        print(f"  Pause: {pause}ms")
                        print(f"  Data length: {length} bytes")
                        f.read(length)  # Skip data
                        
                    elif block_id == 0x11:  # Turbo Speed Data Block
                        print(f"[BLOQUE {block_num}] ID 0x11 - Turbo Speed Data Block")
                        pilot_len = struct.unpack('<H', f.read(2))[0]
                        sync1 = struct.unpack('<H', f.read(2))[0]
                        sync2 = struct.unpack('<H', f.read(2))[0]
                        bit0 = struct.unpack('<H', f.read(2))[0]
                        bit1 = struct.unpack('<H', f.read(2))[0]
                        pilot_pulses = struct.unpack('<H', f.read(2))[0]
                        mask_last_byte = struct.unpack('B', f.read(1))[0]
                        pause = struct.unpack('<H', f.read(2))[0]
                        length_bytes = f.read(3)
                        length = length_bytes[0] | (length_bytes[1] << 8) | (length_bytes[2] << 16)
                        
                        print(f"  Pilot length: {pilot_len}T")
                        print(f"  Sync1: {sync1}T, Sync2: {sync2}T")
                        print(f"  Bit0: {bit0}T, Bit1: {bit1}T")
                        print(f"  Pilot pulses: {pilot_pulses}")
                        print(f"  Mask last byte: {mask_last_byte:08b}")
                        print(f"  Pause: {pause}ms")
                        print(f"  Data length: {length} bytes")
                        f.read(length)  # Skip data
                        
                    elif block_id == 0x15:  # Direct Recording Block
                        print(f"[BLOQUE {block_num}] ID 0x15 - Direct Recording Block")
                        sr_bytes = f.read(3)
                        sr = sr_bytes[0] | (sr_bytes[1] << 8) | (sr_bytes[2] << 16)
                        pause = struct.unpack('<H', f.read(2))[0]
                        mask_last_byte = struct.unpack('B', f.read(1))[0]
                        length_bytes = f.read(3)
                        length = length_bytes[0] | (length_bytes[1] << 8) | (length_bytes[2] << 16)
                        
                        print(f"  Sampling rate: {sr} Hz")
                        print(f"  Pause: {pause}ms")
                        print(f"  Mask last byte: {mask_last_byte:08b}")
                        print(f"  Data length: {length} bytes")
                        f.read(length)  # Skip data
                        
                    elif block_id == 0x20:  # Pause or Stop the Tape
                        print(f"[BLOQUE {block_num}] ID 0x20 - Pause or Stop")
                        pause = struct.unpack('<H', f.read(2))[0]
                        print(f"  Pause: {pause}ms")
                        
                    else:
                        print(f"[BLOQUE {block_num}] ID 0x{block_id:02X} - Desconocido")
                        # Skip known block types with variable length
                        if block_id in [0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07]:
                            # Skip based on specification
                            pass
                        
                except struct.error as e:
                    print(f"  ❌ Error al leer bloque: {e}")
                    break
                    
    except FileNotFoundError:
        print(f"❌ Archivo no encontrado: {filename}")

# Analizar los tres tests
test1 = r'c:\Users\atama\Documents\200.SPECTRUM\500. Proyectos\PowaDCR - General\powadcr_recorder\Xevious_test1.tzx'
test2 = r'c:\Users\atama\Documents\200.SPECTRUM\500. Proyectos\PowaDCR - General\powadcr_recorder\Xevious_test2.tzx'
test3 = r'c:\Users\atama\Documents\200.SPECTRUM\500. Proyectos\PowaDCR - General\powadcr_recorder\Xevious_test3.tzx'

for test in [test1, test2, test3]:
    if os.path.exists(test):
        analyze_tzx(test)
    else:
        print(f"❌ No existe: {test}")
