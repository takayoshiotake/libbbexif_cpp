//
//  bbexif.hpp
//
//  Created by OTAKE Takayoshi on 2017/08/18.
//  Copyright © 2017 OTAKE Takayoshi. All rights reserved.
//

#pragma once

// [C++14]

/* ```Markdown
 references:
 - http://www.cipa.jp/std/documents/j/DC-008-2016-J.pdf
 - http://www.cipa.jp/std/documents/e/DC-008-Translation-2016-E.pdf
 - https://www.media.mit.edu/pia/Research/deepview/exif.html
 - https://sno.phy.queensu.ca/~phil/exiftool/TagNames/EXIF.html
``` */

#include <type_traits>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>
#include <map>
#include <iostream>
#include <sstream>
#include <fstream>

#include "bb/scope_exit.hpp"
#include "bb/binary_reader.hpp"
#include "bb/json.hpp"
#include "bb/debug.hpp"

namespace bbexif {
    // usage: e.g. template<typename _T, enable_if_type<_T, syd::is_pointer> = nullptr>
    template <typename _T, template <typename> class type_check>
    using enable_if_type = typename std::enable_if<type_check<_T>::value, std::nullptr_t>::type;
    
    struct jfif_segment_header_t {
        uint16_t marker_code;
        uint16_t data_length;
    };
    
    using ifd_tag_id_t = uint16_t;
    
    enum class ifd_tag_type_t {
        byte,
        ascii,
        short_,
        long_,
        rational,
        undefined,
        slong,
        srational,
    };
    
    // ifd_tag_type
    using ifd_tag_type_byte_t = uint8_t;
    using ifd_tag_type_ascii_t = char;
    using ifd_tag_type_short_t = uint16_t;
    using ifd_tag_type_long_t = uint32_t;
    struct ifd_tag_type_rational_t {
        // n/d
        ifd_tag_type_long_t n;
        ifd_tag_type_long_t d;
    };
    using ifd_tag_type_undefined_t = uint8_t;
    using ifd_tag_type_slong_t = int32_t;
    struct ifd_tag_type_srational_t {
        // n/d
        ifd_tag_type_slong_t n;
        ifd_tag_type_slong_t d;
    };
    
    struct ifd_tag_t {
        ifd_tag_id_t id_;
        uint16_t type_;
        uint32_t count_;
        uint32_t value_or_offset_;
        
        inline ifd_tag_id_t id() const { return id_; }
        inline uint16_t type() const { return type_; }
        inline uint32_t count() const { return count_; }
        inline uint32_t value() const { return value_or_offset_; }
        inline uint32_t offset() const { return value_or_offset_; }
    };
    
    struct ifd_value_t {
        ifd_tag_type_t type_;
        size_t value_count_;
        std::vector<char> data_;
        
        inline ifd_tag_type_t type() const { return type_; }
        inline size_t value_count() const { return value_count_; }
        inline std::vector<char> const& data() const { return data_; }
        template <typename _T, enable_if_type<_T, std::is_pointer> = nullptr>
        inline _T value_ptr() const { return reinterpret_cast<_T>(data_.data()); }
        template <typename _T, enable_if_type<_T, std::is_pointer> = nullptr>
        inline _T value_ptr() { return reinterpret_cast<_T>(data_.data()); }
    };
    
    using ifd_t = std::map<ifd_tag_id_t, ifd_value_t>;
    
    struct exif_t {
        std::vector<ifd_t> ifds;
        ifd_t exif;
        ifd_t gps;
        std::vector<char> thumbnail;
    };
}

// extension bb::binary_reader
namespace bb {
    template <typename _Readable>
    inline bbexif::jfif_segment_header_t read_jfif_segment_header(_Readable& r) {
        auto marker_code = read<uint16_t>(r, byte_order_t::big_endian);
        if (marker_code == 0xFFD8 || marker_code == 0xFFD9) {
            return {marker_code, 0};
        }
        else if (marker_code && 0xFF00 != 0xFF00) {
            throw std::runtime_error(bb_trace_message("JFIF marker not found"));
        }
        auto data_length = static_cast<uint16_t>(read<uint16_t>(r, byte_order_t::big_endian) - 2);
        return {marker_code, data_length};
    }
    
