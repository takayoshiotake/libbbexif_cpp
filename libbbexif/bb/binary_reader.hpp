//
//  binary_reader.h
//
//  Created by OTAKE Takayoshi on 2017/06/03.
//  Copyright © 2017 OTAKE Takayoshi. All rights reserved.
//

#pragma once

// [C++14]

#include <cstdint>
#include <cstring>
#include <memory>
#include <istream>

namespace bb {
    enum class byte_order_t {
        little_endian,
        big_endian,
        native,
    };
    
    inline static byte_order_t native_byte_order() {
        constexpr uint16_t flag = 0x0001;
        return *reinterpret_cast<uint8_t const*>(&flag) == 0x01 ? byte_order_t::little_endian : byte_order_t::big_endian;
    }
    
    struct memory_reader {
        uint8_t const* ptr_;
        size_t size_;
        size_t cursor_ = 0;
        
        memory_reader() noexcept
        : ptr_(nullptr), size_(0) {
        }
        
        memory_reader(uint8_t const* ptr, size_t const size) noexcept
        : ptr_(ptr), size_(size) {
        }
        
        memory_reader(memory_reader const& obj) noexcept
        : ptr_(obj.ptr_), size_(obj.size_) {
        }
        
        memory_reader(memory_reader&& obj) noexcept
        : ptr_(obj.ptr_), size_(obj.size_) {
            obj.ptr_ = nullptr;
            obj.size_ = 0;
            obj.cursor_ = 0;
        }
        
        memory_reader const& operator=(memory_reader const& obj) noexcept {
            ptr_ = obj.ptr_;
            size_ = obj.size_;
            cursor_ = obj.cursor_;
            return *this;
        }
        
        memory_reader const& operator=(memory_reader&& obj) noexcept {
            ptr_ = obj.ptr_;
            size_ = obj.size_;
            cursor_ = obj.cursor_;
            obj.ptr_ = nullptr;
            obj.size_ = 0;
            obj.cursor_ = 0;
            return *this;
        }
        
        inline void reset(uint8_t const* ptr, size_t const size) noexcept {
            ptr_ = ptr;
            size_ = size;
            cursor_ = 0;
        }
        
        inline uint8_t const* ptr() const noexcept {
            return ptr_;
        }
        
        inline size_t size() const noexcept {
            return size_;
        }
        
        inline size_t cursor() const noexcept {
            return cursor_;
        }
        
        inline size_t available() const {
            return available(cursor_);
        }
        
        inline size_t available(size_t const cursor) const {
            if (size_ < cursor) {
                return 0;
            }
            return size_ - cursor;
        }
        
        inline void move_to(size_t const cursor) {
            cursor_ = cursor;
        }
        
        inline void move(int move) {
            cursor_ += move;
        }
        
        inline uint8_t read() {
            return ptr_[cursor_++];
        }
        
        inline void read(uint8_t* dst, size_t size) {
            std::memcpy(dst, ptr_ + cursor_, size);
            cursor_ += size;
        }
        
        inline uint8_t peek() const {
            return peek(cursor_);
        }
        
        inline void peek(uint8_t* dst, size_t const size) const {
            peek(cursor_, dst, size);
        }
        
        inline uint8_t peek(size_t const cursor) const {
            return ptr_[cursor];
        }
        
        inline void peek(size_t const cursor, uint8_t* dst, size_t const size) const {
            std::memcpy(dst, ptr_ + cursor, size);
        }
    };
    
    // function binary_reader
    
    template <typename _Readable> struct binary_readable_traits;
    
    template <typename _T, typename _Readable, typename binary_readable_policy = binary_readable_traits<_Readable>>
    inline _T read(_Readable& readable) {
        return binary_readable_policy::template read<_T>(readable);
    }
    
    template <typename _T, typename _Readable, typename binary_readable_policy = binary_readable_traits<_Readable>>
    inline _T read(_Readable& readable, byte_order_t const byte_order) {
        return binary_readable_policy::template read<_T>(readable, byte_order);
    }
    
    template <typename _T, typename _Readable, typename binary_readable_policy = binary_readable_traits<_Readable>>
    inline _T peek(_Readable& readable) {
        return binary_readable_policy::template peek<_T>(readable);
    }
    
    template <typename _T, typename _Readable, typename binary_readable_policy = binary_readable_traits<_Readable>>
    inline _T peek(_Readable& readable, byte_order_t const byte_order) {
        return binary_readable_policy::template peek<_T>(readable, byte_order);
    }
    
    template <typename _T, typename _Readable, typename binary_readable_policy = binary_readable_traits<_Readable>>
    inline _T peek(_Readable& readable, size_t const cursor) {
        return binary_readable_policy::template peek<_T>(readable, cursor);
    }
    
