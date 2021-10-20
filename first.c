#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/*global variables*/
int set_num = 0; 
int num_blocks = 0; 
int setBit = 0;
int offsetBits = 0; 

int memory_read = 0; 
int memory_write = 0;
int cache_hit = 0; 
int cache_miss = 0;  

/*method declarations*/
int isLog(int x);
int calculateSets(int cache_size, int block_size, char* associativity, int n); 
size_t** makeCache(int sets, int assoc);
void freeMatrix(size_t** cache, int sets, int assoc);
int search(size_t** cache, size_t address, int offsetBits, int setBit, int num_blocks);
void update(FILE* trace_file, size_t** cache, int offsetBits, int setBit, int num_blocks, char* replace_policy); 
size_t** policyLRU(size_t** cache, size_t address, int offsetBits, int setBit, int num_blocks); 
size_t** add(size_t** cache, size_t address, int offsetBits, int setBit, int num_blocks); 
void printOutput(); 

int main(int argc, char* argv[])
{
	
	if(argc != 6)
	{
		printf("Error: not enough arguments!\n");
	        return 1; 	
	}

	/*first argument: <cache size>*/
	int cache_size = atoi(argv[1]); 
	if(isLog(cache_size) == 0)
	{
		printf("Error: <cache size> is not power of 2\n");
	        return 1; 	
	}

	/*second argument: <associativity> -direct ... -assoc ... -assoc:n*/
	char* associativity = argv[2];  	
	if((associativity[0] == 'd' || associativity[0] == 'a') == 0)
	{
		printf("Error: <associativity> is not one of the required\n");
	        return 1; 	
	}
	int n_way; 
	sscanf(argv[2], "assoc:%d", &n_way);
	
	/*third argument: <replace policy> -lru ... -fifo*/
	char* replace_policy = argv[3];
	if(strcmp(replace_policy, "lru") != 0 && strcmp(replace_policy, "fifo") != 0)
	{
		printf("Error: <replace policy> is not lru or fifo\n");
		return 1; 
	}
	if(strcmp(replace_policy, "lru") == 0)
	{
		replace_policy = "l";
	}
	else if(strcmp(replace_policy, "fifo") == 0)
	{
		replace_policy = "f"; 
	}

	/*fourth argument: <block size>*/
	int block_size = atoi(argv[4]);
	if(isLog(block_size) == 0)
	{
		printf("Error: <block size> is not power of 2\n"); 
		return 1; 
	}

	/*fifth argument: <trace file>*/
	FILE* f = fopen(argv[5], "r");
	if(f == NULL)
	{
		printf("Error: <trace file> cannot be opened\n"); 
		return 1; 
	}
	set_num = calculateSets(cache_size, block_size, associativity, n_way);
        num_blocks = cache_size/(block_size * set_num);	 
	setBit = log(set_num)/log(2);
	offsetBits = log(block_size)/log(2); 
	size_t** cache = makeCache(set_num, num_blocks);
	update(f, cache, offsetBits, setBit, num_blocks, replace_policy);
	printOutput();
	freeMatrix(cache, set_num, num_blocks);
	fclose(f); 
	return 0; 	
}

int isLog(int x)
{
	while((x % 2 == 0) && (x > 2))
	{
		x /= 2;
	}
	return (x == 2); 
}

size_t** makeCache(int sets, int assoc)
{
	size_t** cache = (size_t**) malloc(sizeof(size_t*) * sets);
        int i, j; 	
	for(i = 0; i < sets; i++)
	{
		cache[i] = (size_t*) malloc(sizeof(size_t) * assoc); 
		for(j = 0; j < assoc; j++)
		{
			cache[i][j] = (size_t) NULL; 
		}
	}
	return cache; 
}