    template <>
    inline bbexif::jfif_segment_header_t read(std::istream& is) {
        return read_jfif_segment_header(is);
    }
    
    template <>
    inline bbexif::ifd_tag_t read(memory_reader& mr, byte_order_t const bo) {
        auto id = read<uint16_t>(mr, bo);
        auto type = read<uint16_t>(mr, bo);
        auto count = read<uint32_t>(mr, bo);
        auto value_or_offset = read<uint32_t>(mr, bo);
        return {id, type, count, value_or_offset};
    }
    
    template <>
    inline bbexif::ifd_t read(memory_reader& mr, byte_order_t const bo) {
        using namespace bbexif;
        ifd_t ifd;
        auto number_of_ifd_tags = bb::read<uint16_t>(mr, bo);
        if (mr.available() < number_of_ifd_tags * sizeof(ifd_tag_t) + 4) {
            throw std::runtime_error(bb_trace_message("Unable to read Exif"));
        }
        for (auto ti = 0; ti < number_of_ifd_tags; ++ti) {
            static std::map<uint16_t, ifd_tag_type_t> const ifd_tag_type_map = {
                { 1, ifd_tag_type_t::byte },
                { 2, ifd_tag_type_t::ascii },
                { 3, ifd_tag_type_t::short_ },
                { 4, ifd_tag_type_t::long_ },
                { 5, ifd_tag_type_t::rational },
                { 7, ifd_tag_type_t::undefined },
                { 9, ifd_tag_type_t::slong },
                { 10, ifd_tag_type_t::srational },
            };
            static std::map<ifd_tag_type_t, size_t> const type_size_map = {
                { ifd_tag_type_t::byte, sizeof(ifd_tag_type_byte_t) },
                { ifd_tag_type_t::ascii, sizeof(ifd_tag_type_ascii_t) },
                { ifd_tag_type_t::short_, sizeof(ifd_tag_type_short_t) },
                { ifd_tag_type_t::long_, sizeof(ifd_tag_type_long_t) },
                { ifd_tag_type_t::rational, sizeof(ifd_tag_type_rational_t) },
                { ifd_tag_type_t::undefined, sizeof(ifd_tag_type_undefined_t) },
                { ifd_tag_type_t::slong, sizeof(ifd_tag_type_slong_t) },
                { ifd_tag_type_t::srational, sizeof(ifd_tag_type_srational_t) },
            };
            
            auto ifd_tag = bb::read<ifd_tag_t>(mr, bo);
            ifd_tag_type_t tag_type;
            try {
                tag_type = ifd_tag_type_map.at(ifd_tag.type());
            }
            catch (std::out_of_range const& e) {
                std::cout << bb::make_log_message("[Warning] Skipped reading the not supported type IFD tag: id=0x%04X, type=%d", ifd_tag.id(), ifd_tag.type()) << std::endl;
                continue;
            }
            
            size_t type_size = type_size_map.at(tag_type);
            std::vector<char> tag_data(ifd_tag.count() * type_size);
            if (tag_data.size() <= 4) {
                // ifd_tag.value_or_offset_ is value(s)
                // HACK: ifd_tag.value() is not effective because that is read as uint32_t with specified byte order
                auto offset = mr.cursor() - 4;
                if (type_size == 1) {
                    mr.peek(offset, reinterpret_cast<uint8_t*>(tag_data.data()), tag_data.size());
                }
                else if (type_size == 2 || type_size == 4) {
                    for (auto vi = 0; vi < ifd_tag.count(); ++vi) {
                        switch (type_size) {
                            case 2:
                                *reinterpret_cast<uint16_t*>(tag_data.data() + type_size * vi) = bb::peek<uint16_t>(mr, offset + type_size * vi, bo);
                                break;
                            case 4:
                                *reinterpret_cast<uint32_t*>(tag_data.data() + type_size * vi) = bb::peek<uint32_t>(mr, offset + type_size * vi, bo);
                                break;
                        }
                    }
                }
                else {
                    std::cout << bb::make_log_message("[Warning] Skipped reading the not supported type IFD tag: id=0x%04X, type=%d", ifd_tag.id(), ifd_tag.type()) << std::endl;
                    continue;
                }
            }
            else {
                // ifd_tag.value_or_offset_ is offset to value(s)
                auto offset = ifd_tag.offset();
                if (mr.available(offset) < tag_data.size()) {
                    std::cout << bb::make_log_message("[Warning] Skipped reading the not supported type IFD tag: id=0x%04X, type=%d", ifd_tag.id(), ifd_tag.type()) << std::endl;
                    continue;
                }
                if (type_size == 1) {
                    mr.peek(offset, reinterpret_cast<uint8_t*>(tag_data.data()), tag_data.size());
                }
                else if (type_size == 2 || type_size == 4 || type_size == 8) {
                    for (auto vi = 0; vi < ifd_tag.count(); ++vi) {
                        switch (type_size) {
                            case 2:
                                *reinterpret_cast<uint16_t*>(tag_data.data() + type_size * vi) = bb::peek<uint16_t>(mr, offset + type_size * vi, bo);
                                break;
                            case 4:
                                *reinterpret_cast<uint32_t*>(tag_data.data() + type_size * vi) = bb::peek<uint32_t>(mr, offset + type_size * vi, bo);
                                break;
                            case 8:
                                *reinterpret_cast<uint32_t*>(tag_data.data() + type_size * vi + 0) = bb::peek<uint32_t>(mr, offset + type_size * vi + 0, bo);
                                *reinterpret_cast<uint32_t*>(tag_data.data() + type_size * vi + 4) = bb::peek<uint32_t>(mr, offset + type_size * vi + 4, bo);
                                break;
                        }
                    }
                }
                else {
                    std::cout << bb::make_log_message("[Warning] Skipped reading the not supported type IFD tag: id=0x%04X, type=%d", ifd_tag.id(), ifd_tag.type()) << std::endl;
                    continue;
                }
            }
            
#ifdef DEBUG
            if (0) {
                std::stringstream ss;
                ss << std::setfill('0');
                ss << std::hex << std::setw(4) << ifd_tag.id() << ": ";
                ss << std::dec << static_cast<uint16_t>(tag_type) << " { ";
                ss << std::hex;
                for (auto const& d: tag_data) {
                    ss << std::setw(2) << (+d < 0 ? d + 256 : +d) << " ";
                }
                ss << "} (" << std::dec << tag_data.size() << ")";
                std::cout << "[D] " << ss.str() << std::endl;
            }
#endif
            ifd[ifd_tag.id()] = {tag_type, ifd_tag.count(), std::move(tag_data)};
        }
        
        return ifd;
    }
}

