#!/usr/bin/env python3

# 
# Commits the open changes and creates a new tag
#

import sys
import subprocess

MAIN_BRANCH = "main"

def commit(tag_name):
    # Commit all changes with the tag name as the commit message
    try:
        subprocess.run(['git', 'add', '--all'], check=True)
        subprocess.run(['git', 'commit', '-m', tag_name], check=True)
        push_cmd = ["git", "push", "origin", MAIN_BRANCH]
        subprocess.run(push_cmd, check=True)
        print(f"Tag 'main' pushed to origin.")
        return True
    except subprocess.CalledProcessError:
        print(f"Failed to commit tag")
        return False

def create_tag(tag_name):
    tag_cmd = ["git", "tag", tag_name]
    try:
        subprocess.run(tag_cmd, check=True)
        print(f"Tag '{tag_name}' created.")
        return True
    except subprocess.CalledProcessError:
        print(f"Failed to create tag '{tag_name}'.")
        return False


def push_tag(tag_name):
    push_cmd = ["git", "push", "origin", tag_name]
    try:
        subprocess.run(push_cmd, check=True)
        print(f"Tag '{tag_name}' pushed to origin.")
        return True
    except subprocess.CalledProcessError:
        print(f"Failed to push tag '{tag_name}' to origin.")
        return False


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python create_and_push_tag.py <tag_name>")
        sys.exit(1)

    tag_name = sys.argv[1]
    commit(tag_name)

    if not create_tag(tag_name):
        sys.exit(2)

    if not push_tag(tag_name):
        sys.exit(3)
    
    sys.exit(0)


