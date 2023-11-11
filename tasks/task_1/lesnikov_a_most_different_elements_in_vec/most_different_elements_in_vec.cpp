// Copyright 2023 Kulikov Artem
#include <iostream>
#include <utility>
#include <vector>
#include <string>
#include <random>
#include <algorithm>
#include <functional>
#include <boost/mpi/collectives.hpp>
#include <boost/mpi/communicator.hpp>
#include "task_1/lesnikov_a_most_different_elements_in_vec/most_different_elements_in_vec.h"


std::vector<int> getRandomVector(int n) {
    std::random_device dev;
    std::mt19937 gen(dev());
    std::vector<int> random_vector;

    for (size_t i = 0; i < n; i++) {
        random_vector.push_back(gen() % 100);
    }

    return random_vector;
}


std::vector<std::pair<size_t, size_t>> getSequentialMostDifferentElements(std::vector<int> v) {
    std::vector<std::pair<size_t, size_t>> most_different_elements;
    int max_different_elements_value = 0;

    for (size_t i = 0; i < v.size() - 1; i++) {
        max_different_elements_value = std::max(max_different_elements_value, std::abs(v[i] - v[i+1]));
    }

    for (size_t i = 0; i < v.size() - 1; i++) {
        if (std::abs(v[i] - v[i+1]) == max_different_elements_value) {
            most_different_elements.push_back(std::make_pair(i, i + 1));
        }
    }

    return most_different_elements;
}

std::vector<std::pair<size_t, size_t>> getParallelMostDifferentElements(std::vector<int> v, int n) {
    boost::mpi::communicator world;

    if (n < 2) {
        return std::vector<std::pair<size_t, size_t>>();
    }

    std::vector<int> empty;
    std::vector<size_t> temp2;
    std::vector<int> temp;
    std::vector<int> loc_v;
    std::vector<size_t> glob_most_different_elements;
    std::vector<std::pair<size_t, size_t>> glob_most_different_elements_pairs;
    std::vector<size_t> loc_most_different_elements;
    const int part_size = n / world.size();
    const int remainder = n % world.size();

    if (world.rank() == 0) {
        loc_v = v;
        int loc_part_size_root = world.size() > 1 ? part_size + remainder + 1 : part_size + remainder;
        loc_v.resize(loc_part_size_root);
        for (size_t i = 1; i < world.size(); i++) {
            int loc_part_size = i == world.size() - 1 ? part_size : part_size + 1;
            int start = remainder + i * part_size;
            int end = remainder + i * part_size + loc_part_size;
            temp = std::vector<int>(v.begin() + start, v.begin() + end);

            world.recv(i, 0, empty);
            world.send(i, 0, temp);
        }

    } else {
        world.send(0, 0, empty);
        world.recv(0, 0, loc_v);
    }

    int max_different_elements_value = 0;

    for (int i = 0; i < loc_v.size() - 1; i++) {
        max_different_elements_value = std::max(max_different_elements_value, std::abs(loc_v[i] - loc_v[i+1]));
    }

    for (int i = 0; i < loc_v.size() - 1; i++) {
        if (std::abs(loc_v[i] - loc_v[i+1]) == max_different_elements_value) {
            loc_most_different_elements.push_back(i + world.rank() * part_size
            + remainder * static_cast<int>(world.size() != 0));
        }
    }

    glob_most_different_elements = std::move(loc_most_different_elements);

    if (world.rank() == 0) {
        for (int i = 1; i < world.size(); i++) {
            world.send(i, 0, empty);
            world.recv(i, 0, temp2);

            if (!glob_most_different_elements.size()) {
                glob_most_different_elements = std::move(temp2);
            } else if (temp2.size()) {
                int glob_diff = std::abs(v[glob_most_different_elements[0]] - v[glob_most_different_elements[0] + 1]);
                int temp_diff = std::abs(v[temp2[0]] - v[temp2[0] + 1]);
                if (glob_diff < temp_diff) {
                    glob_most_different_elements = std::move(temp2);
                } else if (glob_diff == temp_diff) {
                    glob_most_different_elements.insert(
                    glob_most_different_elements.end(), temp2.begin(), temp2.end());
                }
            }
        }
    } else {
        world.recv(0, 0, empty);
        world.send(0, 0, glob_most_different_elements);
    }

    for (const size_t& i : glob_most_different_elements) {
        glob_most_different_elements_pairs.push_back(std::make_pair(i, i + 1));
    }

    world.barrier();

    return glob_most_different_elements_pairs;
}
