//
//  scope_exit.hpp
//
//  Created by OTAKE Takayoshi on 2017/08/18.
//  Copyright © 2017 OTAKE Takayoshi. All rights reserved.
//

#pragma once

// [C++14]

namespace bb {
    template <typename _F>
    struct scope_exit {
        _F f_;
        explicit scope_exit(_F f) : f_(f) {}
        ~scope_exit() { f_(); }
    };
    
    template <typename _F>
    inline scope_exit<_F> make_scope_exit(_F f) {
        return scope_exit<_F>(f);
    }
}
