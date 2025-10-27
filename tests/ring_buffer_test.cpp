//
// Created by MyPC on 10/26/2025.
//
#include "../src/common/ring_buffer.h"
#include <thread>
#include <iostream>
#include <vector>


void write_(RingBuffer<char>& r_buf){
    char message[] = {'h','e','l','l','o', ' ', 'w','o','r','l','d'};
    for(int i = 0; i < 11; i++){
        r_buf.write([&](char& w){
          w = message[i];
        });
    }
}

void read_(RingBuffer<char>& r_buf, std::vector<char>& output){
    for(int i = 0; i < 11; i++){
        r_buf.read([&](const char& w){
          output.push_back(w);
        });
    }
}

int main(){

    RingBuffer<char> r_buf(16);
    std::vector<char> output;

    std::thread w(write_, std::ref(r_buf));
    std::thread r(read_, std::ref(r_buf), std::ref(output));

    w.join();
    r.join();

    for(auto i : output){
        std::cout << i;
    }
    std::cout << std::endl;

    return 0;
}