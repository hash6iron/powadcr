# 
# Creates/Updates the Arduino library from the original source code
#
import os
import shutil
import subprocess

def setup_ogg():
    # Ensure original/ogg exists by cloning if necessary
    original_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), '../original'))
    ogg_dir = os.path.join(original_dir, 'ogg')

    # Source directories
    include_src = os.path.join(ogg_dir, 'include')
    src_src = os.path.join(ogg_dir, 'src')

    # Destination directory
    project_root = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
    dest_dir = os.path.join(project_root, 'src', 'ogg')
    if not os.path.exists(dest_dir):
        os.makedirs(dest_dir)

    # Copy all header files from include
    for root, dirs, files in os.walk(include_src):
        for file in files:
            if file.endswith('.h'):
                src_file = os.path.join(root, file)
                shutil.copy2(src_file, dest_dir)


    # Copy all .c and .h files from src
    for file in os.listdir(src_src):
        if file.endswith('.c') or file.endswith('.h'):
            src_file = os.path.join(src_src, file)
            shutil.copy2(src_file, dest_dir)

    print("Ogg files copied.")

    # Create missing ogg/config_types.h if not present
    config_types_path = os.path.join(dest_dir, 'config_types.h')
    if not os.path.exists(config_types_path):
        with open(config_types_path, 'w', encoding='utf-8') as f:
            f.write('// Auto-generated minimal config_types.h for Arduino build\n')
            f.write('#ifndef OGG_CONFIG_TYPES_H\n#define OGG_CONFIG_TYPES_H\n\n')
            f.write('#include <stdint.h>\n\n')
            f.write('typedef int16_t ogg_int16_t;\n')
            f.write('typedef uint16_t ogg_uint16_t;\n')
            f.write('typedef int32_t ogg_int32_t;\n')
            f.write('typedef uint32_t ogg_uint32_t;\n')
            f.write('typedef int64_t ogg_int64_t;\n')
            f.write('typedef uint64_t ogg_uint64_t;\n')
            f.write('\n#endif // OGG_CONFIG_TYPES_H\n')
        print("Created missing ogg/config_types.h.")

def setup_oggz():
    # Ensure original/oggz exists by cloning if necessary
    original_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), '../original'))
    oggz_dir = os.path.join(original_dir, 'oggz')
    if not os.path.exists(oggz_dir):
        print(f"Cloning oggz repository into {original_dir}...")
        subprocess.run(["git", "clone", "https://gitlab.xiph.org/xiph/liboggz.git", oggz_dir], check=True)
    # Checkout latest tag
    result = subprocess.run(["git", "-C", oggz_dir, "tag"], capture_output=True, text=True)
    tags = result.stdout.strip().split('\n')
    if tags:
        latest_tag = sorted(tags, key=lambda s: [int(x) if x.isdigit() else x for x in s.replace('v','').replace('-','.').split('.')], reverse=True)[0]
        print(f"Checking out latest tag {latest_tag} in oggz...")
        subprocess.run(["git", "-C", oggz_dir, "checkout", latest_tag], check=True)

    # Source directory for liboggz
    liboggz_src = os.path.join(oggz_dir, 'src', 'liboggz')
    include_oggz_src = os.path.join(oggz_dir, 'include', 'oggz')

    # Destination directories
    project_root = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
    dest_liboggz_dir = os.path.join(project_root, 'src', 'oggz', 'liboggz')
    dest_oggz_dir = os.path.join(project_root, 'src', 'oggz')
    if not os.path.exists(dest_liboggz_dir):
        os.makedirs(dest_liboggz_dir)
    if not os.path.exists(dest_oggz_dir):
        os.makedirs(dest_oggz_dir)

    # Generate config.h in src/oggz immediately after directory creation
    config_h_path = os.path.join(dest_oggz_dir, 'config.h')
    with open(config_h_path, 'w', encoding='utf-8') as f:
        f.write('#pragma once\n\n')
        f.write('#define OGGZ_CONFIG_READ 1\n')
        f.write('#define OGGZ_CONFIG_WRITE 1\n')
    print("Generated config.h in src/oggz.")

    # Copy all .h and .c files from liboggz_src
    if os.path.exists(liboggz_src):
        for file in os.listdir(liboggz_src):
            if file.endswith('.c') or file.endswith('.h'):
                src_file = os.path.join(liboggz_src, file)
                shutil.copy2(src_file, dest_liboggz_dir)

    # Copy all header files from include/oggz
    if os.path.exists(include_oggz_src):
        for file in os.listdir(include_oggz_src):
            if file.endswith('.h'):
                src_file = os.path.join(include_oggz_src, file)
                shutil.copy2(src_file, dest_oggz_dir)

    print("oggz files copied.")

    # Generate oggz_off_t_generated.h in src/oggz
    oggz_off_t_path = os.path.join(dest_oggz_dir, 'oggz_off_t_generated.h')
    with open(oggz_off_t_path, 'w', encoding='utf-8') as f:
        f.write('#ifndef __OGGZ_OFF_T_GENERATED_H__\n')
        f.write('#define __OGGZ_OFF_T_GENERATED_H__\n\n')
        f.write('#include <sys/types.h>\n')
        f.write('typedef long oggz_off_t;\n\n')
        f.write('#define PRI_OGGZ_OFF_T "ll"\n\n')
        f.write('#endif /* __OGGZ_OFF_T_GENERATED__ */\n')
    print("Generated oggz_off_t_generated.h.")

def patch_includes(dest_dir):
    """Patch #include statements in .c and .h files in dest_dir."""
    # Build a map of all .h file locations (filename -> relative path from src)
    header_locations = {}
    src_root = os.path.dirname(dest_dir)
    for root, dirs, files in os.walk(src_root):
        for file in files:
            if file.endswith('.h'):
                rel_dir = os.path.relpath(root, src_root)
                rel_path = os.path.join(rel_dir, file) if rel_dir != '.' else file
                header_locations[file] = rel_path.replace('\\', '/')

    # Patch all .c and .h files recursively in dest_dir
    for root, dirs, files in os.walk(dest_dir):
        for fname in files:
            if fname.endswith('.c') or fname.endswith('.h'):
                file_path = os.path.join(root, fname)
                with open(file_path, 'r', encoding='utf-8') as f:
                    lines = f.readlines()
                new_lines = []
                changed = False
                for line in lines:
                    if line.strip().startswith('#include "'):
                        start = line.find('"') + 1
                        end = line.find('"', start)
                        inc_file = line[start:end]
                        inc_base = os.path.basename(inc_file)
                        # Check if inc_file is in the same directory as the including file
                        inc_path = os.path.join(root, inc_file)
                        if os.path.exists(inc_path):
                            # File exists in current directory, do not patch
                            pass
                        elif inc_base in header_locations:
                            new_path = header_locations[inc_base]
                            if inc_file != new_path:
                                line = line.replace(inc_file, new_path)
                                changed = True
                    new_lines.append(line)
                if changed:
                    with open(file_path, 'w', encoding='utf-8') as f:
                        f.writelines(new_lines)

if __name__ == "__main__":
    setup_ogg()
    setup_oggz()
    project_root = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
    patch_includes(os.path.join(project_root, 'src', 'ogg'))
    patch_includes(os.path.join(project_root, 'src', 'oggz'))
