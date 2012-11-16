#pragma once

#define STATICASSERT(x) static_assert((x), #x)

//#define TESTING
#define EXCEPTION_ASSERT

#ifdef TESTING
    #ifdef EXCEPTION_ASSERT
        #include <stdexcept>

        #define STR_VALUE_OF_MACRO(arg)      #arg                       //Arg won't be expanded if it is a macro
        #define STR_VALUE_OF_MACRO2(arg)     STR_VALUE_OF_MACRO(arg)    //So we need another call

        #define ASSERT(x) if (!(x)) {throw std::logic_error( \
            "Assertion failure in " __FILE__ " at line " STR_VALUE_OF_MACRO2(__LINE__) ": " #x "\n");}
    #else
        #include <cassert>
        #define ASSERT(x) assert(x)
    #endif

    #define PRINT(x) std::cout << #x ": " << (x) << '\n'
#else
    #define PRINT(x)
    #define ASSERT(x)
#endif
