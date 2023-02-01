# Trapit

_Trapit_ is a command line utility which traps _short-lived processes_
(e.g. `curl`) with _inspectors_ (e.g. `strace`).

For example:

```sh
# In terminal A: the process holds off running until a signal is received
trapit exec -- curl https://httpbin.org/get

# In terminal B: the PID will be returned and the target process will wake up
strace -p "$(trapit wake)"
```
