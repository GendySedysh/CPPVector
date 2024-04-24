#pragma once
#include <memory>
#include <cassert>
#include <algorithm>
#include <cstdlib>
#include <new>
#include <utility>

template <typename T>
class RawMemory {
public:
    RawMemory() = default;

    explicit RawMemory(size_t capacity)
        : buffer_(Allocate(capacity))
        , capacity_(capacity) {
    }

    ~RawMemory() {
        Deallocate(buffer_);
    }

    RawMemory(const RawMemory&) = delete;
    RawMemory& operator=(const RawMemory& rhs) = delete;

    RawMemory(RawMemory&& other) noexcept {
        buffer_ = std::exchange(other.buffer_, nullptr);
        capacity_ = std::exchange(other.capacity_, 0);
    }
    RawMemory& operator=(RawMemory&& rhs) noexcept {
        buffer_ = std::exchange(rhs.buffer_, nullptr);
        capacity_ = std::exchange(rhs.capacity_, 0);

        return *this;
    }

    T* operator+(size_t offset) noexcept {
        // Разрешается получать адрес ячейки памяти, следующей за последним элементом массива
        assert(offset <= capacity_);
        return buffer_ + offset;
    }

    const T* operator+(size_t offset) const noexcept {
        return const_cast<RawMemory&>(*this) + offset;
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<RawMemory&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < capacity_);
        return buffer_[index];
    }

    void Swap(RawMemory& other) noexcept {
        std::swap(buffer_, other.buffer_);
        std::swap(capacity_, other.capacity_);
    }

    const T* GetAddress() const noexcept {
        return buffer_;
    }

    T* GetAddress() noexcept {
        return buffer_;
    }

    size_t Capacity() const {
        return capacity_;
    }

private:
    static T* Allocate(size_t n) {
        return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
    }

    static void Deallocate(T* buf) noexcept {
        operator delete(buf);
    }

    T* buffer_ = nullptr;
    size_t capacity_ = 0;
};

/*------------------------------------------*/
/*------------------------------------------*/
/*------------------------------------------*/

template <typename T>
class Vector {
public:
    using iterator = T*;
    using const_iterator = const T*;

    Vector() = default;

    explicit Vector(size_t size)
        : data_(size)
        , size_(size)
    {
        std::uninitialized_value_construct_n(data_.GetAddress(), size);
    }

    Vector(const Vector& other)
        : data_(other.size_)
        , size_(other.size_)
    {
        std::uninitialized_copy_n(other.data_.GetAddress(), size_, data_.GetAddress());
    }

    Vector(Vector&& other) noexcept {
        Swap(other);
    }

    Vector& operator=(const Vector& rhs) {
        if (this != &rhs) {
            if (rhs.size_ > data_.Capacity()) {
                Vector rhs_copy(rhs);
                Swap(rhs_copy);
            }
            else {
                size_t i = 0;
                size_t n_to_copy = std::min(size_, rhs.size_);
                for (; i < n_to_copy; ++i) {
                    data_[i] = rhs.data_[i];
                }
                if (rhs.size_ > size_) {
                    std::uninitialized_copy_n(rhs.data_.GetAddress() + i, rhs.size_ - size_, data_.GetAddress() + i);
                }
                else {
                    std::destroy_n(data_.GetAddress() + i, size_ - rhs.size_);
                }
                size_ = rhs.size_;
            }
        }
        return *this;
    }

    Vector& operator=(Vector&& rhs) noexcept {
        Swap(rhs);
        return *this;
    }

    ~Vector() {
        if (size_ != 0) {
            std::destroy_n(data_.GetAddress(), size_);
        }
    }

    void Resize(size_t new_size) {
        if (new_size < size_) {
            std::destroy_n(data_.GetAddress() + new_size, size_ - new_size);
            size_ = new_size;
            return;
        }
        Reserve(new_size);
        std::uninitialized_value_construct_n(data_.GetAddress() + size_, new_size - size_);
        size_ = new_size;
    }

    void PushBack(const T& value) {
        if (data_.Capacity() > size_) {
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(&value, 1, data_.GetAddress() + size_);
            }
            else {
                std::uninitialized_copy_n(&value, 1, data_.GetAddress() + size_);
            }
            ++size_;
            return;
        }
        size_t new_size;
        if (size_ == 0) {
            new_size = 1;
        }
        else {
            new_size = size_ * 2;
        }
        RawMemory<T> new_data(new_size);

        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(&value, 1, new_data.GetAddress() + size_);
            std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        else {
            std::uninitialized_copy_n(&value, 1, new_data.GetAddress() + size_);
            std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        std::destroy_n(data_.GetAddress(), size_);
        data_.Swap(new_data);
        ++size_;
    }

