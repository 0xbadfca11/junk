thread_local variable except function scope that require dynamic initialization and non-static data member with initializer are using TLS callback for call constructor on VC++14.
gcc and Clang are using lazy initialization.
By above reasons, there is a case in which the constructor was not called.

License: CC BY-NC-SA
