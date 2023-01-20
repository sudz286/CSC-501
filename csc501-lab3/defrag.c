#include <stdio.h> 
#include <sys/types.h> 
#include <sys/stat.h> 
#include <unistd.h> 
#include <errno.h> 
#include <string.h> 
#include <fcntl.h> 
#include <stdlib.h>

#define N_DBLOCKS 10 
#define N_IBLOCKS 4 
#define TRUE 1
#define FALSE 0

int debug_mode = FALSE;
// Variable to keep track of current block
int current_db = 0;


struct superblock {
    int blocksize; /* size of blocks in bytes */
    int inode_offset; /* offset of inode region in blocks */
    int data_offset; /* data region offset in blocks */
    int swap_offset; /* swap region offset in blocks */
    int free_inode; /* head of free inode list */
    int free_block; /* head of free block list */
};

struct inode {  
      int next_inode; /* list for free inodes */  
      int protect;        /*  protection field */ 
      int nlink;  /* Number of links to this file */ 
      int size;  /* Number of bytes in file */   
      int uid;   /* Owner's user ID */  
      int gid;   /* Owner's group ID */  
      int ctime;  /* Time field */ // Useless 
      int mtime;  /* Time field */  // Useless
      int atime;  /* Time field */  // Useless
      int dblocks[N_DBLOCKS];   /* Pointers to data blocks */  
      int iblocks[N_IBLOCKS];   /* Pointers to indirect blocks */  
      int i2block;     /* Pointer to doubly indirect block */  
      int i3block;     /* Pointer to triply indirect block */  
};


void read_superblock(struct superblock *sb, unsigned char *buffer )
{
    sb->blocksize =  *((int *) &(buffer[512]));
    sb->inode_offset=  *((int *) &(buffer[512 + 4]));
    sb->data_offset =  *((int *) &(buffer[512 + 8]));
    sb->swap_offset =  *((int *) &(buffer[512 + 12]));
    sb->free_inode =  *((int *) &(buffer[512 + 16]));
    sb->free_block =  *((int *) &(buffer[512 + 20]));
}

void display_superblock(struct superblock * sb)
{
    printf("Blocksize: %d \n", sb->blocksize);
    printf("Inode Offset: %d \n", sb->inode_offset);
    printf("Data offset: %d \n", sb->data_offset);
    printf("Swap Offset: %d \n", sb->swap_offset);
    printf("Free Inode: %d \n", sb->free_inode);
    printf("Free Block: %d \n", sb->free_block);
}

void read_inodes(struct inode in[], struct superblock * sb, unsigned char *buffer)
{
    int i = 0, inode_num = 0;
    int data_block_base_addr = (1024 + (sb->blocksize * sb->data_offset));
    int inode_base = (1024 + (sb->blocksize * sb->inode_offset));

    int num_inodes = (data_block_base_addr - inode_base) / 100;
    // printf("%d \n", num_inodes);

    for(inode_num = 0; inode_num < num_inodes; inode_num++)
    {
        int inode_base_addr =  inode_base + ( 100 * inode_num );

        in[inode_num].next_inode = *((int *) &(buffer[inode_base_addr]));
        in[inode_num].protect = *((int *)&(buffer[inode_base_addr + 4]));
        in[inode_num].nlink = *((int *)&(buffer[inode_base_addr + 8]));
        in[inode_num].size = *((int *)&(buffer[inode_base_addr + 12]));
        in[inode_num].uid = *((int *)&(buffer[inode_base_addr + 16]));
        in[inode_num].gid = *((int *)&(buffer[inode_base_addr + 20]));
        in[inode_num].ctime = *((int *)&(buffer[inode_base_addr + 24]));
        in[inode_num].mtime = *((int *)&(buffer[inode_base_addr + 28]));
        in[inode_num].atime = *((int *)&(buffer[inode_base_addr + 32]));
        
    //    printf("Next Inode: %d \n", in[inode_num].next_inode);
        for(int i =0;i < N_DBLOCKS;i++)
        {
            in[inode_num].dblocks[i] = *((int *)&(buffer[inode_base_addr + 36 + (4 * i)]));
        }

        for(i = 0; i < N_IBLOCKS;i++)
        {
            in[inode_num].iblocks[i] = *((int *)&(buffer[inode_base_addr + 76 + (4 * i) ])); 
        }

        in[inode_num].i2block = *((int *)&(buffer[inode_base_addr + 92]));
        in[inode_num].i3block = *((int *)&(buffer[inode_base_addr + 96]));
    }
}

