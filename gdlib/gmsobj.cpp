#include "gmsobj.h"

namespace gdlib::gmsobj {

    void TQuickSortClass::SortN(int n) {
        if(n > 1)
            QuickSort(OneBased, n - 1 + OneBased);
    }

    void TQuickSortClass::QuickSort(int L, int R) {
        int i {L};
        while(i < R) {
            int j{R}, p{(L+R) >> 1};
            do {
                while(Compare(i,p)<0)i++;
                while(Compare(j,p) > 0)j--;
                if(i<j) {
                    Exchange(i,j);
                    if(p == i) p = j;
                    else if(p == j) p = i;
                    i++;
                    j--;
                } else if(i == j) {
                    i++;
                    j--;
                }
            } while(i <= j);
            // partition finished, now sort left and right
            // starting with the smaller piece to keep recursion in check
            if((j-L) > (R-i)) { // left part is bigger, look right first
                if(i < R) QuickSort(i,R); // sort the right part if necessary
                i = L; // move to the left part
                R = j;
            }
            else { // right part is bigger, look left first
                if(L < j) QuickSort(L,j); // sort the right part if necessary
                L = i;
            }
        }
    }

    int getSCHashSize(int itemCount) {
        int k{itemCount / SCHASH_FACTOR_MIN};
        int res{SCHASHSIZE0};
        for(int i{5}; i>=0; i--)
            if(k >= schashSizes[i])
                res = schashSizes[i+1];
        return res;
    }
}