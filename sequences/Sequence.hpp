#ifndef SEQUENCE_HPP
#define SEQUENCE_HPP

#include <functional>


template <typename T>
class Sequence {
    public:
        virtual ~Sequence() = default;

        // Декомпозиция
        virtual size_t GetLength() const = 0;
        virtual T Get(size_t index) const = 0;
        virtual T GetFirst() const = 0;
        virtual T GetLast() const = 0;

        // Перегрузка операторов
        virtual T& operator[](size_t index) = 0;
        virtual const T& operator[](size_t index) const = 0;

        // Операции
        virtual Sequence<T>* Append(T item) = 0;
        virtual Sequence<T>* Prepend(T item) = 0;
        virtual Sequence<T>* Remove(size_t index) = 0;
        virtual Sequence<T>* InsertAt(T item, size_t index) = 0;
        virtual Sequence<T>* PutAt(T item, size_t index) = 0;
        virtual Sequence<T>* Concat(Sequence<T> *other) = 0;
        virtual Sequence<T>* GetSubsequence(size_t startIndex, size_t endIndex) = 0;
};

#endif // SEQUENCE_HPP