    void PushBack(T&& value) {
        if (data_.Capacity() > size_) {
            std::uninitialized_move_n(&value, 1, data_.GetAddress() + size_);
            ++size_;
            return;
        }
        size_t new_size;
        if (size_ == 0) {
            new_size = 1;
        }
        else {
            new_size = size_ * 2;
        }
        RawMemory<T> new_data(new_size);

        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(&value, 1, new_data.GetAddress() + size_);
            std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        else {
            std::uninitialized_move_n(&value, 1, new_data.GetAddress() + size_);
            std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        std::destroy_n(data_.GetAddress(), size_);
        data_.Swap(new_data);
        ++size_;
    }

    void PopBack() noexcept {
        if (size_ != 0) {
            --size_;
            Destroy(data_.GetAddress() + size_);
        }
    }

    template <typename... Args>
    T& EmplaceBack(Args&&... args) {
        if (data_.Capacity() > size_) {
            new (data_.GetAddress() + size_) T(std::forward<Args>(args)...);
            ++size_;
            return *(data_.GetAddress() + size_ - 1);
        }
        size_t new_size;
        if (size_ == 0) {
            new_size = 1;
        }
        else {
            new_size = size_ * 2;
        }
        RawMemory<T> new_data(new_size);

        new (new_data.GetAddress() + size_) T(std::forward<Args>(args)...);
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        else {
            std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        std::destroy_n(data_.GetAddress(), size_);
        data_.Swap(new_data);
        ++size_;
        return *(data_.GetAddress() + size_ - 1);
    }

    size_t Size() const noexcept {
        return size_;
    }

    size_t Capacity() const noexcept {
        return data_.Capacity();
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<Vector&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < size_);
        return data_[index];
    }

    void Swap(Vector& other) noexcept {
        std::swap(data_, other.data_);
        std::swap(size_, other.size_);
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity <= data_.Capacity()) {
            return;
        }
        RawMemory<T> new_data(new_capacity);

        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        else {
            std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        std::destroy_n(data_.GetAddress(), size_);
        data_.Swap(new_data);
    }

    template <typename... Args>
    iterator Emplace(const_iterator pos, Args&&... args) {
        size_t i_pos = std::distance(cbegin(), pos);
        if (data_.Capacity() > size_) {
            if (size_ != i_pos) {
                new (end()) T(std::forward<T>(*(end() - 1)));
                std::move_backward(begin() + i_pos, end() - 1, end());
                data_[i_pos] = T(std::forward<Args>(args)...);
            }
            else {
                new (begin() + i_pos) T(std::forward<Args>(args)...);
            }
            ++size_;
            return begin() + i_pos;
        }

        size_t new_size;
        if (size_ == 0) {
            new_size = 1;
        }
        else {
            new_size = size_ * 2;
        }
        RawMemory<T> new_data(new_size);

        size_t count_before = std::distance(cbegin(), pos);
        size_t count_after = std::distance(pos, cend());

        new (new_data.GetAddress() + i_pos) T(std::forward<Args>(args)...);
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(begin(), count_before, new_data.GetAddress());
            std::uninitialized_move_n(begin() + count_before, count_after, new_data.GetAddress() + count_before + 1);
        }
        else {
            std::uninitialized_copy_n(begin(), count_before, new_data.GetAddress());
            std::uninitialized_copy_n(begin() + count_before, count_after, new_data.GetAddress() + count_before + 1);
        }
        std::destroy_n(begin(), size_);
        data_.Swap(new_data);
        ++size_;
        return begin() + i_pos;
    }


    iterator Erase(const_iterator pos) noexcept(std::is_nothrow_move_assignable_v<T>) {
        size_t i_pos = std::distance(cbegin(), pos);

        std::move(begin() + i_pos + 1, end(), begin() + i_pos);
        std::destroy_n(end() - 1, 1);

        --size_;
        return begin() + i_pos;
    }

    iterator Insert(const_iterator pos, const T& value) {
        return Emplace(pos, value);
    }
    iterator Insert(const_iterator pos, T&& value) {
        return Emplace(pos, std::move(value));
    }

    iterator begin() noexcept { return data_.GetAddress(); }
    iterator end() noexcept { return data_.GetAddress() + size_; }
    const_iterator begin() const noexcept { return data_.GetAddress(); }
    const_iterator end() const noexcept { return data_.GetAddress() + size_; }
    const_iterator cbegin() const noexcept { return data_.GetAddress(); }
    const_iterator cend() const noexcept { return data_.GetAddress() + size_; }

private:
    RawMemory<T> data_;
    size_t size_ = 0;

    static void DestroyN(T* buf, size_t n) noexcept {
        for (size_t i = 0; i != n; ++i) {
            Destroy(buf + i);
        }
    }

    static void CopyConstruct(T* buf, const T& elem) {
        new (buf) T(elem);
    }

    static void Destroy(T* buf) noexcept {
        buf->~T();
    }
};
