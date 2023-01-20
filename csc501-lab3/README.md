# README.md
## File Structure
```
Disk-Defragmenter
│   README.md
│   Makefile
|   defrag
│   defrag.c
└───disk
│   │   disk_defrag
│   │   disk_defrag_3
│   │   disk_defrag_2
|   |   disk_defrag_1
|   |   disk_frag_0
|   |   disk_frag_1
|   |   disk_frag_2
|   |   disk_frag_3
|   |   disk_pairs.zip
```
## Instructions

1. Run `make` in the root directory.
2. Makefile creates an executable defrag that takes in the defragmented diskfile name as a CLA. 
3. Run `make run FILE_NAME="disk/disk_frag_1"`
    or
   Run `./defrag disk/disk_frag_1` generates the defragemented disk image `disk_defrag` in the parent directory