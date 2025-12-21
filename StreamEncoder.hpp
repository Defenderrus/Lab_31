#ifndef STREAMENCODER_HPP
#define STREAMENCODER_HPP

#include "Stream.hpp"


class StreamEncoder {
    public:
        // RLE кодирование
        static void RLEEncode(shared_ptr<ReadableStream<char>> input, shared_ptr<WritableStream<string>> output) {
            input->Open();
            output->Open();
            char prev = '\0';
            int count = 0;
            bool first = true;
            while (!input->IsEndOfStream()) {
                char current = input->Read();
                if (first) {
                    prev = current;
                    count = 1;
                    first = false;
                } else if (current == prev) {
                    count++;
                } else {
                    output->Write(to_string(count)+prev);
                    prev = current;
                    count = 1;
                }
            }
            if (!first) output->Write(to_string(count) + prev);
            input->Close();
            output->Close();
        }
        
        // RLE декодирование
        static void RLEDecode(shared_ptr<ReadableStream<string>> input, shared_ptr<WritableStream<char>> output) {
            input->Open();
            output->Open();
            while (!input->IsEndOfStream()) {
                string encoded = input->Read();
                if (encoded.length() < 2) continue;
                int count = 0;
                size_t i = 0;
                while (i < encoded.length() && isdigit(encoded[i])) {
                    count = count*10+(encoded[i]-'0');
                    i++;
                }
                if (i < encoded.length()) {
                    char symbol = encoded[i];
                    for (int j = 0; j < count; j++) {
                        output->Write(symbol);
                    }
                }
            }
            input->Close();
            output->Close();
        }
};

#endif // STREAMENCODER_HPP
