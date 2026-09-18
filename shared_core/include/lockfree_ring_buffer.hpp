#pragma once

#include <atomic>
#include <cstddef>
#include <vector>
#include <new>
#include <type_traits>

namespace polyrhythm {

/**
 * @brief Lock-free Single-Producer Single-Consumer (SPSC) Ring Buffer.
 * Real-Time safe: Wait-free, zero allocation, zero lock.
 */
template <typename T, size_t Capacity = 1024>
class LockFreeRingBuffer {
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of 2");

public:
    LockFreeRingBuffer() : head_(0), tail_(0) {}

    ~LockFreeRingBuffer() = default;

    // Non-copyable, non-movable
    LockFreeRingBuffer(const LockFreeRingBuffer&) = delete;
    LockFreeRingBuffer& operator=(const LockFreeRingBuffer&) = delete;

    /**
     * @brief Pushes an item to the buffer.
     * @return true if pushed, false if buffer is full. Real-Time safe.
     */
    bool push(const T& item) noexcept {
        const size_t current_tail = tail_.load(std::memory_order_relaxed);
        const size_t current_head = head_.load(std::memory_order_acquire);

        if ((current_tail - current_head) >= Capacity) {
            return false; // Dolu
        }

        buffer_[current_tail & BufferMask] = item;
        tail_.store(current_tail + 1, std::memory_order_release);
        return true;
    }

    /**
     * @brief Pops an item from the buffer.
     * @return true if popped, false if buffer is empty. Real-Time safe.
     */
    bool pop(T& item) noexcept {
        const size_t current_head = head_.load(std::memory_order_relaxed);
        const size_t current_tail = tail_.load(std::memory_order_acquire);

        if (current_head == current_tail) {
            return false; // Boş
        }

        item = buffer_[current_head & BufferMask];
        head_.store(current_head + 1, std::memory_order_release);
        return true;
    }

    [[nodiscard]] size_t size() const noexcept {
        const size_t head = head_.load(std::memory_order_relaxed);
        const size_t tail = tail_.load(std::memory_order_relaxed);
        return (tail >= head) ? (tail - head) : 0;
    }

    [[nodiscard]] bool empty() const noexcept {
        return head_.load(std::memory_order_relaxed) == tail_.load(std::memory_order_relaxed);
    }

    void clear() noexcept {
        head_.store(0, std::memory_order_relaxed);
        tail_.store(0, std::memory_order_relaxed);
    }

private:
    static constexpr size_t BufferMask = Capacity - 1;
    T buffer_[Capacity];

    // Hardware destructive interference (false sharing) önlemek için cache line padding
    alignas(64) std::atomic<size_t> head_{0};
    alignas(64) std::atomic<size_t> tail_{0};
};

} // namespace polyrhythm
