#pragma once
#include <algorithm>

template <typename tup_t, bool reverse>
struct HeapComparator{
    bool operator() (const tup_t& a, const tup_t& b) const
    {
        if (reverse) {return std::get<1>(a) > std::get<1>(b);}
        return std::get<1>(a) < std::get<1>(b);
    }
};

template <typename T, typename K, bool minHeap = true>
struct PriorityQueue{
//    using namespace ::std;
    typedef std::tuple<T, K> val_t;
    typedef HeapComparator<val_t, minHeap> compare;

    std::vector<val_t> data;

    size_t size() const {return data.size();}
    operator bool() const {return data.size();}

    K minKey() const {return std::get<1>(data.front());}

    void push(T val, K key){
        data.push_back(std::make_tuple(val, key));
        std::push_heap(data.begin(), data.end(), compare());
    }

    val_t pop_val(){
        std::pop_heap(data.begin(), data.end(), compare());

        val_t temp = data.back();
        data.pop_back();
        return temp;
    }

    T pop() {return std::get<0>(pop_val());}

    void heapify(){
        std::make_heap(data.begin(), data.end(), compare());
    }

//    static MinHeapComparator<val_t> compare() {return MinHeapComparator<val_t>();}
};
