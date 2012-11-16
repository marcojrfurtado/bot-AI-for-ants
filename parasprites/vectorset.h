#pragma once

template<typename T>
struct VectorSet : public std::vector<T>
{
    std::size_t count( const T& x ) const
        {return std::binary_search(this->begin(), this->end(), x);}
//        { ASSERT(sorted()); return std::binary_search(this->begin(), this->end(), x);}

    void sort() {
        std::sort(this->begin(), this->end());
        erase(std::unique(this->begin(), this->end()), this->end()); //delete duplicates
        ASSERT(sorted());
    }

    bool sorted() const {
        std::set<T> a(this->begin(), this->end());
        std::vector<T> b(a.begin(), a.end());
        return *this == b;
    }
};
