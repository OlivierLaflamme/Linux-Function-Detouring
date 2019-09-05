# Linux-Function-Detouring

## mapper.cpp
Here a simple linux detouring class. It should work for x64/x86 architecture.

Basically, what it's doing is just replacing the function/address you want to detour with a jmp instruction. If you call the original function, it will reset the original bytes and restore the detour.

Most of detours I saw are using 0xE9 for a jmp (I don't even know why), but it can't use a relative address based on 64 bits, because there is a limitation of 2GB (32bits) of address space with this jmp.

You need to set the value of RAX/EAX of your new function that will be detoured, and then write a jmp with EAX/RAX depending on 32/64 bits accordignly.

For converting into windows, it should be pretty easy too aswell.

This might be a bad example, as some functions on a debug build could make like 5 bytes wich is actually not enough for some detours because the program could call an instruction in another thread overwritten by the jmp routine and result in a crash.

## ghettomapper.cpp
Another approach for detouring functions. Instead of directly push a jmp into the function, I search for all callers that contains the address of a wanted function in the sections. This way is ghetto and is for windows but it works. In a certain amount, for example if your function is a (pure) virtual function it won't be able detour/hook the function. Mapped modules aswell could call the original function and this won't be able to catch it, but it's interesting if you're searching to do it in more in a static way I guess. I could add more like, we could scan all the process with all mapped virtual memory and see wich one contains our caller.