namespace bbexif {
    exif_t read_exif(std::string const& filepath);
    exif_t read_exif(std::istream& is);
    exif_t read_exif_from_app1_segment(char const* ptr, size_t const size);
    
    exif_t read_exif(std::string const& filepath) {
        std::ifstream ifs;
        ifs.open(filepath);
        if (!ifs.is_open()) {
            throw std::runtime_error(bb_trace_message("Unable to open the file: %s", filepath.c_str()));
        }
        return read_exif(ifs);
    }
    
    exif_t read_exif(std::istream& is) {
        auto const iostatus = is.exceptions();
        auto revert_exceptions = bb::make_scope_exit([&is, &iostatus]() {
            is.exceptions(iostatus);
        });
        is.exceptions(std::istream::eofbit);
        
        std::vector<char> app1_segment_data;
        try {
            if (bb::read<jfif_segment_header_t>(is).marker_code != 0xFFD8) {
                throw std::exception();
            }
            auto jfif_segment = bb::read<jfif_segment_header_t>(is);
            if (jfif_segment.marker_code != 0xFFE1) {
                // This is not SOI segment
                throw std::runtime_error(bb_trace_message("APP1 segment not found"));
            }
            // APP1 segment must be recorded immediately after SOI
            app1_segment_data.resize(jfif_segment.data_length);
            is.read(reinterpret_cast<char*>(app1_segment_data.data()), jfif_segment.data_length);
        }
        catch (std::exception const&) {
            throw std::runtime_error(bb_trace_message("Unable to read a exif"));
        }
        return read_exif_from_app1_segment(app1_segment_data.data(), app1_segment_data.size());
    }
    
