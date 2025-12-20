#ifndef STREAM_HPP
#define STREAM_HPP

#include <string>
#include <vector>
#include <fstream>
#include <algorithm>
#include "LazySequence.hpp"
using namespace std;


// Интерфейс десериализации для чтения
template <typename T>
class Deserializer {
    public:
        virtual T Deserialize(const string &data) = 0;
        virtual ~Deserializer() = default;
};

// Интерфейс сериализации для записи
template <typename T>
class Serializer {
    public:
        virtual string Serialize(const T &item) = 0;
        virtual ~Serializer() = default;
};

// Интерфейс основного потока
template <typename T>
class Stream {
    protected:
        bool isOpen;
        size_t position;
    public:
        // Конструкторы
        Stream(): isOpen(false), position(0) {}
        virtual ~Stream() = default;

        // Декомпозиция
        bool IsOpen() const { return isOpen; }
        size_t GetPosition() const { return position; }

        // Операции
        virtual void Open() = 0;
        virtual void Close() = 0;
};

// Интерфейс потока для чтения
template <typename T>
class ReadableStream: virtual public Stream<T> {
    public:
        virtual ~ReadableStream() = default;
        virtual bool IsCanSeek() const = 0;
        virtual bool IsCanGoBack() const = 0;
        virtual bool IsEndOfStream() const = 0;
        virtual T Peek() const = 0;
        virtual T Read() = 0;
        virtual size_t Seek(size_t index) = 0;
        virtual shared_ptr<DynamicArray<T>> ReadBlock(size_t count) {
            auto block = make_shared<DynamicArray<T>>(count);
            size_t actualCount = 0;
            for (size_t i = 0; i < count && !IsEndOfStream(); i++) {
                try {
                    block->Set(i, Read());
                    actualCount++;
                } catch (...) {
                    break;
                }
            }
            if (actualCount < count) block->Resize(actualCount);
            return block;
        }
};

// Интерфейс потока для записи
template <typename T>
class WritableStream: virtual public Stream<T> {
    public:
        virtual ~WritableStream() = default;
        virtual size_t Write(const T &item) = 0;
        virtual size_t WriteAll(shared_ptr<Sequence<T>> seq) = 0;
        virtual size_t WriteBlock(shared_ptr<DynamicArray<T>> arr) = 0;
};

// Класс потока для чтения
template <typename T>
class ReadOnlyStream: public ReadableStream<T> {
    protected:
        shared_ptr<LazySequence<T>> data;
        shared_ptr<Deserializer<T>> deserializer;
        ifstream fileStream;
        bool canSeek;
        bool canGoBack;
    public:
        // Конструкторы
        ~ReadOnlyStream() override { try { Close(); } catch (...) {} }

        ReadOnlyStream(shared_ptr<Sequence<T>> seq):
            Stream<T>(), data(make_shared<LazySequence<T>>(seq)), deserializer(nullptr), canSeek(true), canGoBack(true) {}
        
        ReadOnlyStream(shared_ptr<LazySequence<T>> lazySeq):
            Stream<T>(), data(lazySeq), deserializer(nullptr), canSeek(true), canGoBack(true) {}
        
        ReadOnlyStream(const string &filename, shared_ptr<Deserializer<T>> deser):
            Stream<T>(), data(nullptr), deserializer(deser), canSeek(true), canGoBack(true) {
            if (!deser) throw runtime_error("Десериализатор не может быть пустым!");
            fileStream.open(filename);
            if (!fileStream.is_open()) throw runtime_error("Невозможно открыть файл: " + filename);
        }
        
        ReadOnlyStream(const string &dataString, shared_ptr<Deserializer<T>> deser, char delimiter):
            Stream<T>(), data(nullptr), deserializer(deser), canSeek(true), canGoBack(true) {
            if (!deser) throw runtime_error("Десериализатор не может быть пустым!");
            vector<T> tempItems;
            size_t start = 0, end = 0;
            while ((end = dataString.find(delimiter, start)) != string::npos) {
                string itemStr = dataString.substr(start, end-start);
                if (!itemStr.empty()) {
                    try {
                        tempItems.push_back(deserializer->Deserialize(itemStr));
                    } catch (const exception& e) {
                        throw runtime_error("Ошибка десериализации: " + string(e.what()));
                    }
                }
                start = end + 1;
            }
            if (start < dataString.length()) {
                string itemStr = dataString.substr(start);
                if (!itemStr.empty()) {
                    try {
                        tempItems.push_back(deserializer->Deserialize(itemStr));
                    } catch (const exception& e) {
                        throw runtime_error("Ошибка десериализации: " + string(e.what()));
                    }
                }
            }
            if (!tempItems.empty()) {
                auto tempArray = make_shared<DynamicArray<T>>(tempItems.size());
                for (size_t i = 0; i < tempItems.size(); i++) {
                    tempArray->Set(i, tempItems[i]);
                }
                data = make_shared<LazySequence<T>>(tempArray);
            } else {
                data = make_shared<LazySequence<T>>();
            }
        }