void display_used_inodes(struct inode in[], int num_inodes)
{
    int inode_num = 0;
    int used_inode_count = 0, free_inode_count = 0;
    

    for(inode_num = 0; inode_num < num_inodes; inode_num++)
    {
        // if(!in[inode_num].nlink){ continue;}
        if(in[inode_num].next_inode == 0){ used_inode_count++;}else{ free_inode_count++;}

        printf("Next Inode: %d \n", in[inode_num].next_inode);
        printf("Protect: %d \n", in[inode_num].protect);
        printf("nlink: %d \n", in[inode_num].nlink);
        printf("size: %d \n", in[inode_num].size);
        printf("uid: %d \n", in[inode_num].uid);
        printf("gid: %d \n", in[inode_num].gid);
        printf("ctime: %d \n", in[inode_num].ctime);
        printf("mtime: %d \n", in[inode_num].mtime);
        printf("atime: %d \n", in[inode_num].atime);

        int i = 0;

        for(i =0;i < N_DBLOCKS;i++)
        {
            printf("Data Block[%d]: %d \n", i, in[inode_num].dblocks[i]);
        }
        for(i =0;i < N_IBLOCKS;i++)
        {
            printf("Indirect Block[%d]: %d \n", i, in[inode_num].iblocks[i]);
        }

        printf("i2block: %d \n", in[inode_num].i2block);
        printf("i3block: %d \n", in[inode_num].i3block);
    }

}

void traverse_free_block_list(int free_block_start, int data_block_base_addr, unsigned char * buffer, int blocksize)
{
    // Take free block no and data_block_base_addr as argument and perform traversal until -1
    // free_block_start = 1;
    int next_free_block;
    int count = 0;
    while(free_block_start != -1)
    {
        int data_block_addr = data_block_base_addr + (free_block_start * blocksize);
        // printf("%d \n", free_block_start);
        next_free_block = *((int *) &(buffer[data_block_addr]));
        
        printf("%d \n", next_free_block);

        free_block_start = next_free_block;
        
    }

}

void define_free_block_list(int old_free_block_end,  int new_free_block_end, int data_block_base_addr,  int blocksize, unsigned char * buffer, unsigned char * new_buffer)
{    

    while(new_free_block_end != old_free_block_end)
    {
        int data_block_addr = data_block_base_addr + (new_free_block_end * blocksize);
        new_free_block_end++;
        memcpy(new_buffer + data_block_addr, (unsigned char *)&new_free_block_end, sizeof(unsigned char *));
    }
    int x = -1;
    // printf("%d", new_free_block_end);
    memcpy(new_buffer + data_block_base_addr + (new_free_block_end * blocksize), (unsigned char *)&x, sizeof(unsigned char *));

    if(debug_mode)
    {
        // traverse_free_block_list(2136, data_block_base_addr, new_buffer, blocksize);
    }
   
}

