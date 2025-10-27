//
// Created by MyPC on 10/26/2025.
//

#ifndef LOW_LATENCY_MARKET_FEED_RING_BUFFER_H
#define LOW_LATENCY_MARKET_FEED_RING_BUFFER_H

#include <array>
#include <mutex>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <atomic>
#include <functional>

template <typename T>
class RingBuffer{

  struct obj_t{
    T data_;
    std::mutex mtx;
  };

  struct pointer_t{
    obj_t* ptr;
    std::atomic<std::uint8_t> idx;

    pointer_t() : ptr(nullptr), idx(0) {}
    pointer_t(obj_t* ptr, std::uint8_t idx) : ptr(ptr), idx(idx) {}

    bool operator==(const pointer_t& other) const {
      return idx.load() == other.idx.load();
    }

    bool operator!=(const pointer_t& other) const {
      return idx.load() != other.idx.load();
    }
  };

static const size_t MAX_RING_SIZE = 32;
  std::array<obj_t, MAX_RING_SIZE> ring;
  size_t size;
  pointer_t writer;
  pointer_t reader;

public:
  RingBuffer(size_t size) : ring(), size(size), writer(&ring[0], 0), reader(&ring[0], 0) {
    if(size < 2){
      throw std::out_of_range("Size must be between 2 and " + std::to_string(MAX_RING_SIZE));
    }else if(size > MAX_RING_SIZE){
      throw std::out_of_range("Size must be between 2 and " + std::to_string(MAX_RING_SIZE));
    }
  }

  void write(T some_data){
    // Calculate next writer position
    std::uint8_t current_writer_idx = writer.idx.load();
    std::uint8_t next_writer_idx = (current_writer_idx + 1) % size;

    // Wait if buffer is full (next position would be reader)
    while (next_writer_idx == reader.idx.load()){
      usleep(500);
    }

    std::lock_guard<std::mutex> lock(writer.ptr->mtx);
    writer.ptr->data_ = some_data;

    // Move to next position (atomic update)
    writer.idx.store(next_writer_idx);
    writer.ptr = &ring[next_writer_idx];
  }

  T read(T some_data){
    // Wait while buffer is empty (reader caught up to writer)
    while (reader.idx.load() == writer.idx.load()){
      usleep(1000);
    }

    std::lock_guard<std::mutex> lock(reader.ptr->mtx);
    T temp = reader.ptr->data_;

    // Move to next position (atomic update)
    std::uint8_t next_reader_idx = (reader.idx.load() + 1) % size;
    reader.idx.store(next_reader_idx);
    reader.ptr = &ring[next_reader_idx];

    return temp;
  }

  void write(std::function<T&> func){
    // Calculate next writer position
    std::uint8_t current_writer_idx = writer.idx.load();
    std::uint8_t next_writer_idx = (current_writer_idx + 1) % size;

    // Wait if buffer is full (next position would be reader)
    while (next_writer_idx == reader.idx.load()){
      usleep(500);
    }

    std::lock_guard<std::mutex> lock(writer.ptr->mtx);
    func(writer.ptr->data_);

    // Move to next position (atomic update)
    writer.idx.store(next_writer_idx);
    writer.ptr = &ring[next_writer_idx];
  }

  void read(std::function<const T&> func){
    // Wait while buffer is empty (reader caught up to writer)
    while (reader.idx.load() == writer.idx.load()){
      usleep(1000);
    }

    std::lock_guard<std::mutex> lock(reader.ptr->mtx);
    func(reader.ptr->data_);

    // Move to next position (atomic update)
    std::uint8_t next_reader_idx = (reader.idx.load() + 1) % size;
    reader.idx.store(next_reader_idx);
    reader.ptr = &ring[next_reader_idx];
  }

  ~RingBuffer(){

  }

};


#endif //LOW_LATENCY_MARKET_FEED_RING_BUFFER_H