        // Декомпозиция
        bool IsCanSeek() const override { return canSeek; }

        bool IsCanGoBack() const override { return canGoBack; }

        bool IsEndOfStream() const override {
            if (!this->isOpen) return true;
            if (data) {
                try {
                    return this->position >= data->GetLength();
                } catch (const runtime_error &e) {
                    try {
                        data->Get(this->position);
                        return false;
                    } catch (const out_of_range&) {
                        return true;
                    } catch (const runtime_error&) {
                        return true;
                    }
                }
            }
            return false;
        }

        T Peek() const override {
            if (!this->isOpen) throw runtime_error("Поток не открыт!");
            if (IsEndOfStream()) throw runtime_error("Достигнут конец потока!");
            return data->Get(this->position);
        }
        
        T Read() override {
            if (!this->isOpen) throw runtime_error("Поток не открыт!");
            if (IsEndOfStream()) throw runtime_error("Достигнут конец потока!");
            try {
                T item = data->Get(this->position);
                this->position++;
                return item;
            } catch (const out_of_range &e) {
                throw runtime_error("Достигнут конец потока!");
            }
        }

        size_t Seek(size_t index) override {
            if (!this->isOpen) throw runtime_error("Поток не открыт!");
            if (!IsCanSeek()) throw runtime_error("Перемещение по потоку не поддерживается!");
            if (data) {
                try {
                    if (index >= data->GetLength()) throw out_of_range("Индекс за пределами потока!");
                } catch (const runtime_error&) {}
            }
            this->position = index;
            return this->position;
        }

        shared_ptr<LazySequence<T>> GetData() const { return data; }

        // Операции
        void Open() override {
            if (this->isOpen) return;
            if (data) {
                this->isOpen = true;
                return;
            }
            if (fileStream.is_open() && deserializer) {
                auto fileReader = [this]() -> T {
                    string line;
                    if (getline(this->fileStream, line)) {
                        return this->deserializer->Deserialize(line);
                    }
                    throw runtime_error("Конец файла");
                };
                auto hasNext = [this]() -> bool {
                    return this->fileStream && !this->fileStream.eof();
                };
                auto gen = make_shared<Generator<T>>(fileReader, hasNext);
                data = make_shared<LazySequence<T>>(gen);
                this->isOpen = true;
                return;
            }
            throw runtime_error("Не удалось открыть поток: нет данных!");
        }
        
        void Close() override {
            if (!this->isOpen) return;
            if (fileStream.is_open()) {
                fileStream.close();
            }
            this->isOpen = false;
            this->position = 0;
        }
};

// Класс потока для записи
template <typename T>
class WriteOnlyStream: public WritableStream<T> {
    protected:
        shared_ptr<DynamicArray<T>> outputBuffer;
        shared_ptr<Serializer<T>> serializer;
        ofstream fileStream;
        size_t bufferSize;
    public:
        // Конструкторы
        ~WriteOnlyStream() override { try { Close(); } catch (...) {} }

        WriteOnlyStream(shared_ptr<DynamicArray<T>> buffer):
            Stream<T>(), outputBuffer(buffer), serializer(nullptr), bufferSize(buffer->GetSize()) {}

        WriteOnlyStream(const string &filename, shared_ptr<Serializer<T>> ser):
            Stream<T>(), outputBuffer(nullptr), serializer(ser), bufferSize(0) {
            if (!ser) throw runtime_error("Сериализатор не может быть пустым!");
            fileStream.open(filename);
            if (!fileStream.is_open()) throw runtime_error("Невозможно открыть файл: " + filename);
        }

        // Декомпозиция
        size_t GetBufferSize() const { return bufferSize; }
        shared_ptr<DynamicArray<T>> GetBuffer() const { return outputBuffer; }

        // Операции
        void Open() override {
            if (this->isOpen) return;
            if (outputBuffer || (fileStream.is_open() && serializer)) {
                this->isOpen = true;
                return;
            }
            throw runtime_error("Не удалось открыть поток для записи: нет приемника данных!");
        }
        
        void Close() override {
            if (!this->isOpen) return;
            if (fileStream.is_open()) {
                fileStream.close();
            }
            this->isOpen = false;
            this->position = 0;
        }
        
