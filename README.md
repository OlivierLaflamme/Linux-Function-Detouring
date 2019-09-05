# Linux-Function-Detouring

## Ghetto mapping.cpp
Another approach for detouring functions. Instead of directly push a jmp into the function, I search for all callers that contains the address of a wanted function in the sections. This way is ghetto and is for windows but it works. In a certain amount, for example if your function is a (pure) virtual function it won't be able detour/hook the function. Mapped modules aswell could call the original function and this won't be able to catch it, but it's interesting if you're searching to do it in more in a static way I guess. I could add more like, we could scan all the process with all mapped virtual memory and see wich one contains our caller.
