#include <stdio.h> 
#include <sys/types.h> 
#include <sys/stat.h> 
#include <unistd.h> 
#include <errno.h> 
#include <string.h> 
#include <fcntl.h> 
#include <stdlib.h>

/* Write all this into a h file */ 
#define N_DBLOCKS 10 
#define N_IBLOCKS 4 
#define TRUE 1
#define FALSE 0

int debug_mode = TRUE;

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

// Variable to keep track of current block
int current_db = 0;

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

void compare_data_block(struct superblock * sb, struct inode in[], unsigned char * buffer, unsigned char * buffer2)
{
    unsigned char * new_buffer = malloc(10 * 1024 * 1024);
    int data_block_base_addr = (1024 + (sb->blocksize * sb->data_offset));
    int inode_base = (1024 + (sb->blocksize * sb->inode_offset));

    /* NUMBER OF INODES */
    int num_inodes = (data_block_base_addr - inode_base) / 100;

    int i = 0, j= 0;

    for(i = 0; i < num_inodes; i++)
    {
        // printf("Inode Number: %d \n", i);
        if(!in[i].next_inode && in[i].nlink)
        {
            int k = 0;
            
            for(k = 0; k < N_DBLOCKS; k++)
            {
                if(in[i].dblocks[k] != -1)
                {
                    int block_to_be_freed = in[i].dblocks[k];
                    int dest_position = data_block_base_addr + (current_db * sb->blocksize);

                    // printf("CMP DDB# %d: %d \n", k, memcmp(buffer + dest_position, buffer2 + dest_position, sb->blocksize));
                                      
                    // Modify Inode structure
                    current_db++;
                    // in[i].dblocks[k] = current_db++;

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
                // int dest_position = data_block_base_addr + (current_db * sb->blocksize);
                
                // printf("IP Block %d: %d \n", j, memcmp(buffer + dest_position, buffer2 + dest_position, sb->blocksize));
                
                int indirect_block_ptr = current_db;
                // in[i].iblocks[j] = current_db;
                current_db++;

                int indirect_block_iterator = *((int *)&(buffer[src_position]));
                int max_pointers = sb->blocksize / 4;
                int itr = 0;
                while(itr < max_pointers && indirect_block_iterator != -1 && indirect_block_iterator != 0)
                {
                    int src_ib_position = data_block_base_addr + (indirect_block_iterator * sb->blocksize);
                    int dest_ib_position = data_block_base_addr + (current_db * sb->blocksize);

                    // Copy Indirect block and modify index to that block
                    // memcpy(new_buffer + dest_ib_position, buffer + src_ib_position, sb->blocksize);
                    if(memcmp(buffer + dest_ib_position, buffer2 + dest_ib_position, sb->blocksize))
                    {
                        printf("%d \n", i);
                        printf("Indirect Block %d: %d\n", j, memcmp(buffer + dest_ib_position, buffer2 + dest_ib_position, sb->blocksize));
                        // memcpy(new_buffer + dest_position, (unsigned char *)&current_db, sizeof(unsigned char*));
                        printf("Child %d \n", memcmp(buffer + dest_ib_position, buffer2 + dest_ib_position, sizeof(unsigned char *)));
                    }
                    
                    current_db++;
                    indirect_block_iterator = *((int *) & (buffer[src_position + 4]));
                    // src_position += 4;
                    src_position += 4;
                    itr++;
                }

            }
        }

        if(in[i].i2block != -1)
        {
            // Copy Doubly indirect block from old to new buffer
            int i2src_position = data_block_base_addr + (in[i].i2block * sb->blocksize);
            int i2dest_position = data_block_base_addr + (current_db * sb->blocksize);

            // memcpy(new_buffer + i2dest_position, buffer + i2src_position, sb->blocksize);
            // printf("Parent Block: %d \n",memcmp(buffer + i2dest_position, buffer2 + i2dest_position, sb->blocksize));
            // in[i].i2block = current_db;
            current_db++;

            int i2parent_iterator =  *((int *)&(buffer[i2src_position]));
            int max_i2_pointers = sb->blocksize / 4;
            int parent_itr = 0;

            while(parent_itr < max_i2_pointers && i2parent_iterator != -1 && i2parent_iterator != 0)
            {
                // Copy parent index block
                int src_parent_i2_position = data_block_base_addr + (i2parent_iterator * sb->blocksize);
                int dest_parent_i2_position = data_block_base_addr + (current_db * sb->blocksize);

                // memcpy(new_buffer + dest_parent_i2_position, buffer + src_parent_i2_position, sb->blocksize);
                // memcpy(new_buffer + i2dest_position, (unsigned char *)&current_db, sizeof(unsigned char *));
                
                // printf("Parent 2 Block: %d \n", memcmp(buffer + dest_parent_i2_position, buffer2 + dest_parent_i2_position, sb->blocksize));
                // printf("Parent Block[%d]: %d \n", current_db, i2parent_iterator);
                current_db++;

                int i2child_iterator = *((int *)&(buffer[src_parent_i2_position]));
                int child_itr = 0;
                
                while(child_itr < max_i2_pointers && i2child_iterator != -1 && i2child_iterator != 0)
                {

                    // Copy child index block
                    int src_child_i2_position = data_block_base_addr + (i2child_iterator * sb->blocksize);
                    int dest_child_i2_position = data_block_base_addr + (current_db * sb->blocksize);

                    // memcpy(new_buffer + dest_child_i2_position, buffer  + src_child_i2_position, sb->blocksize);
                    // memcpy(new_buffer + dest_parent_i2_position, (unsigned char *)&current_db, sizeof(unsigned char *));
                    
                    // printf("Child Block: %d \n", memcmp(buffer + dest_child_i2_position, buffer2  + dest_child_i2_position, sb->blocksize));
                    // if(count  < 4000){ printf("Child Block[%d]: %d\n", current_db, i2child_iterator); count++;}
                    current_db++;

                    i2child_iterator =  *((int *)&(buffer[src_parent_i2_position + 4]));

                    src_parent_i2_position += 4;
                    dest_parent_i2_position += 4;
                    child_itr++;
                }
                
                i2parent_iterator = *((int *)&(buffer[i2src_position +4 ]));

                i2src_position += 4;
                i2dest_position += 4; 
                parent_itr++;               
            }
        }
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

void main()
{
    FILE *f;
    unsigned char * buffer;
    struct stat st;
    size_t bytes;
    char *filename;
    buffer = malloc(10*1024*1024);

    f = fopen("disk/disk_defrag","r");

    bytes =  fread(buffer,10*1024*1024,1,f);
    struct superblock sb;
    read_superblock(&sb, buffer);
    // display_superblock(&sb);
    int data_block_base_addr = (1024 + (sb.blocksize * sb.data_offset));
    int inode_base = (1024 + (sb.blocksize * sb.inode_offset));
    int num_inodes = (data_block_base_addr - inode_base) / 100;
    struct inode in[num_inodes];
    
    read_inodes(&in, &sb, buffer);
    // display_used_inodes(&in, num_inodes);
    // traverse_free_block_list(sb.free_block, data_block_base_addr, buffer, sb.blocksize);
    // read_data_block(&sb, &in, buffer);
    // unsigned char * new_buffer = malloc(10 * 1024 * 1024);
    // define_free_block_list(sb.free_block, 2136, data_block_base_addr, sb.blocksize, buffer, new_buffer);
    

    FILE *f2;
    unsigned char * buffer2;
    struct stat st2;
    size_t bytes2;

    buffer2 = malloc(10 * 1024 * 1024);
    f2 = fopen("disk/disk_defrag_1","r");

    bytes2 =  fread(buffer2,10 *1024*1024,1,f2);
    struct superblock sb2;
    read_superblock(&sb2, buffer2);
    // display_superblock(&sb);
    int data_block_base_addr2 = (1024 + (sb2.blocksize * sb2.data_offset));
    int inode_base2 = (1024 + (sb2.blocksize * sb2.inode_offset));
    int num_inodes2 = (data_block_base_addr2- inode_base2) / 100;
    struct inode in2[num_inodes2];
    
    read_inodes(&in2, &sb2, buffer2);
    
    // traverse_free_block_list(sb.free_block, data_block_base_addr, buffer, sb.blocksize);
    // read_data_block(&sb2, &in2, buffer2);
    // unsigned char * new_buffer = malloc(10 * 1024 * 1024);
    // define_free_block_list(sb2.free_block, 2136, data_block_base_addr, sb2.blocksize, buffer, new_buffer);
    // display_used_inodes(&in2, num_inodes2);

    compare_data_block(&sb, &in, buffer, buffer2);
    
    // traverse_free_block_list(sb.free_block, data_block_base_addr, buffer, sb.blocksize);
    // traverse_free_block_list(sb.data_offset, data_block_base_addr, buffer2, sb.blocksize);
    // printf("%d\n",memcmp(buffer, buffer2, 10 * 1024 * 1024));

}