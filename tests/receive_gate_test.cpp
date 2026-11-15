#include "core/receive_gate.hpp"
#include <cassert>
int main() {
    input_trace::ReceiveGate gate;
    assert(gate.begin(1,false,false) && !gate.accept());
    gate.drained(); assert(gate.accept());
    assert(!gate.begin(1.1,false,false));
    assert(gate.begin(3,false,false) && !gate.accept()); // suspended render
    assert(gate.begin(3.1,false,false)); // queue still not drained
    gate.drained(); assert(!gate.begin(3.2,false,false));
    gate.exhausted(); assert(!gate.accept()); // receive budget hit
    gate.drained(); assert(gate.begin(3.3,true,false)); // session reset
    gate.drained(); assert(gate.begin(3.4,false,true));
    gate.drained(); assert(gate.begin(3.5,false,false)); // resume barrier
    gate.drained(); assert(!gate.begin(3.6,false,false));
}
