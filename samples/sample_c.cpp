// sample_c.cpp – different original work
#include <iostream>
#include <vector>

int binarySearch(const std::vector<int>& arr, int target) {
    int lo = 0, hi = (int)arr.size() - 1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        if (arr[mid] == target) return mid;
        else if (arr[mid] < target) lo = mid + 1;
        else hi = mid - 1;
    }
    return -1;
}

int main() {
    std::vector<int> sorted = {1, 3, 5, 7, 9, 11, 13};
    int idx = binarySearch(sorted, 7);
    std::cout << "Found at index: " << idx << "\n";
    return 0;
}
