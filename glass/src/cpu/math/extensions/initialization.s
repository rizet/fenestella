
global configure_math_extensions
configure_math_extensions:
    extern enable_fpu
    call $+(enable_fpu-$)
    extern allow_sse
    call $+(allow_sse-$)
    ret
    