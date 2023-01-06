#pragma once

#include <vector>
#include <string>

namespace gmsobj {

    class TQuickSortClass {
        void QuickSort(int L, int R);
    public:
        bool OneBased;
        virtual void Exchange(int Index1, int Index2) = 0;
        virtual int Compare(int Index1, int Index2) = 0;
        void SortN(int n);
    };

    template<typename T>
    struct TStringItem {
        std::string FString;
        T *FObject;
    };

    template<typename T>
    class TXCustomStringList : public TQuickSortClass {
        std::vector<TStringItem<T>> FList;
    public:
        std::string GetName(int Index) const {
            return FList[Index].FString;
        }

        int Count() const { return FList.size(); }
    };

    template<typename T>
    class TXHashedStringList : public TXCustomStringList<T> {
    public:
        int AddObject(const std::string &s, T *APointer) {
            // FIXME: Implement this!
            return 0;
        }

        int Add(const std::string &S) {
            return AddObject(S, nullptr);
        }
    };

    template<typename T>
    class TXStrPool : public TXHashedStringList<T> {
    public:
        int Compare(int Index1, int Index2) override {
            // FIXME: Implement this!
            return 0;
        }
    };

}
