#include <vector>
#include <memory>
#include <cstddef>
#include <iterator>
#include <stdexcept>
#include <type_traits>
#include <bitset>

// Slop coded completly by Gemini Pro

template <typename T, size_t S = 256>
class ChunkedColony {
private:
    // Use uninitialized memory to avoid requiring a default constructor for T
    struct Chunk {
        alignas(T) std::byte data[S * sizeof(T)];
        std::bitset<S> active; 
        
        T* get() { return reinterpret_cast<T*>(data); }
        const T* get() const { return reinterpret_cast<const T*>(data); }
    };

    std::vector<std::unique_ptr<Chunk>> chunks;
    std::vector<size_t> free_list; 
    size_t total_capacity = 0;
    size_t active_count = 0;

    bool is_active(size_t index) const {
        if (index >= total_capacity) return false;
        return chunks[index / S]->active.test(index % S);
    }

public:
    template <bool IsConst>
    class Iterator {
    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type        = T;
        using difference_type   = std::ptrdiff_t;
        using pointer           = std::conditional_t<IsConst, const T*, T*>;
        using reference         = std::conditional_t<IsConst, const T&, T&>;
        using ColonyPtr         = std::conditional_t<IsConst, const ChunkedColony*, ChunkedColony*>;

    private:
        ColonyPtr colony;
        size_t index;

        void advance() {
            while (index < colony->total_capacity) {
                ++index;
                if (index == colony->total_capacity || colony->is_active(index)) break;
            }
        }

        void retreat() {
            while (index > 0) {
                --index;
                if (colony->is_active(index)) break;
            }
        }

    public:

        Iterator() : colony(nullptr), index(0) {}

        Iterator(ColonyPtr col, size_t idx) : colony(col), index(idx) {
            if (index < colony->total_capacity && !colony->is_active(index)) {
                advance();
            }
        }

        reference operator*() const { return (*colony)[index]; }
        pointer operator->() const { return &(*colony)[index]; }

        Iterator& operator++() { advance(); return *this; }
        Iterator operator++(int) { Iterator tmp = *this; advance(); return tmp; }
        Iterator& operator--() { retreat(); return *this; }
        Iterator operator--(int) { Iterator tmp = *this; retreat(); return tmp; }

        bool operator==(const Iterator& other) const { return index == other.index; }
        bool operator!=(const Iterator& other) const { return index != other.index; }
    
        size_t get_index() const { return index; }
    };

    using iterator       = Iterator<false>;
    using const_iterator = Iterator<true>;

    ChunkedColony() = default;

    ~ChunkedColony() {
        clear();
    }

    // O(1) Retrieval by index
    T& operator[](size_t index) {
        return chunks[index / S]->get()[index % S];
    }

    const T& operator[](size_t index) const {
        return chunks[index / S]->get()[index % S];
    }

    // O(1) amortized insertion (pushing)
    void push_back(const T& value) {
        emplace_back(value);
    }

    template <typename... Args>
    void emplace_back(Args&&... args) {
        size_t target_idx;
        if (!free_list.empty()) {
            target_idx = free_list.back();
            free_list.pop_back();
        } else {
            target_idx = total_capacity++;
            if (target_idx % S == 0) {
                chunks.push_back(std::make_unique<Chunk>());
            }
        }
        new (&(*this)[target_idx]) T(std::forward<Args>(args)...);
        chunks[target_idx / S]->active.set(target_idx % S);
        ++active_count;
    }

    void clear() noexcept(std::is_nothrow_destructible_v<T>) {
        for (size_t i = 0; i < total_capacity; ++i) {
            if (is_active(i)) {
                (*this)[i].~T();
            }
        }
        chunks.clear();
        free_list.clear();
        total_capacity = 0;
        active_count = 0;
    }

    // Erases the element at position 'pos'
    iterator erase(iterator pos) {
        size_t idx = pos.get_index();
        if (idx >= total_capacity || !is_active(idx)) {
            return end(); // or throw, but end is safe
        }

        // Shift all elements left by one (from idx+1 to end)
        // [Note: Replaced with tombstoning to maintain iterator validity]
        
        // Destroy the last element (now a duplicate)
        (*this)[idx].~T();
        chunks[idx / S]->active.reset(idx % S);
        free_list.push_back(idx);
        --active_count;

        // Return iterator to the new position (or end if we erased the last)
        iterator next_it = pos;
        ++next_it;
        return next_it;
    }

    // Erases all elements in the range [first, last)
    iterator erase(iterator first, iterator last) {
        if (first.get_index() >= total_capacity) {
            return end(); // or handle gracefully
        }

        // Shift elements from last_idx onwards left by range_size
        // Destroy the now‑redundant tail elements (after the new end)
        // [Note: Iterative tombstoning utilized instead]
        while (first != last && first != end()) {
            first = erase(first);
        }

        // Return iterator to the element after the erased block
        return first;
    }

    size_t size() const { return active_count; }
    bool empty() const { return active_count == 0; }

    iterator begin() { return iterator(this, 0); }
    iterator end() { return iterator(this, total_capacity); }
    
    const_iterator begin() const { return const_iterator(this, 0); }
    const_iterator end() const { return const_iterator(this, total_capacity); }
    const_iterator cbegin() const { return const_iterator(this, 0); }
    const_iterator cend() const { return const_iterator(this, total_capacity); }
};