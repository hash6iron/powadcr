#!/usr/bin/env python3

# 
# Deletes all tags
#

import subprocess

# Get all local tags
def get_local_tags():
    result = subprocess.run(['git', 'tag'], stdout=subprocess.PIPE, text=True, check=True)
    return [tag.strip() for tag in result.stdout.splitlines() if tag.strip()]

def main():
    tags = get_local_tags()
    if not tags:
        print("No tags found.")
        return
    for tag in tags:
        print(f"Deleting local tag: {tag}")
        subprocess.run(['git', 'tag', '-d', tag], check=True)
        print(f"Deleting remote tag: {tag}")
        subprocess.run(['git', 'push', '--delete', 'origin', tag], check=True)
    print("All tags deleted locally and remotely.")

if __name__ == '__main__':
    main()