void copy_data_blocks(struct superblock * sb, struct inode in[], unsigned char * buffer)
{
    unsigned char * new_buffer = malloc(10 * 1024 * 1024);
    int data_block_base_addr = (1024 + (sb->blocksize * sb->data_offset));
    int inode_base = (1024 + (sb->blocksize * sb->inode_offset));

    /* NUMBER OF INODES */
    int num_inodes = (data_block_base_addr - inode_base) / 100;

    int i = 0, j= 0;

    for(i = 0; i < num_inodes; i++)
    {
        // READ DIRECT BLOCKS
        if(!in[i].next_inode && in[i].nlink)
        {
            int k = 0;

            for(k = 0; k < N_DBLOCKS; k++)
            {
                if(in[i].dblocks[k] != -1)
                {
                    int block_to_be_freed = in[i].dblocks[k];


                    int src_position = data_block_base_addr + ( in[i].dblocks[k] * sb->blocksize);
                    int dest_position = data_block_base_addr + (current_db * sb->blocksize);

                    memcpy(new_buffer + dest_position, buffer + src_position, sb->blocksize);

                    // Modify Inode structure
                    in[i].dblocks[k] = current_db++;

                }   
            }
        }
        // READ INDIRECT BLOCK
        for(j = 0; j < N_IBLOCKS; j++)
        {

            if(in[i].iblocks[j] != -1)
            {
                // Copy indirect block from old to new buffer
                int src_position = data_block_base_addr + (in[i].iblocks[j] * sb->blocksize);
                int dest_position = data_block_base_addr + (current_db * sb->blocksize);
                
                memcpy(new_buffer + dest_position, buffer + src_position, sb->blocksize);
                
                int indirect_block_ptr = current_db;
                in[i].iblocks[j] = current_db;
                current_db++;

                int indirect_block_iterator = *((int *)&(buffer[src_position]));
                int max_pointers = sb->blocksize / 4;
                int itr = 0;
                while(itr < max_pointers && indirect_block_iterator != -1 && indirect_block_iterator != 0)
                {
                    int src_ib_position = data_block_base_addr + (indirect_block_iterator * sb->blocksize);
                    int dest_ib_position = data_block_base_addr + (current_db * sb->blocksize);

                    // Copy Indirect block and modify index to that block
                    memcpy(new_buffer + dest_ib_position, buffer + src_ib_position, sb->blocksize);
                    memcpy(new_buffer + dest_position, (unsigned char *)&current_db, sizeof(unsigned char*));
                    current_db++;
                    indirect_block_iterator = *((int *) & (buffer[src_position + 4]));
                    src_position += 4;
                    dest_position += 4;
                    itr++;
                }

            }
        }

        // READ DOUBLY INDIRECT BLOCK
        if(in[i].i2block != -1)
        {
            // Copy Doubly indirect block from old to new buffer
            int i2src_position = data_block_base_addr + (in[i].i2block * sb->blocksize);
            int i2dest_position = data_block_base_addr + (current_db * sb->blocksize);

            memcpy(new_buffer + i2dest_position, buffer + i2src_position, sb->blocksize);

            in[i].i2block = current_db;
            current_db++;

            int i2parent_iterator =  *((int *)&(buffer[i2src_position]));
            int max_i2_pointers = sb->blocksize / 4;
            int parent_itr = 0;

            while(parent_itr < max_i2_pointers && i2parent_iterator != -1 && i2parent_iterator != 0)
            {
                // Copy parent index block
                int src_parent_i2_position = data_block_base_addr + (i2parent_iterator * sb->blocksize);
                int dest_parent_i2_position = data_block_base_addr + (current_db * sb->blocksize);

                memcpy(new_buffer + dest_parent_i2_position, buffer + src_parent_i2_position, sb->blocksize);
                memcpy(new_buffer + i2dest_position, (unsigned char *)&current_db, sizeof(unsigned char *));
                
                // printf("Parent Block[%d]: %d \n", current_db, i2parent_iterator);
                current_db++;

                int i2child_iterator = *((int *)&(buffer[src_parent_i2_position]));
                int child_itr = 0;
                
                while(child_itr < max_i2_pointers && i2child_iterator != -1 && i2child_iterator != 0)
                {

                    // Copy child index block
                    int src_child_i2_position = data_block_base_addr + (i2child_iterator * sb->blocksize);
                    int dest_child_i2_position = data_block_base_addr + (current_db * sb->blocksize);

                    memcpy(new_buffer + dest_child_i2_position, buffer  + src_child_i2_position, sb->blocksize);
                    memcpy(new_buffer + dest_parent_i2_position, (unsigned char *)&current_db, sizeof(unsigned char *));

                    // if(count  < 4000){ printf("Child Block[%d]: %d\n", current_db, i2child_iterator); count++;}
                    current_db++;

                    i2child_iterator =  *((int *)&(buffer[src_parent_i2_position + 4]));

                    src_parent_i2_position += 4;
                    dest_parent_i2_position += 4;
                    child_itr++;
                }
                
                i2parent_iterator = *((int *)&(buffer[i2src_position + 4]));

                i2src_position += 4;
                i2dest_position += 4; 
                parent_itr++;               
            }
        }

        // READ TRIPLY INDIRECT BLOCK
        if(in[i].i3block != -1)
        {
            // Copy Triply indirect block from old to new buffer
            int i3src_position = data_block_base_addr + (in[i].i3block * sb->blocksize);
            int i3dest_position = data_block_base_addr + (current_db * sb->blocksize);

            memcpy(new_buffer + i3dest_position, buffer + i3src_position, sb->blocksize);

            in[i].i3block = current_db;
            current_db++;

            int i3parent_iterator =  *((int *)&(buffer[i3src_position]));
            int max_i3_pointers = sb->blocksize / 4;
            int parent_itr = 0;

            while(parent_itr < max_i3_pointers && i3parent_iterator != -1 && i3parent_iterator != 0)
            {
                // Copy parent index block
                int src_parent_i3_position = data_block_base_addr + (i3parent_iterator * sb->blocksize);
                int dest_parent_i3_position = data_block_base_addr + (current_db * sb->blocksize);

                memcpy(new_buffer + dest_parent_i3_position, buffer + src_parent_i3_position, sb->blocksize);
                memcpy(new_buffer + i3dest_position, (unsigned char *)&current_db, sizeof(unsigned char *));

                current_db++;

                int i3child_iterator = *((int *)&(buffer[src_parent_i3_position]));
                int child_itr = 0;
                
                while(child_itr < max_i3_pointers && i3child_iterator != -1 && i3child_iterator != 0)
                {

                    // Copy child index block
                    int src_child_i3_position = data_block_base_addr + (i3child_iterator * sb->blocksize);
                    int dest_child_i3_position = data_block_base_addr + (current_db * sb->blocksize);

                    memcpy(new_buffer + dest_child_i3_position, buffer  + src_child_i3_position, sb->blocksize);
                    memcpy(new_buffer + dest_parent_i3_position, (unsigned char *)&current_db, sizeof(unsigned char *));

                    current_db++;

                    int i3final_iterator = *((int *)&(buffer[src_child_i3_position]));
                    int final_itr = 0;

                    while(final_itr < max_i3_pointers && i3final_iterator != -1 && i3final_iterator != 0)
                    {
                        // Copy innermost block
                        int src_final_i3_position = data_block_base_addr + (i3final_iterator * sb->blocksize);
                        int dest_final_i3_position = data_block_base_addr + (current_db * sb->blocksize);

                        memcpy(new_buffer + dest_final_i3_position, buffer + src_final_i3_position, sb->blocksize);
                        memcpy(new_buffer + dest_child_i3_position, (unsigned char *)&current_db, sizeof(unsigned char *));

                        current_db++;

                        i3final_iterator =  *((int *)&(buffer[src_child_i3_position + 4]));

                        src_child_i3_position += 4;
                        dest_child_i3_position += 4;
                        final_itr++;
                    }

                    i3child_iterator =  *((int *)&(buffer[src_parent_i3_position + 4]));

                    src_parent_i3_position += 4;
                    dest_parent_i3_position += 4;
                    child_itr++;
                }
                
                i3parent_iterator = *((int *)&(buffer[i3src_position + 4 ]));

                i3src_position += 4;
                i3dest_position += 4; 
                parent_itr++;               
            }
        }

    }

    if(debug_mode)
    {
        printf("%d \n", current_db);
    }

    // Write SuperBlock and bootblock to new_buffer
    memcpy(new_buffer, buffer, 532);
    memcpy(new_buffer + 532, (unsigned char *)&current_db, sizeof(unsigned char *));
    memcpy(new_buffer + 512 + 24, buffer + 512 + 24, 488);

    // Logic to write Inode to new_buffer
    for(i = 0;i < num_inodes;i++)
    {
        int inode_base_addr = inode_base + (100 * i);
        memcpy(new_buffer + inode_base_addr, (unsigned char *)&in[i].next_inode, sizeof(unsigned char *));
        memcpy(new_buffer + inode_base_addr + 4, (unsigned char *)&in[i].protect, sizeof(unsigned char* ));
        memcpy(new_buffer + inode_base_addr + 8, (unsigned char *)&in[i].nlink, sizeof(unsigned char* ));
        memcpy(new_buffer + inode_base_addr + 12, (unsigned char *)&in[i].size, sizeof(unsigned char* ));
        memcpy(new_buffer + inode_base_addr + 16, (unsigned char *)&in[i].uid, sizeof(unsigned char* ));
        memcpy(new_buffer + inode_base_addr + 20, (unsigned char *)&in[i].gid, sizeof(unsigned char* ));
        memcpy(new_buffer + inode_base_addr + 24, (unsigned char *)&in[i].ctime, sizeof(unsigned char* ));
        memcpy(new_buffer + inode_base_addr + 28, (unsigned char *)&in[i].mtime, sizeof(unsigned char* ));
        memcpy(new_buffer + inode_base_addr + 32, (unsigned char *)&in[i].atime, sizeof(unsigned char* ));

        int j = 0;
        for(j = 0; j< N_DBLOCKS;j++)
        {
            memcpy(new_buffer+ inode_base_addr + 36 + (4 * j), (unsigned char*)&in[i].dblocks[j], sizeof(unsigned char *));
        }

        for(j = 0; j< N_IBLOCKS;j++)
        {
            memcpy(new_buffer+ inode_base_addr + 76 + (4 * j), (unsigned char*)&in[i].iblocks[j], sizeof(unsigned char *));
        }
        memcpy(new_buffer + inode_base_addr + 92, (unsigned char *)&in[i].i2block, sizeof(unsigned char* ));
        memcpy(new_buffer + inode_base_addr + 96, (unsigned char *)&in[i].i3block, sizeof(unsigned char* ));
    }

    // Remaining bytes until Data region need to be zero
    int non_zero_inode_region = inode_base + (num_inodes * 100);
    int inode_rem_bytes = data_block_base_addr - non_zero_inode_region;

    memcpy(new_buffer + non_zero_inode_region, malloc(inode_rem_bytes) ,inode_rem_bytes);
    // Modify Free List and write to buffer
    define_free_block_list(sb->free_block, current_db, data_block_base_addr, sb->blocksize, buffer, new_buffer);

    // Open output file and write new_buffer
    FILE *of;
    of = fopen("disk_defrag", "w");
    fwrite(new_buffer, 10 * 1024 * 1024 * 1, 1, of);

    // Debugging
    if(debug_mode)
    {
        read_superblock(&sb, new_buffer);
        display_superblock(&sb);
        read_inodes(in,&sb,new_buffer);
        display_used_inodes(in, num_inodes);
    }

}

void main(int argc, char * argv[])
{
    FILE *f;
    unsigned char * buffer;
    struct stat st;
    size_t bytes;
    char *filename;
    buffer = malloc(10 * 1024 * 1024);
    if(argc < 2)
    {
        printf("Missing Filename");
        exit (1);
    }
    else
    {
        printf("Opening the file \n");
        filename = argv[1];
        f = fopen(filename,"r");

    }
    bytes =  fread(buffer,10*1024*1024,1,f);
    struct superblock sb;
    read_superblock(&sb, buffer);

    int data_block_base_addr = (1024 + (sb.blocksize * sb.data_offset));
    int inode_base = (1024 + (sb.blocksize * sb.inode_offset));
    int num_inodes = (data_block_base_addr - inode_base) / 100;
    struct inode in[num_inodes];
    
    read_inodes(&in, &sb, buffer);
    
    if(debug_mode)
    {
        display_superblock(&sb);
        display_used_inodes(&in, num_inodes);
    }

    copy_data_blocks(&sb, &in, buffer);
    printf("Disk defragmented and data copied to disk_defrag \n");
}