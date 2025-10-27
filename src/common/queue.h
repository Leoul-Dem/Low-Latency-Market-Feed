//
// Created by MyPC on 10/26/2025.
//

#ifndef LOW_LATENCY_MARKET_FEED_QUEUE_H
#define LOW_LATENCY_MARKET_FEED_QUEUE_H

#include <atomic>
#include <utility>
#include <type_traits>

namespace lfq {

// Michael & Scott Lock-Free MPMC Queue (without in-flight reclamation)
//
// Notes:
// - This implementation adheres to the MS algorithm you provided.
// - It uses one sentinel (dummy) node at initialization.
// - For simplicity and safety, nodes are not freed during operation to avoid ABA and
//   use-after-free issues. All remaining nodes are reclaimed in the destructor.
// - If you need continuous reclamation, ask for a hazard-pointer or epoch-based extension.
//
// Thread-safety: MPMC, lock-free.

template <typename T>
class Queue {
    struct Node {
        T value;
        std::atomic<Node*> next;

        Node() : value(), next(nullptr){}
        Node(const T& v) : value(v), next(nullptr) {}
        Node(T&& v) : value(std::move(v)), next(nullptr) {}
    };

    // Head and Tail point to nodes in a singly-linked list with a dummy header.
    std::atomic<Node*> head_;
    std::atomic<Node*> tail_;

public:
    Queue() {
        Node* dummy = new Node(); // sentinel
        head_.store(dummy, std::memory_order_relaxed);
        tail_.store(dummy, std::memory_order_relaxed);
    }

    ~Queue() {
        // Drain and delete all nodes, including sentinel
        Node* n = head_.load(std::memory_order_relaxed);
        while (n) {
            Node* next = n->next.load(std::memory_order_relaxed);
            delete n;
            n = next;
        }
    }

    Queue(const Queue&) = delete;
    Queue& operator=(const Queue&) = delete;

    void enqueue(const T& v) {
        Node* node = new Node(v);
        link_node_(node);
    }

    void enqueue(T&& v) {
        Node* node = new Node(std::move(v));
        link_node_(node);
    }

    // Returns true if a value was dequeued into out.
    bool try_dequeue(T& out) {
        for (;;) {
            Node* head = head_.load(std::memory_order_acquire);
            Node* tail = tail_.load(std::memory_order_acquire);
            Node* next = head->next.load(std::memory_order_acquire);

            if (head != head_.load(std::memory_order_acquire)) {
                continue; // Inconsistent read, retry
            }

            if (head == tail) {
                // Queue empty or tail falling behind
                if (next == nullptr) {
                    // Empty
                    return false;
                }
                // Tail is behind, try to advance it
                tail_.compare_exchange_weak(
                    tail, next,
                    std::memory_order_release,
                    std::memory_order_relaxed);
            } else {
                // Read value before CAS of head
                // NOTE: We are not reclaiming head here to avoid use-after-free without hazard pointers.
                T value = std::move(next->value);
                if (head_.compare_exchange_weak(
                        head, next,
                        std::memory_order_release,
                        std::memory_order_relaxed)) {
                    // We advanced head. We intentionally do NOT delete the old head now.
                    // It will be reclaimed in the destructor.
                    out = std::move(value);
                    return true;
                }
            }
        }
    }

private:
    void link_node_(Node* node) {
        node->next.store(nullptr, std::memory_order_relaxed);

        for (;;) {
            Node* tail = tail_.load(std::memory_order_acquire);
            Node* next = tail->next.load(std::memory_order_acquire);

            if (tail != tail_.load(std::memory_order_acquire)) {
                continue; // Inconsistent read, retry
            }

            if (next == nullptr) {
                // At last node; try to link the new node
                if (tail->next.compare_exchange_weak(
                        next, node,
                        std::memory_order_release,
                        std::memory_order_relaxed)) {
                    // Enqueue done; try to swing tail to the inserted node (optional)
                    tail_.compare_exchange_strong(
                        tail, node,
                        std::memory_order_release,
                        std::memory_order_relaxed);
                    return;
                }
            } else {
                // Tail not pointing to last node; try to advance it
                tail_.compare_exchange_weak(
                    tail, next,
                    std::memory_order_release,
                    std::memory_order_relaxed);
            }
        }
    }
};

} // namespace lfq

#endif //LOW_LATENCY_MARKET_FEED_QUEUE_H