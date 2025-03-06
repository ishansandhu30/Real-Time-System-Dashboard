#ifndef DATA_BUFFER_H
#define DATA_BUFFER_H

#include <vector>
#include <mutex>
#include <cstddef>
#include <stdexcept>
#include <algorithm>

/// Thread-safe circular buffer with fixed capacity and automatic eviction
/// of oldest data when full.
template <typename T>
class DataBuffer {
public:
    explicit DataBuffer(size_t capacity = 300)
        : m_capacity(capacity)
        , m_head(0)
        , m_count(0)
    {
        if (capacity == 0) {
            throw std::invalid_argument("DataBuffer capacity must be > 0");
        }
        m_buffer.resize(capacity);
    }

    // Non-copyable due to std::mutex
    DataBuffer(const DataBuffer&) = delete;
    DataBuffer& operator=(const DataBuffer&) = delete;

    // Move constructor: transfers data, creates fresh mutex
    DataBuffer(DataBuffer&& other) noexcept
        : m_capacity(0), m_head(0), m_count(0)
    {
        std::lock_guard<std::mutex> lock(other.m_mutex);
        m_buffer = std::move(other.m_buffer);
        m_capacity = other.m_capacity;
        m_head = other.m_head;
        m_count = other.m_count;
        other.m_capacity = 0;
        other.m_head = 0;
        other.m_count = 0;
    }

    // Move assignment: transfers data, creates fresh mutex
    DataBuffer& operator=(DataBuffer&& other) noexcept {
        if (this != &other) {
            std::lock_guard<std::mutex> lock1(m_mutex);
            std::lock_guard<std::mutex> lock2(other.m_mutex);
            m_buffer = std::move(other.m_buffer);
            m_capacity = other.m_capacity;
            m_head = other.m_head;
            m_count = other.m_count;
            other.m_capacity = 0;
            other.m_head = 0;
            other.m_count = 0;
        }
        return *this;
    }

    /// Push a new element, evicting the oldest if at capacity.
    void push(const T& value) {
        std::lock_guard<std::mutex> lock(m_mutex);
        size_t write_pos = (m_head + m_count) % m_capacity;
        m_buffer[write_pos] = value;
        if (m_count < m_capacity) {
            ++m_count;
        } else {
            // Overwrite oldest: advance head
            m_head = (m_head + 1) % m_capacity;
        }
    }

    /// Push via move semantics.
    void push(T&& value) {
        std::lock_guard<std::mutex> lock(m_mutex);
        size_t write_pos = (m_head + m_count) % m_capacity;
        m_buffer[write_pos] = std::move(value);
        if (m_count < m_capacity) {
            ++m_count;
        } else {
            m_head = (m_head + 1) % m_capacity;
        }
    }

    /// Get element at logical index (0 = oldest).
    T at(size_t index) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (index >= m_count) {
            throw std::out_of_range("DataBuffer index out of range");
        }
        return m_buffer[(m_head + index) % m_capacity];
    }

    /// Get the most recently pushed element.
    T latest() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_count == 0) {
            throw std::out_of_range("DataBuffer is empty");
        }
        return m_buffer[(m_head + m_count - 1) % m_capacity];
    }

    /// Return a snapshot of all elements in chronological order (oldest first).
    std::vector<T> snapshot() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<T> result;
        result.reserve(m_count);
        for (size_t i = 0; i < m_count; ++i) {
            result.push_back(m_buffer[(m_head + i) % m_capacity]);
        }
        return result;
    }

    /// Return the last n elements (most recent), in chronological order.
    std::vector<T> last_n(size_t n) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        size_t actual = std::min(n, m_count);
        std::vector<T> result;
        result.reserve(actual);
        size_t start = m_count - actual;
        for (size_t i = start; i < m_count; ++i) {
            result.push_back(m_buffer[(m_head + i) % m_capacity]);
        }
        return result;
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_count;
    }

    size_t capacity() const {
        return m_capacity;
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_count == 0;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_head = 0;
        m_count = 0;
    }

    /// Resize the buffer capacity. Data may be lost if new capacity < current count.
    void resize(size_t new_capacity) {
        if (new_capacity == 0) {
            throw std::invalid_argument("DataBuffer capacity must be > 0");
        }
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<T> old_data;
        old_data.reserve(m_count);
        for (size_t i = 0; i < m_count; ++i) {
            old_data.push_back(m_buffer[(m_head + i) % m_capacity]);
        }

        m_capacity = new_capacity;
        m_buffer.resize(new_capacity);
        m_head = 0;

        // Keep the most recent elements that fit
        if (old_data.size() <= new_capacity) {
            m_count = old_data.size();
            for (size_t i = 0; i < m_count; ++i) {
                m_buffer[i] = std::move(old_data[i]);
            }
        } else {
            m_count = new_capacity;
            size_t offset = old_data.size() - new_capacity;
            for (size_t i = 0; i < new_capacity; ++i) {
                m_buffer[i] = std::move(old_data[offset + i]);
            }
        }
    }

private:
    std::vector<T> m_buffer;
    size_t m_capacity;
    size_t m_head;
    size_t m_count;
    mutable std::mutex m_mutex;
};

#endif // DATA_BUFFER_H