    template <typename _T, typename _Readable, typename binary_readable_policy = binary_readable_traits<_Readable>>
    inline _T peek(_Readable& readable, size_t const cursor, byte_order_t const byte_order) {
        return binary_readable_policy::template peek<_T>(readable, cursor, byte_order);
    }
    
    // extension memory_reader
    
    template <>
    struct binary_readable_traits<memory_reader> {
        template <typename _T>
        static inline _T read(memory_reader& reader) {
            _T const* value = reinterpret_cast<_T const*>(reader.ptr() + reader.cursor());
            reader.move(sizeof(_T));
            return *value;
        }
        
        template <typename _T>
        static inline _T read(memory_reader& reader, byte_order_t const byte_order) {
            if (byte_order == byte_order_t::little_endian) {
                _T value = reader.ptr()[reader.cursor() + 0];
                for (auto i = 1; i < sizeof(_T); ++i) {
                    value |= static_cast<_T>(reader.ptr()[reader.cursor() + i]) << (8 * i);
                }
                reader.move(sizeof(_T));
                return value;
            }
            else if (byte_order == byte_order_t::big_endian) {
                _T value = reader.ptr()[reader.cursor() + 0];
                for (auto i = 1; i < sizeof(_T); ++i) {
                    value <<= 8;
                    value |= reader.ptr()[reader.cursor() + i];
                }
                reader.move(sizeof(_T));
                return value;
            }
            else {
                return read<_T>(reader);
            }
        }
        
        template <typename _T>
        static inline _T peek(memory_reader& reader) {
            return peek<_T>(reader, reader.cursor());
        }
        
        template <typename _T>
        static inline _T peek(memory_reader& reader, byte_order_t const byte_order) {
            return peek<_T>(reader, reader.cursor(), byte_order);
        }
        
        template <typename _T>
        static inline _T peek(memory_reader& reader, size_t const cursor) {
            return *reinterpret_cast<_T const*>(reader.ptr() + cursor);
        }

        template <typename _T>
        static inline _T peek(memory_reader& reader, size_t const cursor, byte_order_t const byte_order) {
            if (byte_order == byte_order_t::little_endian) {
                _T value = reader.ptr()[cursor + 0];
                for (auto i = 1; i < sizeof(_T); ++i) {
                    value |= static_cast<_T>(reader.ptr()[cursor + i]) << (8 * i);
                }
                return value;
            }
            else if (byte_order == byte_order_t::big_endian) {
                _T value = reader.ptr()[cursor + 0];
                for (auto i = 1; i < sizeof(_T); ++i) {
                    value <<= 8;
                    value |= reader.ptr()[cursor + i];
                }
                return value;
            }
            else {
                return peek<_T>(reader, cursor);
            }
        }
    };
    
    // for optimization
    template <>
    inline uint8_t binary_readable_traits<memory_reader>::read(memory_reader& reader) {
        return reader.read();
    }
    
    // for optimization
    template <>
    inline uint8_t binary_readable_traits<memory_reader>::read(memory_reader& reader, byte_order_t const) {
        return reader.read();
    }
    
    // for optimization
    template <>
    inline uint8_t binary_readable_traits<memory_reader>::peek(memory_reader& reader) {
        return reader.peek();
    }
    
    // for optimization
    template <>
    inline uint8_t binary_readable_traits<memory_reader>::peek(memory_reader& reader, byte_order_t const) {
        return reader.peek();
    }
    
    // for optimization
    template <>
    inline uint8_t binary_readable_traits<memory_reader>::peek(memory_reader& reader, size_t const cursor) {
        return reader.peek(cursor);
    }
    
    // for optimization
    template <>
    inline uint8_t binary_readable_traits<memory_reader>::peek(memory_reader& reader, size_t const cursor, byte_order_t const) {
        return reader.peek(cursor);
    }
    
    // extension std::istream
    
    template <>
    struct binary_readable_traits<std::istream> {
        template <typename _T>
        static inline _T read(std::istream& is) {
            char buf[sizeof(_T)];
            is.read(buf, sizeof(_T));
            return *reinterpret_cast<_T*>(buf);
        }
        
        template <typename _T>
        static inline _T read(std::istream& is, byte_order_t const byte_order) {
            if (byte_order == byte_order_t::little_endian) {
                _T value = read<uint8_t>(is);
                for (auto i = 1; i < sizeof(_T); ++i) {
                    value |= static_cast<_T>(read<uint8_t>(is)) << (8 * i);
                }
                return value;
            }
            else if (byte_order == byte_order_t::big_endian) {
                _T value = read<uint8_t>(is);
                for (auto i = 1; i < sizeof(_T); ++i) {
                    value <<= 8;
                    value |= read<uint8_t>(is);
                }
                return value;
            }
            else {
                return read<_T>(is);
            }
        }
    };
}
