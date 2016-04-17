FLS callbacks better than DllMain/TLS callbacks ?

####Advantage
* Callback to all elements when freed.

####Disadvantage
* Unregister is not synchronized with unloading DLL.

Sample code may cause race condition.

License: CC BY-NC-SA