int calculateSets(int cache_size, int block_size, char* associativity, int n)
{
	if(strcmp(associativity, "direct") == 0)
	{
		return ((cache_size) / (block_size)); 
	}
	else if(strcmp(associativity, "assoc") == 0)
	{
		return 1; 
	}
	else 
	{
		if(isLog(n) == 0)
		{
			printf("Error: <assoc:n> n not a power of 2\n");
		}
		return (cache_size / (block_size * n)); 	
	}
}

void update(FILE* trace_file, size_t** cache, int offsetBits, int setBit, int num_blocks, char* replace_policy)
{ 
	char letter;
	size_t address;
	while((fscanf(trace_file, "%c %zx\n", &letter, &address) != EOF) && (letter != '#'))
	{	
		if(letter == 'W')
		{
			memory_write++;
			if(search(cache, address, offsetBits, setBit, num_blocks) == 1)
			{
				cache_hit++;
				if(strcmp(replace_policy, "l") == 0)
				{
					cache = policyLRU(cache, address, offsetBits, setBit, num_blocks);
				}
				
			}
			else
			{
				cache = add(cache, address, offsetBits, setBit, num_blocks);
				cache_miss++;
				memory_read++;	
			}	
		}
		else if(letter == 'R')
		{
			if(search(cache, address, offsetBits, setBit, num_blocks) == 1)
			{
				cache_hit++;
				if(strcmp(replace_policy, "l") == 0)
				{
					cache = policyLRU(cache, address, offsetBits, setBit, num_blocks);
				}
				
			}
			else
			{
				cache = add(cache, address, offsetBits, setBit, num_blocks);
				cache_miss++;
				memory_read++;	
			}
		}
	}	
}

int search(size_t** cache, size_t address, int offsetBits, int setBit, int num_blocks)
{
	size_t setIndex = (address>>offsetBits)&((1<<setBit)-1);
	size_t tag = (address>>offsetBits)>>setBit; 
        int b;
	for(b = 0; b < num_blocks; b++)
	{
		if(tag == cache[setIndex][b])
		{
			return 1; 
		}
	}
	return 0; 	
}

size_t** policyLRU(size_t** cache, size_t address, int offsetBits, int setBit, int num_blocks)
{
	size_t setIndex = (address>>offsetBits)&((1<<setBit)-1);
	size_t tag = (address>>offsetBits)>>setBit; 
	int flag = 0;
	int b; 
	for(b = 0; b < num_blocks - 1; b++)
	{
		if(cache[setIndex][b + 1] == (size_t) NULL)
		{
			break;
		}
		if(cache[setIndex][b] == tag || flag)
		{
			flag = 1; 
			cache[setIndex][b] = cache[setIndex][b + 1];
		}
	}
	cache[setIndex][b] = tag;
	return cache; 
}

size_t** add(size_t** cache, size_t address, int offsetBits, int setBit, int num_blocks)
{
	size_t setIndex = (address>>offsetBits)&((1<<setBit)-1);
	size_t tag = (address>>offsetBits)>>setBit; 
	int added = 0;
	int b; 
	for(b = 0; b < num_blocks; b++)
	{
		if(cache[setIndex][b] == (size_t) NULL)
		{
			cache[setIndex][b] = tag; 
			added = 1;
			break;
		}
	}
	if(added == 0)
	{
		for(b = 1; b < num_blocks; b++)
		{
			cache[setIndex][b - 1] = cache[setIndex][b];
		}
		cache[setIndex][num_blocks - 1] = tag; 
	}
	return cache; 
}

void printOutput()
{
	printf("Memory reads: %d\n", memory_read);
	printf("Memory writes: %d\n", memory_write);
	printf("Cache hits: %d\n", cache_hit);
	printf("Cache misses: %d\n", cache_miss);
}

void freeMatrix(size_t** cache, int sets, int assoc)
{
	int i, j;
	for(i = 0; i < sets; i++)
	{
		for(j = 0; j < assoc; j++)
		{
			cache[i][j] = (size_t) NULL;
		}
		free(cache[i]);
		cache[i] = NULL;
	}
	free(cache);
	cache = NULL;	
}
