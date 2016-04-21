FLS callbacks better than DllMain/TLS callbacks ?

####Advantage
* Callback to all elements when freed.

####Disadvantage
* Unregister is not synchronized with unloading DLL. [There is one I heard.](https://support.microsoft.com/en-us/kb/2754614)

Sample code may cause race condition.

License: CC BY-NC-SA
