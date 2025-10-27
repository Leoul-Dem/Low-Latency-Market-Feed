cmake_minimum_required(VERSION 4.0)
project(Low_Latency_Market_Feed)

set(CMAKE_CXX_STANDARD 20)

add_executable(Low_Latency_Market_Feed main.cpp
        src/aggregate/aggregator.cpp
        src/aggregate/order_book.h
        src/analytics/analytics.cpp
        src/analytics/metrics.h
        src/common/ring_buffer.h
        src/common/queue.h
        src/feed/binance_feed_parser.h
        src/feed/feed_handler.cpp
        src/normalize/message_format.h
        src/normalize/normalize.cpp
        src/output/publisher.cpp
        tests/queue_test.cpp
        tests/ring_buffer_test.cpp
        benchmarks/latency_test.cpp
        benchmarks/throughput_test.cpp)
