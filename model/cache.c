#include "cache.h"
#include "dogfault.h"
#include <assert.h>
#include <ctype.h>
#include <getopt.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// DO NOT MODIFY THIS FILE. INVOKE AFTER EACH ACCESS FROM runTrace
void print_result(result r) {
  if (r.status == CACHE_EVICT)
    printf(" [status: %d victim_block: 0x%llx insert_block: 0x%llx]", r.status,
           r.victim_block, r.insert_block);
  if (r.status == CACHE_HIT)
    printf(" [status: %d]", r.status);
  if (r.status == CACHE_MISS)
    printf(" [status: %d insert_block: 0x%llx]", r.status, r.insert_block);
}

// HELPER FUNCTIONS USEFUL FOR IMPLEMENTING THE CACHE
// Convert address to block address. 0s out the bottom block bits.
unsigned long long address_to_block(const unsigned long long address, const Cache *cache) {
    unsigned long long addressblock = address >> cache->blockBits; 
    addressblock <<= cache->blockBits;
    return addressblock;
}

// Access the cache after successful probing.
void access_cache(const unsigned long long address, const Cache *cache) {
    unsigned long long set_index = cache_set(address, cache);
    unsigned long long tag = cache_tag(address, cache);
    Set *set = &cache->sets[set_index];

    for (int i = 0; i < cache->linesPerSet; i++) {
        if (set->lines[i].valid && set->lines[i].tag == tag) {
            set->lines[i].r_rate = ++set->recentRate;
            return;
        }
    }
}

// Calculate the tag of the address. 0s out the bottom set bits and the bottom block bits.
unsigned long long cache_tag(const unsigned long long address, const Cache *cache) {
    unsigned long long addressblock = address >> (cache->setBits + cache->blockBits);
    addressblock <<= (cache->setBits + cache->blockBits);
    return addressblock;
}

// Calculate the set of the address. 0s out the bottom block bits, 0s out the tag bits, and then shift the set bits to the right.
unsigned long long cache_set(const unsigned long long address, const Cache *cache) {
    unsigned long long addressblock = (address >> cache->blockBits) & ((1ULL << cache->setBits) - 1);
    return addressblock;
}

// Check if the address is found in the cache. If so, return true. else return false.
bool probe_cache(const unsigned long long address, const Cache *cache) {
    unsigned long long set_index = cache_set(address, cache);
    unsigned long long tag = cache_tag(address, cache);
    Set *set = &cache->sets[set_index];

    for (int i = 0; i < cache->linesPerSet; i++) {
        if (set->lines[i].valid && set->lines[i].tag == tag) {
            set->lines[i].r_rate = ++set->recentRate;
            return true;
        }
    }
    return false;
}

// Allocate an entry for the address. If the cache is full, evict an entry to create space. This method will not fail. When method runs there should have already been space created.
void allocate_cache(const unsigned long long address, const Cache *cache) {
    unsigned long long set_index = cache_set(address, cache);
    unsigned long long tag = cache_tag(address, cache);
    Set *set = &cache->sets[set_index];

    for (int i = 0; i < cache->linesPerSet; i++) {
        if (!set->lines[i].valid) {
            set->lines[i].valid = 1;
            set->lines[i].tag = tag;
            set->lines[i].block_addr = address_to_block(address, cache);
            set->lines[i].r_rate = ++set->recentRate;
            return;
        }
    }

}

// Is there space available in the set corresponding to the address?
bool avail_cache(const unsigned long long address, const Cache *cache) {
    unsigned long long set_index = cache_set(address, cache);
    Set *set = &cache->sets[set_index];

    for (int i = 0; i < cache->linesPerSet; i++) {
        if (!set->lines[i].valid) {
            return true;
        }
    }
    return false;
}

// If the cache is full, evict an entry to create space. This method figures out which entry to evict. Depends on the policy.
unsigned long long victim_cache(const unsigned long long address, Cache *cache) {
    unsigned long long set_index = cache_set(address, cache);
    Set *set = &cache->sets[set_index];
    int victim = 0;
    int min_rate = set->lines[0].r_rate;

    for (int i = 1; i < cache->linesPerSet; i++) {
        if (set->lines[i].r_rate < min_rate) {
            min_rate = set->lines[i].r_rate;
            victim = i;
        }
    }
    return victim;
}

// Set can be determined by the address. Way is determined by policy and set by the operate cache. 
void evict_cache(const unsigned long long address, int way, Cache *cache) {
    unsigned long long set_index = cache_set(address, cache);
    Set *set = &cache->sets[set_index];
    set->lines[way].valid = 0;
}

// Given a block address, find it in the cache and when found remove it.
// If not found don't remove it. Useful when implementing 2-level policies. 
// and triggering evictions from other caches. 
void flush_cache(const unsigned long long block_address, Cache *cache) {
    unsigned long long set_index = cache_set(block_address, cache);
    unsigned long long tag = cache_tag(block_address, cache);
    Set *set = &cache->sets[set_index];

    for (int i = 0; i < cache->linesPerSet; i++) {
        if (set->lines[i].valid && set->lines[i].tag == tag) {
            set->lines[i].valid = 0;
            return;
        }
    }
}

// checks if the address is in the cache, if not and if the cache is full
// evicts an address
result operateCache(const unsigned long long address, Cache *cache) {
  // checkCache checks if the address is in the cache
  result r;

  if (probe_cache(address, cache) == true){  
    access_cache(address, cache);            // hit
    r.status = 1;
    printf(" %s hit ", cache->name);
  } else {
    if (avail_cache(address, cache) == false){
      unsigned long long victim = victim_cache(address, cache); // miss + eviction
      unsigned long long set = cache_set(address, cache);
      unsigned long long victimAdd = cache->sets[set].lines[victim].block_addr;
      evict_cache(address, victim, cache);
      r.status = 2;
      r.insert_block = address_to_block(address, cache);
      r.victim_block = address_to_block(victimAdd, cache);
    } else {
      r.status = 0;         // miss
      r.insert_block = address_to_block(address, cache);
    }
    allocate_cache(address, cache);
  }        
  // Hit
  // Miss
  // Evict
  return r;
}

// initialize the cache and allocate space for it
void cacheSetUp(Cache *cache, char *name) {
    cache->hit_count = 0;
    cache->miss_count = 0;
    cache->eviction_count = 0;
    
    cache->name = (char *)malloc(strlen(name) + 1);
    strcpy(cache->name, name);

    int num_sets = 1 << cache->setBits;
    cache->sets = (Set *)malloc(num_sets * sizeof(Set));

    for (int i = 0; i < num_sets; i++) {
        cache->sets[i].lines = (Line *)malloc(cache->linesPerSet * sizeof(Line));
        cache->sets[i].recentRate = 0;
        cache->sets[i].placementRate = 0;

        for (int j = 0; j < cache->linesPerSet; j++) {
            cache->sets[i].lines[j].valid = 0;
            cache->sets[i].lines[j].r_rate = 0;
            // cache->sets[i].lines[j].block_addr = 0;
            // cache->sets[i].lines[j].tag = 0;
        }
    }
} 

// deallocate memory
void deallocate(Cache *cache) {
    int num_sets = 1 << cache->setBits;

    for (int i = 0; i < num_sets; i++) {
        free(cache->sets[i].lines);
    }
    free(cache->sets);
    // Deallocate memory for the name
    free(cache->name);
}

void printSummary(const Cache *cache) {
  printf("\n%s hits:%d misses:%d evictions:%d", cache->name, cache->hit_count,
         cache->miss_count, cache->eviction_count);
}