    exif_t read_exif_from_app1_segment(char const* ptr, size_t const size) {
        auto mr = bb::memory_reader(reinterpret_cast<uint8_t const*>(ptr), size);
        
        if (mr.available() < 6 + 2 + 2 + 4) {
            throw std::runtime_error(bb_trace_message("Exif not found"));
        }
        {
            uint8_t exif_id_code[6];
            mr.read(exif_id_code, sizeof(exif_id_code));
            if (::memcmp(exif_id_code, "Exif\0\0", sizeof(exif_id_code))) {
                throw std::runtime_error(bb_trace_message("Exif not found"));
            }
        }
        // Exif identifier header
        mr.reset(mr.ptr() + mr.cursor(), mr.available());
        // TIFF header
        bb::byte_order_t const bo = [](uint16_t byte_order) {
            switch (byte_order) {
            case 0x4949: // "II"
                return bb::byte_order_t::little_endian;
            case 0x4A4A: // "MM"
                return bb::byte_order_t::big_endian;
            default:
                throw std::runtime_error(bb_trace_message("Exif not found"));
            }
        }(bb::read<uint16_t>(mr));
        if (bb::read<uint16_t>(mr, bo) != 0x002A) {
            throw std::runtime_error(bb_trace_message("Unsupported Exif version"));
        }
        
        std::vector<ifd_t> ifds;
        // IFD (loop)
        for (;;) {
            auto next_ifd_offset = bb::read<uint32_t>(mr, bo);
            if (next_ifd_offset == 0) {
                // This means there is no linked IFD
                break;
            }
            if (next_ifd_offset < mr.cursor()) {
                throw std::runtime_error(bb_trace_message("Unable to read Exif"));
            }
            if (mr.available(next_ifd_offset) < 4 + 0 + 4) {
                throw std::runtime_error(bb_trace_message("Unable to read Exif"));
            }
            mr.move_to(next_ifd_offset);
            
            auto ifd = bb::read<ifd_t>(mr, bo);
            ifds.push_back(std::move(ifd));
        }
        
        ifd_t exif;
        ifd_t gps;
        if (ifds.size() >= 1) {
            static ifd_tag_id_t const exif_ifd_tag_id = 0x8769;
            static ifd_tag_id_t const gps_ifd_tag_id = 0x8852;
            auto& ifd = ifds[0];
            if (ifd.count(exif_ifd_tag_id)) {
                auto& sub_ifd_tag = ifd.at(exif_ifd_tag_id);
                if (sub_ifd_tag.type() == ifd_tag_type_t::long_) {
                    auto offset = *sub_ifd_tag.value_ptr<uint32_t const*>();
                    bb::memory_reader sub_ifd_mr(mr.ptr() + offset, mr.available(offset));
                    exif = bb::read<ifd_t>(sub_ifd_mr, bo);
                }
            }
            if (ifd.count(gps_ifd_tag_id)) {
                auto& sub_ifd_tag = ifd.at(gps_ifd_tag_id);
                if (sub_ifd_tag.type() == ifd_tag_type_t::long_) {
                    auto offset = *sub_ifd_tag.value_ptr<uint32_t const*>();
                    bb::memory_reader sub_ifd_mr(mr.ptr() + offset, mr.available(offset));
                    gps = bb::read<ifd_t>(sub_ifd_mr, bo);
                }
            }
        }
        
        std::vector<char> thumbnail;
        if (ifds.size() >= 2) {
            static ifd_tag_id_t const thumbnail_offset_tag_id = 0x0201; // known as JPEGInterchangeFormat
            static ifd_tag_id_t const thumbnail_length_tag_id = 0x0202; // known as JPEGInterchangeFormatLength
            auto& ifd = ifds[1];
            if (ifd.count(thumbnail_offset_tag_id) && ifd.count(thumbnail_length_tag_id)) {
                auto& thumbnail_offset_tag = ifd.at(thumbnail_offset_tag_id);
                auto& thumbnail_length_tag = ifd.at(thumbnail_length_tag_id);
                if (thumbnail_offset_tag.type() == ifd_tag_type_t::long_ && thumbnail_length_tag.type() == ifd_tag_type_t::long_) {
                    auto offset = *thumbnail_offset_tag.value_ptr<uint32_t const*>();
                    auto length = *thumbnail_length_tag.value_ptr<uint32_t const*>();
                    if (mr.available(offset) >= length) {
                        thumbnail.resize(length);
                        mr.peek(offset, reinterpret_cast<uint8_t*>(thumbnail.data()), thumbnail.size());
                    }
                }
            }
        }
        
        return {ifds, exif, gps, thumbnail};
    }
}

