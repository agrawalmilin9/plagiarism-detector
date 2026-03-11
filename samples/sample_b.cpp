// sample_b.cpp – copied and lightly renamed
#include <iostream>
#include <vector>
#include <algorithm>

// sort integers using bubble method
int doSort(std::vector<int>& arr) {
    int n = arr.size();
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            if (arr[j] > arr[j + 1]) {
                std::swap(arr[j], arr[j + 1]);
            }
        }
    }
    return n;
}

int main() {
    std::vector<int> nums = {64, 34, 25, 12, 22, 11, 90};
    doSort(nums);
    for (int v : nums) std::cout << v << " ";
    std::cout << std::endl;
    return 0;
}
