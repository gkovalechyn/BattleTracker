# Battle Tracker
Old abandoned project to track what happened duriong an Arma 3 OP to provide a better AAR.  

Currently only has one thing that kind of works that is the ExtensionProxy that allows hot-swapping of Arma 3 extensions to facilitate development of said extensions.

There is only 1 bug with it, when Dll's are unloaded it takes a little bit for the Handle to become invalid and the dll file to become valid to be swapped written to again. So right now it crashes after the second time the DLL is loaded.  
If there is a way to handle that without using a busy loop I will continue this project.