// extension bb::json
namespace bb {
    template <>
    json_value_object_t make_json(bbexif::ifd_value_t const& value) {
        using namespace bbexif;
        json_value_object_t json;
        // TODO: improve
        {
            {
                static std::map<ifd_tag_type_t, uint16_t> const ifd_tag_type_map = {
                    { ifd_tag_type_t::byte, 1 },
                    { ifd_tag_type_t::ascii, 2 },
                    { ifd_tag_type_t::short_, 3 },
                    { ifd_tag_type_t::long_, 4 },
                    { ifd_tag_type_t::rational, 5 },
                    { ifd_tag_type_t::undefined, 7 },
                    { ifd_tag_type_t::slong, 9 },
                    { ifd_tag_type_t::srational, 10 },
                };
                
                std::stringstream ss;
                ss << ifd_tag_type_map.at(value.type());
                json.push_back({"type", bb::make_json_value(ss.str())});
            }
            {
                std::stringstream ss;
                ss << std::setfill('0') << std::hex;
                ss << "\"";
                {
                    bool is_first = true;
                    for (auto const& d: value.data()) {
                        if (!is_first) {
                            ss << ",";
                        }
                        ss << std::setw(2) << (+d < 0 ? d + 256 : +d);
                        is_first = false;
                    }
                }
                ss << "\"";
                json.push_back({"data", bb::make_json_value(ss.str())});
            }
        }
        return json;
    }
    
    template <>
    json_value_object_t make_json(bbexif::ifd_t const& ifd) {
        json_value_object_t json;
        for (auto const& value: ifd) {
            std::stringstream ss;
            ss << std::setfill('0') << std::hex << std::setw(4) << value.first;
            json.push_back({ss.str(), bb::make_json_value(bb::make_json(value.second))});
        }
        return json;
    }
    
    template <>
    json_value_object_t make_json(bbexif::exif_t const& exif) {
        json_value_object_t json;

        json_value_array_t ifds;
        for (auto const& ifd: exif.ifds) {
            ifds.push_back(bb::make_json_value(bb::make_json(ifd)));
        }
        json.push_back({"ifd", bb::make_json_value(ifds)});
        json.push_back({"exif", bb::make_json_value(bb::make_json(exif.exif))});
        json.push_back({"gps", bb::make_json_value(bb::make_json(exif.gps))});
        {
            std::stringstream ss;
            ss << std::setfill('0') << std::hex;
            ss << "\"";
            {
                bool is_first = true;
                for (auto const& d: exif.thumbnail) {
                    if (!is_first) {
                        ss << ",";
                    }
                    ss << std::setw(2) << (+d < 0 ? d + 256 : +d);
                    is_first = false;
                }
            }
            ss << "\"";
            json.push_back({"thumbnail", bb::make_json_value(ss.str())});
        }
        
        return json;
    }
}