        size_t Write(const T &item) override {
            if (!this->isOpen) throw runtime_error("Поток не открыт!");
            if (outputBuffer) {
                if (this->position < bufferSize) {
                    outputBuffer->Set(this->position, item);
                } else {
                    outputBuffer->Resize(bufferSize+1);
                    outputBuffer->Set(bufferSize, item);
                    bufferSize++;
                }
            } else if (fileStream.is_open() && serializer) {
                fileStream << serializer->Serialize(item) << endl;
                if (!fileStream.good()) throw runtime_error("Ошибка записи в файл");
            }
            this->position++;
            return this->position;
        }

        size_t WriteAll(shared_ptr<Sequence<T>> seq) override {
            for (size_t i = 0; i < seq->GetLength(); i++) {
                Write(seq->Get(i));
            }
            return this->position;
        }

        size_t WriteBlock(shared_ptr<DynamicArray<T>> arr) override {
            for (size_t i = 0; i < arr->GetSize(); i++) {
                Write(arr->Get(i));
            }
            return this->position;
        }
};

// Класс потока для чтения и записи
template <typename T>
class ReadWriteStream: public ReadableStream<T>, public WritableStream<T> {
    protected:
        shared_ptr<LazySequence<T>> readData;
        shared_ptr<DynamicArray<T>> writeBuffer;
        shared_ptr<Deserializer<T>> deserializer;
        shared_ptr<Serializer<T>> serializer;
        fstream fileStream;
        string filename;
        size_t writeBufferSize;
        bool canSeek;
        bool canGoBack;
        bool isFileMode;

        void ReloadFileData() {
            if (!isFileMode || !deserializer) return;
            fileStream.close();
            fileStream.open(filename, ios::in | ios::out);
            if (!fileStream.is_open()) throw runtime_error("Не удалось переоткрыть файл: " + filename);
            auto fileReader = [this]() -> T {
                string line;
                if (getline(this->fileStream, line)) {
                    return this->deserializer->Deserialize(line);
                }
                throw runtime_error("Конец файла");
            };
            auto hasNext = [this]() -> bool {
                return this->fileStream && !this->fileStream.eof();
            };
            auto gen = make_shared<Generator<T>>(fileReader, hasNext);
            readData = make_shared<LazySequence<T>>(gen);
        }
    public:
        // Конструкторы
        ~ReadWriteStream() override { try { Close(); } catch (...) {} }

        ReadWriteStream(shared_ptr<LazySequence<T>> readSeq, shared_ptr<DynamicArray<T>> writeBuf = nullptr): 
            Stream<T>(), readData(readSeq), writeBuffer(writeBuf), writeBufferSize(0),
            deserializer(nullptr), serializer(nullptr), canSeek(true), canGoBack(true), isFileMode(false) {
            if (!writeBuffer) writeBuffer = make_shared<DynamicArray<T>>(0);
            writeBufferSize = writeBuffer->GetSize();
        }
        
        ReadWriteStream(const string &filename, shared_ptr<Deserializer<T>> deser = nullptr, 
                        shared_ptr<Serializer<T>> ser = nullptr, ios_base::openmode mode = ios::in | ios::out):
            Stream<T>(), filename(filename), readData(nullptr), writeBuffer(nullptr), writeBufferSize(0),
            deserializer(deser), serializer(ser), canSeek(true), canGoBack(true), isFileMode(true) {
            if ((mode & ios::in) && !deser) throw runtime_error("Режим чтения требует десериализатор!");
            if ((mode & ios::out) && !ser) throw runtime_error("Режим записи требует сериализатор");
            fileStream.open(filename, mode);
            if (!fileStream.is_open()) throw runtime_error("Невозможно открыть файл: " + filename);
        }
        
        // Декомпозиция (чтение)
        bool IsCanSeek() const override { return canSeek; }

        bool IsCanGoBack() const override { return canGoBack; }

        bool IsEndOfStream() const override {
            if (!this->isOpen) return true;
            if (readData) {
                try {
                    return this->position >= readData->GetLength();
                } catch (const runtime_error &e) {
                    try {
                        readData->Get(this->position);
                        return false;
                    } catch (const out_of_range&) {
                        return true;
                    } catch (const runtime_error&) {
                        return true;
                    }
                }
            }
            return false;
        }

        T Peek() const override {
            if (!this->isOpen) throw runtime_error("Поток не открыт!");
            if (IsEndOfStream()) throw runtime_error("Достигнут конец потока!");
            return readData->Get(this->position);
        }

