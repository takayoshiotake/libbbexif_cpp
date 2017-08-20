//
//  json.hpp
//
//  Created by OTAKE Takayoshi on 2017/08/20.
//  Copyright © 2017 OTAKE Takayoshi. All rights reserved.
//

#pragma once

// [C++14]

/* ```Markdown
 references:
 - http://www.json.org/
``` */

#include <string>
#include <vector>
#include <utility>
#include <sstream>
#include <iomanip>

namespace bb {
    enum class json_value_type_t {
        primitive, // string, number, true or false
        object,
        array,
    };
    
    struct json_value_t;
    using json_value_primitive_t = std::string;
    using json_value_object_t = std::vector<std::pair<std::string, json_value_t>>;
    using json_value_array_t = std::vector<json_value_t>;
    
    struct json_value_t {
        json_value_type_t type_;
        // variant:
        json_value_primitive_t primitive_;
        json_value_object_t object_;
        json_value_array_t array_;
        
        json_value_type_t type() const { return type_; }
    };
    
    inline json_value_t make_json_value(json_value_primitive_t primitive) {
        return { json_value_type_t::primitive, primitive, json_value_object_t(), json_value_array_t() };
    }
    
    inline json_value_t make_json_value(json_value_object_t object) {
        return { json_value_type_t::object, json_value_primitive_t(), object, json_value_array_t() };
    }
    
    inline json_value_t make_json_value(json_value_array_t array) {
        return { json_value_type_t::array, json_value_primitive_t(), json_value_object_t(), array };
    }
    
    
    template <class _T>
    json_value_object_t make_json(_T const&);
    
    std::string stringify(json_value_object_t const& json, int indent_level, int indent_size);
    std::string stringify(json_value_array_t const& array, int indent_level, int indent_size);
    
    std::string stringify(json_value_object_t const& json, int indent_level, int indent_size) {
        std::stringstream ss;
        ss << "{";
        {
            bool is_first = true;
            for (auto const& pair: json) {
                if (!is_first) {
                    ss << ",";
                }
                ss << std::endl;
                ss << std::setw((indent_level + 1) * indent_size) << "" << "\"" << pair.first << "\": ";
                
                auto const& value = pair.second;
                switch (value.type()) {
                    case json_value_type_t::primitive:
                        ss << value.primitive_;
                        break;
                    case json_value_type_t::array:
                        ss << stringify(value.array_, indent_level + 1, indent_size);
                        break;
                    case json_value_type_t::object:
                        ss << stringify(value.object_, indent_level + 1, indent_size);
                        break;
                }
                is_first = false;
            }
        }
        ss << std::endl;
        ss << std::setw(indent_level * indent_size) << "" << "}";
        return ss.str();
    }
    
    std::string stringify(json_value_array_t const& array, int indent_level, int indent_size) {
        std::stringstream ss;
        ss << "[";
        {
            bool is_first = true;
            for (auto const& value: array) {
                if (!is_first) {
                    ss << ",";
                }
                ss << std::endl;
                ss << std::setw((indent_level + 1) * indent_size) << "";
                switch (value.type()) {
                    case json_value_type_t::primitive:
                        ss << value.primitive_;
                        break;
                    case json_value_type_t::array:
                        ss << stringify(value.array_, indent_level + 1, indent_size);
                        break;
                    case json_value_type_t::object:
                        ss << stringify(value.object_, indent_level + 1, indent_size);
                        break;
                }
                is_first = false;
            }
        }
        ss << std::endl;
        ss << std::setw(indent_level * indent_size) << "" << "]";
        return ss.str();
    }
}
