thread_local variable except function scope that require dynamic initialization or non-static data member with initializer are using TLS callback for call constructor on VC++14.
gcc and Clang are using lazy initialization.
By above reasons, there is a case in which the constructor was not called.

This behavior has changed in MSVC 16.5.  
https://developercommunity.visualstudio.com/content/problem/124121/thread-local-variables-fail-to-be-initialized-when.html

License: CC BY-NC-SA
