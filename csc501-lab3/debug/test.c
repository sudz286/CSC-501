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


void traverse_free_block_list(int free_block_start, int data_block_base_addr, unsigned char * buffer, int blocksize)
{
    // Take free block no and data_block_base_addr as argument and perform traversal until -1
    // free_block_start = 1;
    int next_free_block;
    int count = 0;
    while(free_block_start != -1)
    {
        int data_block_addr = data_block_base_addr + (free_block_start * blocksize);
        printf("%d \n", free_block_start);
        next_free_block = *((int *) &(buffer[data_block_addr]));
        
        // printf("%d \n", next_free_block);

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
    printf("%d", new_free_block_end);
    memcpy(new_buffer + data_block_base_addr + (new_free_block_end * blocksize), (unsigned char *)&x, sizeof(unsigned char *));

    if(debug_mode)
    {
        // traverse_free_block_list(2136, data_block_base_addr, new_buffer, blocksize);
    }
   
}

void read_data_block(struct superblock * sb, struct inode in[], unsigned char * buffer)
{
    unsigned char * new_buffer = malloc(10 * 1024 * 1024);
    // memcpy(new_buffer, buffer, 10 * 1024 * 1024);
    // Copy boot block to new_buffer
    // memcpy(new_buffer, buffer, 512); Did this in end

    int data_block_base_addr = (1024 + (sb->blocksize * sb->data_offset));
    int inode_base = (1024 + (sb->blocksize * sb->inode_offset));

    /* NUMBER OF INODES */
    int num_inodes = (data_block_base_addr - inode_base) / 100;

    int i = 0;

    for(i = 0; i < num_inodes; i++)
    {
        // int num_bytes_to_be_read = in[i].size;

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
        
        int j = 0;
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
                while(itr < max_pointers && indirect_block_iterator != -1 )
                {
                    int src_ib_position = data_block_base_addr + (indirect_block_iterator * sb->blocksize);
                    int dest_ib_position = data_block_base_addr + (current_db * sb->blocksize);

                    // memcpy(new_buffer + dest_ib_position, buffer + src_ib_position, sb->blocksize);
                    // memcpy(new_buffer + dest_position, (unsigned char *)&current_db, sizeof(unsigned char*));
                    current_db++;
                    indirect_block_iterator = *((int *) & (buffer[src_position + 4]));
                    src_position += 4;
                    dest_position += 4;
                    itr++;
                }
                // in[i].iblocks[j] = indirect_block_ptr;

            }
        }

    }

    printf("%d", current_db);
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
    
    // Modify Free List and write to buffer
    define_free_block_list(sb->free_block, current_db, data_block_base_addr, sb->blocksize, buffer, new_buffer);

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

        // traverse_free_block_list(2136, data_block_base_addr, new_buffer, 512);
    }

}

// Function to read an indirect block and display the block
void test_data_block(struct superblock * sb, struct inode in[], unsigned char * buffer)
{
    unsigned char * new_buffer = malloc(10 * 1024 * 1024);
    // memcpy(new_buffer, buffer, 10 * 1024 * 1024);
    // Copy boot block to new_buffer
    // memcpy(new_buffer, buffer, 512); Did this in end
    FILE *f = fopen("validation_output.txt", "w");
    int varrray[1982];
    int data_block_base_addr = (1024 + (sb->blocksize * sb->data_offset));
    int inode_base = (1024 + (sb->blocksize * sb->inode_offset));
    int count = 0;
    /* NUMBER OF INODES */
    int num_inodes = (data_block_base_addr - inode_base) / 100;

    int i = 0;

    for(i = 0; i < num_inodes; i++)
    {
        // int num_bytes_to_be_read = in[i].size;

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

                    // printf("Direct Block [%d]: %d \n", k, *((int *) &(buffer[dest_position])));
                    // *((int *) &(buffer[512]));
                    // memcpy(new_buffer + dest_position, buffer + src_position, sb->blocksize);

                    // Modify Inode structure
                    in[i].dblocks[k] = current_db++;

                }   
            }
        }
        
        int j = 0;

        for(j = 0; j < N_IBLOCKS; j++)
        {

            if(in[i].iblocks[j] != -1)
            {
                // Copy indirect block from old to new buffer
                int src_position = data_block_base_addr + (in[i].iblocks[j] * sb->blocksize);
                int dest_position = data_block_base_addr + (current_db * sb->blocksize);

                // memcpy(new_buffer + dest_position, buffer + src_position, sb->blocksize);
                // printf("Indirect Block[%d]: %d \n", current_db, buffer[dest_position]);
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
                    if (count < 500){printf("%d \n", indirect_block_iterator); count++;}
                    // varrray[count] = *((int *)&(buffer[dest_position]));
                    // count++;

                    // if (count < 500) {printf("Indirect Block  %d \n", buffer[src_ib_position]); count++;}
                    // memcpy(new_buffer + dest_ib_position, buffer + src_ib_position, sb->blocksize);
                    // memcpy(new_buffer + dest_position, (unsigned char *)&current_db, sizeof(unsigned char*));
                    current_db++;
                    indirect_block_iterator = *((int *) & (buffer[src_position + 4]));
                    src_position += 4;
                    dest_position += 4;
                    itr++;
                }
                // in[i].iblocks[j] = indirect_block_ptr;

            }
        }
        
    }
    // printf("%d", count);
    fwrite(varrray, 1982, 1, f);

}

void main()
{

    FILE *f;
    unsigned char * buffer;
    struct stat st;
    size_t bytes;

    buffer = malloc(10*1024*1024);

    f = fopen("disk_defrag_1","r");
    bytes =  fread(buffer,10*1024*1024,1,f);
    struct superblock sb;
    read_superblock(&sb, buffer);
    display_superblock(&sb);
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
    // display_used_inodes(&in, num_inodes);
    test_data_block(&sb,&in, buffer);

}