"""Delete ChromaDB collections whose name starts with a scene-id prefix.

Called by reset.bat when clearing a single scenario. Kept as a file (not an
inline `python -c`) because cmd's parenthesized if/else blocks choke on the
parentheses in Python code.

Usage: reset_chroma.py <chroma_path> <scene_prefix>
"""

import sys

import chromadb


def main() -> None:
    chroma_path, prefix = sys.argv[1], sys.argv[2]
    client = chromadb.PersistentClient(path=chroma_path)
    deleted = []
    for col in client.list_collections():
        if col.name.startswith(prefix):
            client.delete_collection(col.name)
            deleted.append(col.name)
    if deleted:
        print(f"  Deleted {len(deleted)} collection(s): {deleted}")
    else:
        print("  No matching collections found.")


if __name__ == "__main__":
    main()
