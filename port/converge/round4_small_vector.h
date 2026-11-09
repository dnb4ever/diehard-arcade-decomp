#pragma once
#include <vector>
#include <iterator>
#include <stdexcept>
// Adapter-local inline storage avoids heap traffic on small ordered effects.
// Larger transactions retain the same vector behavior and fail-closed limits.
namespace converge {
template<class T,unsigned N> class SmallVector {
    T local[N];
    std::vector<T> heap;
    size_t used=0;
public:
    void push_back(const T& value) {
        if(used<N)local[used]=value;
        else {if(used==N)heap.assign(local,local+N);heap.push_back(value);}
        ++used;
    }
    size_t size() const{return used;}
    bool empty() const{return !used;}
    T* begin(){return used>N ? heap.data() : local;}
    const T* begin() const{return used>N ? heap.data() : local;}
    T* end(){return begin()+used;}
    const T* end() const{return begin()+used;}
    auto rbegin() const{return std::reverse_iterator<const T*>(end());}
    auto rend() const{return std::reverse_iterator<const T*>(begin());}
    const T& operator[](size_t i) const{return begin()[i];}
    T& operator[](size_t i){return begin()[i];}
    const T& at(size_t i) const{if(i>=used)throw std::out_of_range("effect bytes");return begin()[i];}
};
}
