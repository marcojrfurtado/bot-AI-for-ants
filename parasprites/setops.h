#include <algorithm>
template <typename T>
inline void operator &= (std::set<T>& a, const std::set<T>& b){
    std::set<T> c;
    std::set_intersection(a.begin(), a.end(), b.begin(), b.end(), std::inserter(c, c.end()));
    a.swap(c);
}

template <typename T>
inline void operator |= (std::set<T>& a, const std::set<T>& b){
    a.insert(b.begin(), b.end());
}

template <typename T>
inline std::set<T> operator - (const std::set<T>& a, const std::set<T>& b){
    std::set<T> c;
    std::set_difference(a.begin(), a.end(), b.begin(), b.end(), std::inserter(c, c.end()));
    return c;
}

//Generic container ops
template <typename T> inline void sort(T& b){ std::sort(b.begin(), b.end());}
template <typename T, class Compare> inline void sort(T& b, Compare comp){ std::sort(b.begin(), b.end(), comp);}

//Make sure that these functions are not used with unordered sets!

template <typename T, typename U>
inline void unionEq(T& a, const U& b){
    T c;
    std::set_union(a.begin(), a.end(), b.begin(), b.end(), std::inserter(c, c.end()));
    a.swap(c);
}

template <typename T, typename U>
inline void differenceEq(T& a, const U& b){
    T c;
    std::set_difference(a.begin(), a.end(), b.begin(), b.end(), std::inserter(c, c.end()));
    a.swap(c);
}

template <typename T, typename U, typename V>
inline T setDifference(const U& a, const V& b){
    T c;
    std::set_difference(a.begin(), a.end(), b.begin(), b.end(), std::inserter(c, c.end()));
    return c;
}

template <typename T, typename U, typename V>
inline T setIntersection(const U& a, const V& b){
    T c;
    std::set_intersection(a.begin(), a.end(), b.begin(), b.end(), std::inserter(c, c.end()));
    return c;
}
