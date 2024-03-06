#include "common.hpp"

#include <erebus/lrucache.hxx>


TEST(LruCache, simple)
{
    const int NUM_OF_RECORDS = 100;
    const int CACHE_CAPACITY = 50;

    Er::LruCache<int, int> cache(CACHE_CAPACITY);

    for (int i = 0; i < NUM_OF_RECORDS; ++i)
    {
        cache.put(i, i);
    }

    for (int i = 0; i < NUM_OF_RECORDS - CACHE_CAPACITY; ++i)
    {
        EXPECT_FALSE(cache.exists(i));
    }

    for (int i = NUM_OF_RECORDS - CACHE_CAPACITY; i < NUM_OF_RECORDS; ++i)
    {
        EXPECT_TRUE(cache.exists(i));
        auto v = cache.get(i);
        EXPECT_NE(v, nullptr);
        EXPECT_EQ(i, v ? *v : 0);
    }

    size_t size = cache.size();
    EXPECT_EQ(CACHE_CAPACITY, size);
}