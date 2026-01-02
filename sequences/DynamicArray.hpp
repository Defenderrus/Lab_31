#ifndef DYNAMICARRAY_HPP
#define DYNAMICARRAY_HPP


template <typename T>
class DynamicArray {
    private:
        T *data;
        size_t size;
    public:
        // Создание объекта
        DynamicArray(): size(0), data(nullptr) {}

        ~DynamicArray() { delete[] data; }

        DynamicArray(size_t count): size(count), data(new T[count]) {}

        DynamicArray(T *items, size_t count): size(count), data(new T[count]) {
            if (count) {
                for (size_t i = 0; i < count; i++) {
                    data[i] = items[i];
                }
            }
        }

        DynamicArray(const DynamicArray<T> &other): size(other.size), data(new T[other.size]) {
            if (other.size) {
                for (size_t i = 0; i < other.size; i++) {
                    data[i] = other.data[i];
                }
            }
        }

        DynamicArray(DynamicArray<T> &&other) noexcept: data(other.data), size(other.size) {
            other.data = nullptr;
            other.size = 0;
        }

        // Декомпозиция
        size_t GetSize() const { return size; }

        T Get(size_t index) const {
            if (size > index) {
                return data[index];
            }
            throw std::out_of_range("Некорректный индекс!");
        }

        // Перегрузка операторов
        T& operator[](size_t index) {
            if (size > index) {
                return data[index];
            }
            throw std::out_of_range("Некорректный индекс!");
        }

        const T& operator[](size_t index) const {
            if (size > index) {
                return data[index];
            }
            throw std::out_of_range("Некорректный индекс!");
        }

        DynamicArray<T>& operator=(DynamicArray<T> &&other) noexcept {
            if (this != &other) {
                delete[] data;
                size = other.size;
                data = other.data;
                other.size = 0;
                other.data = nullptr;
            }
            return *this;
        }
        
        // Операции
        void Set(size_t index, T value) {
            if (size > index) {
                data[index] = value;
            } else {
                throw std::out_of_range("Некорректный индекс!");
            }
        }

        void Resize(size_t newSize) {
            if (newSize == size) return;
            if (newSize == 0) {
                delete[] data;
                data = nullptr;
                size = 0;
                return;
            }
            T *newData = new T[newSize];
            for (size_t i = 0; i < std::min(size, newSize); i++) {
                newData[i] = std::move(data[i]);
            }
            for (size_t i = std::min(size, newSize); i < newSize; i++) {
                newData[i] = T();
            }
            delete[] data;
            data = newData;
            size = newSize;
        }
};

#endif // DYNAMICARRAY_HPP
