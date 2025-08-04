#!/usr/bin/env python3
"""
File System Monitor using watchdog
Monitors all file changes under a specified directory
"""

import os
import csv
import sys
import time
import argparse
from pathlib import Path
from watchdog.observers import Observer
from watchdog.events import FileSystemEventHandler, FileSystemEvent, FileCreatedEvent, FileDeletedEvent

# entry to store metadata (next asset id, if any asset is dirty)
META = "__metadata__"

# file extensions to keep track of (others are ignored)
TRACK_EXTS = [ 
    ".txt", 
    ".png",
    ".hlsl",
    ".glb",
    ".gltf"
]


class FileChangeHandler(FileSystemEventHandler):
    """Handles file system events"""

    def __init__(self, directory, index_path):
        self.update_queue: list[FileSystemEvent] = []
        self.root_path = os.path.abspath(directory) + os.sep
        self.index_path = index_path

    def on_created(self, event):
        if not event.is_directory and os.path.splitext(event.src_path)[1] in TRACK_EXTS:
            event.src_path = event.src_path.removeprefix(self.root_path)
            self.update_queue.append(event)
            print(f"[CREATED] \t{event.src_path}")

    def on_modified(self, event):
        if not event.is_directory and os.path.splitext(event.src_path)[1] in TRACK_EXTS:
            event.src_path = event.src_path.removeprefix(self.root_path)
            self.update_queue.append(event)
            print(f"[MODIFIED] \t{event.src_path}")
    
    def on_deleted(self, event):
        event.src_path = event.src_path.removeprefix(self.root_path)
        self.update_queue.append(event)
        print(f"[DELETED] \t{event.src_path}")
    
    def on_moved(self, event):
        if not event.is_directory:
            track_src = os.path.splitext(event.src_path)[1] in TRACK_EXTS
            track_dest = os.path.splitext(event.dest_path)[1] in TRACK_EXTS

            if track_src and track_dest:
                event.src_path = event.src_path.removeprefix(self.root_path)
                event.dest_path = event.dest_path.removeprefix(self.root_path)
                self.update_queue.append(event)
                print(f"[MOVED] \t{event.src_path} -> {event.dest_path}")
            elif track_src:
                self.on_deleted(FileDeletedEvent(event.src_path, is_synthetic=True))
            elif track_dest:
                self.on_created(FileCreatedEvent(event.dest_path, is_synthetic=True))

    def make_index_map(self):
        index_map = { META : { 'id' : "0", 'dirty' : "0" } }

        if not os.path.exists(self.index_path):
            return index_map
        
        with open(self.index_path, 'r') as file:
            index_map = { str(row['path']) : { 'id' : row['id'], 'dirty' : row['dirty'] } for row in csv.DictReader(file) }

        return index_map

    def initial_sync(self):
        index_map = self.make_index_map()
        
        current_files = set()
        for root, dirs, files in os.walk(self.root_path):
            for file in files:
                if os.path.splitext(file)[1] in TRACK_EXTS:
                    current_files.add(str(os.path.join(root, file)))
        
        indexed_files = { str(os.path.join(self.root_path, i)) for i in index_map.keys() if i != META }
        
        for missing_file in current_files - indexed_files:
            self.on_created(FileCreatedEvent(missing_file, is_synthetic=True))

        for orphaned_file in indexed_files - current_files:
            self.on_deleted(FileDeletedEvent(orphaned_file, is_synthetic=True))

    def process_events(self):

        if not self.update_queue:
            return

        index_map = self.make_index_map()
            
        next_id = int(index_map[META]['id'])
        index_map[META]['dirty'] = "1"

        for event in self.update_queue:

            match event.event_type:
                case "created":
                    index_map[event.src_path] = { 'id' : str(next_id), 'dirty' : "1" }
                    next_id += 1

                case "modified":
                    if event.src_path in index_map:
                        index_map[event.src_path]['dirty'] = "1"
                    else:
                        index_map[event.src_path] = { 'id' : str(next_id), 'dirty' : "1" }
                        next_id += 1

                case "moved":
                    elem = index_map.pop(event.src_path, None)
                    if elem:
                        index_map[event.dest_path] = elem
                    else:
                        index_map[event.dest_path] = { 'id' : str(next_id), 'dirty' : "1" }
                        next_id += 1

                case "deleted":
                    if event.is_directory:
                        dir_prefix = event.src_path + os.sep
                        index_map = { path : value for path, value in index_map.items() if not path.startswith(dir_prefix) }
                    else:
                        index_map.pop(event.src_path, None)

                case _:
                    pass

        index_map[META]['id'] = str(next_id)
        self.update_queue.clear()

        with open(self.index_path, 'w', newline='') as file:
            writer = csv.DictWriter(file, fieldnames=['id', 'path', 'dirty'])
            writer.writeheader()
            writer.writerows([{'id' : value['id'], 'path' : path, 'dirty' : value['dirty']} for path, value in index_map.items()])

def main():
    parser = argparse.ArgumentParser(description='Monitor file system changes')
    parser.add_argument('directory', help='Directory to monitor')
    parser.add_argument('index', help='File save monitor info to')
    
    args = parser.parse_args()
    
    # Validate directory
    watch_dir = Path(args.directory).resolve()
    if not watch_dir.exists():
        print(f"Error: Directory '{watch_dir}' does not exist")
        sys.exit(1)
    
    if not watch_dir.is_dir():
        print(f"Error: '{watch_dir}' is not a directory")
        sys.exit(1)
    
    # Set up monitoring
    event_handler = FileChangeHandler(args.directory, args.index)
    observer = Observer()
    observer.schedule(event_handler, str(watch_dir), recursive=True)
    
    # Start monitoring
    print(f"Monitoring directory: {watch_dir}")
    print("Press Ctrl+C to stop...\n")
    
    observer.start()
    
    try:
        event_handler.initial_sync()
        while True:
            time.sleep(1)
            event_handler.process_events()

    except KeyboardInterrupt:
        observer.stop()
        observer.join()
    
    except Exception as e:
        print("\n", e)
        observer.stop()
        observer.join()
        input("Press return to close...")

if __name__ == "__main__":
    main()