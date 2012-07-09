TO USE OpalDotNET:

Simply reference the OpalDotNet.dll or the project inside your .NET project. You must compile the opal and ptlib
dlls yourself, and put them in the executing directory of your application using OpalDotNet.dll.

Also, it is not necessary to use the Opal_API structures directly. The OpalContext.cs file contains the wrappers and context for your to use.
These classes will handle marshaling the data back and forth between managed and unmanaged types.

Happy Coding!