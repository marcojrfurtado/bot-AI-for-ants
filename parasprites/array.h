#pragma once
#include "ints.h"
#include "macros.h"
#include <vector>

template <typename T>
struct Array2d
{
    typedef T val_t;
    typedef int size_t;

    std::vector<T> data;
    size_t width;

    Array2d(): width(0) {}
    Array2d(size_t h, size_t w, const T& value= T()): data(h*w, value), width(w) {ASSERT(w > 0 && h > 0);}

    T& operator()(size_t y, size_t x) {
//        return data[ normalize(y,sizeh()) * sizew() + normalize(x,sizew()) ];
        return data.at(normalize(y,sizeh()) * sizew() + normalize(x,sizew()));
    }

    const T& operator()(size_t y, size_t x) const {
//        return data[ normalize(y,sizeh()) * sizew() + normalize(x,sizew()) ];
        return data.at(normalize(y,sizeh()) * sizew() + normalize(x,sizew()));
    }

    T& operator[](Pos l) {return (*this)(l.row, l.col);}
    const T& operator[](Pos l) const {return (*this)(l.row, l.col);}

    size_t size() const {return data.size();}
    size_t sizew() const {return width;}
    size_t sizeh() const {return width ? (data.size() / width) : 0;} //Return 0 for empty array

    //Wrap around indices that are at most max away from the bounds
    static size_t normalize(size_t val, size_t max){
        if (val > max) {val -= max;}
        if (val < 0) {val += max;}
        return val;
    }

    void swap(Array2d<T>& other){
        auto temp = width;
        width = other.width;
        other.width = temp;
        data.swap(other.data);
    }
};