        T Read() override {
            if (!this->isOpen) throw runtime_error("Поток не открыт!");
            if (IsEndOfStream()) throw runtime_error("Достигнут конец потока!");
            try {
                T item = readData->Get(this->position);
                this->position++;
                return item;
            } catch (const out_of_range &e) {
                throw runtime_error("Достигнут конец потока!");
            }
        }

        size_t Seek(size_t index) override {
            if (!this->isOpen) throw runtime_error("Поток не открыт!");
            if (!IsCanSeek()) throw runtime_error("Перемещение не поддерживается!");
            try {
                if (readData->GetLength() != 0 && index >= readData->GetLength()) {
                    throw out_of_range("Индекс за пределами потока!");
                }
            } catch (const runtime_error &e) {}
            this->position = index;
            return this->position;
        }

        shared_ptr<LazySequence<T>> GetReadData() const { return readData; }
        
        // Декомпозиция (запись)
        size_t GetWriteBufferSize() const { return writeBufferSize; }
        shared_ptr<DynamicArray<T>> GetWriteBuffer() const { return writeBuffer; }

        // Операции
        void Open() override {
            if (this->isOpen) return;
            if (isFileMode) {
                if (!fileStream.is_open()) {
                    fileStream.open(filename, ios::in | ios::out);
                    if (!fileStream.is_open()) throw runtime_error("Не удалось открыть файл: " + filename);
                }
                if (deserializer && !readData) {
                    auto fileReader = [this]() -> T {
                        string line;
                        if (getline(this->fileStream, line)) {
                            return this->deserializer->Deserialize(line);
                        }
                        throw runtime_error("Конец файла");
                    };
                    auto hasNext = [this]() -> bool {
                        return this->fileStream && !this->fileStream.eof();
                    };
                    auto gen = make_shared<Generator<T>>(fileReader, hasNext);
                    readData = make_shared<LazySequence<T>>(gen);
                }
                this->isOpen = true;
                return;
            }
            if (readData || writeBuffer) {
                this->isOpen = true;
                return;
            }
            throw runtime_error("Не удалось открыть поток!");
        }
        
        void Close() override {
            if (!this->isOpen) return;
            if (fileStream.is_open()) {
                fileStream.close();
            }
            this->isOpen = false;
            this->position = 0;
        }

        size_t Write(const T &item) override {
            if (!this->isOpen) throw runtime_error("Поток не открыт!");
            if (writeBuffer) {
                if (this->position < writeBufferSize) {
                    writeBuffer->Set(this->position, item);
                } else {
                    writeBuffer->Resize(writeBufferSize + 1);
                    writeBuffer->Set(writeBufferSize, item);
                    writeBufferSize++;
                }
            } else if (isFileMode && serializer) {
                size_t writePos = this->position;
                vector<string> lines;
                fileStream.clear();
                fileStream.seekg(0, ios::beg);
                string line;
                while (getline(fileStream, line)) {
                    lines.push_back(line);
                }
                if (writePos >= lines.size()) {
                    lines.resize(writePos+1);
                }
                lines[writePos] = serializer->Serialize(item);
                fileStream.close();
                fileStream.open(filename, ios::out | ios::trunc);
                if (!fileStream.is_open()) throw runtime_error("Не удалось открыть файл для записи: " + filename);
                for (const auto &l : lines) {
                    fileStream << l << endl;
                }
                fileStream.flush();
                fileStream.close();
                fileStream.open(filename, ios::in | ios::out);
                if (!fileStream.is_open()) throw runtime_error("Не удалось открыть файл для записи: " + filename);
                ReloadFileData();
            }
            this->position++;
            return this->position;
        }

        size_t WriteAll(shared_ptr<Sequence<T>> seq) override {
            for (size_t i = 0; i < seq->GetLength(); i++) {
                Write(seq->Get(i));
            }
            return this->position;
        }

        size_t WriteBlock(shared_ptr<DynamicArray<T>> arr) override {
            for (size_t i = 0; i < arr->GetSize(); i++) {
                Write(arr->Get(i));
            }
            return this->position;
        }
};

// Класс десериализатора для int
class IntDeserializer: public Deserializer<int> {
    public:
        int Deserialize(const string &data) override { return stoi(data); }
};

// Класс десериализатора для string
class StringDeserializer: public Deserializer<string> {
    public:
        string Deserialize(const string &data) override { return data; }
};

// Класс сериализатора для int
class IntSerializer: public Serializer<int> {
    public:
        string Serialize(const int &item) override { return to_string(item); }
};

// Класс сериализатора для string  
class StringSerializer: public Serializer<string> {
    public:
        string Serialize(const string &item) override { return item; }
};

#endif // STREAM_HPP
