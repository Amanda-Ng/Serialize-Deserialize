# File and Directory Serialization and Deserialization
## Overview
This command-line utility is designed to serialize a tree of files and directories into a sequence of bytes, and to deserialize that sequence to reconstruct the original file tree. It can be used to "transplant" a directory structure from one location to another. 

The utility supports two main operations: serialization and deserialization. In serialization, the program traverses a directory structure and outputs its serialized form to stdout. In deserialization, it reads serialized data from stdin and reconstructs the corresponding files and directories.

## Features
- **Serialization**: Convert a file tree into a stream of serialized data.
- **Deserialization**: Rebuild a file tree from serialized data.
## Command-line interface: Supports flags for different operations and options.
- `-h`: Displays the help message and exits successfully.
- `-s`: Serializes the file tree and outputs it to stdout.
- `-d`: Deserializes data from stdin to recreate the file tree.
- `-c`: (Optional) Allows clobbering existing files during deserialization.
- `-p DIR`: (Optional) Specifies the directory for deserialization.
## Data Format
The serialized data consists of a series of records, each with a 16-byte header followed by data. The header format is as follows:

- Magic (3 bytes): 0x0C, 0x0D, 0xED
- Type (1 byte)
- Depth (4 bytes, unsigned 32-bit)
- Size (8 bytes, unsigned 64-bit)

<br>Record types include:
- START_OF_TRANSMISSION (type = 0)
- END_OF_TRANSMISSION (type = 1)
- START_OF_DIRECTORY (type = 2)
- END_OF_DIRECTORY (type = 3)
- DIRECTORY_ENTRY (type = 4)
- FILE_DATA (type = 5)
The serialized data begins with a START_OF_TRANSMISSION and ends with an END_OF_TRANSMISSION. Directory entries are enclosed by START_OF_DIRECTORY and END_OF_DIRECTORY records, with each directory's contents listed between these markers.

## Functionality
The program is divided into several key parts:

- **Argument Validation (validargs)**: Validates command-line arguments and sets global options.
- **Path Management (path_init, path_push, path_pop)**: Functions to manage and modify the current path during serialization and deserialization.
- **Serialization (serialize, serialize_file, serialize_directory)**: Handles the serialization of file and directory contents.
- **Deserialization (deserialize, deserialize_file, deserialize_directory)**: Handles the deserialization of file and directory structures.
## Program Usage
- Help Message
To display the help message:

`bin/transplant -h`
- Serialization
To serialize a directory structure to stdout:

`bin/transplant -s [-p DIR]`
Where:

-p DIR: (Optional) Specifies the directory to serialize.
- Deserialization
To deserialize data from stdin:

`bin/transplant -d [-c] [-p DIR]`
Where:

-c: (Optional) Clobbers existing files during deserialization.
-p DIR: (Optional) Specifies the directory to deserialize into.
