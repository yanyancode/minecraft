#ifndef WORLD_H_
#define WORLD_H_

#define CHUNK_SIZE 16

typedef struct {
    int id;
} Block;

typedef struct {
    Block blocks[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];
    int x;
    int z;
    int is_loaded; // Using int as boolean (0 for false, 1 for true)
    int block_count;
} Chunk;

typedef struct {
    // For now, let's use a fixed-size array of chunks.
    // A more dynamic approach (list or hash map) can be implemented later.
    Chunk chunks[100]; // Assuming a maximum of 100 chunks for now
    int num_chunks; // To keep track of the actual number of chunks
    char* name;
} World;

#endif /* WORLD_H_ */
