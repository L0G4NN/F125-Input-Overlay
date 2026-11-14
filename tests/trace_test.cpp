#include "core/trace.hpp"
#include <cstdlib>
#include <iostream>
#include <limits>
using namespace input_trace;
void check(bool pass, const char* message) {
    if (!pass) { std::cerr << message << '\n'; std::exit(1); }
}
bool near(double a, double b) { return std::abs(a-b) < 0.0001; }
int main() {
    Trace<> trace;
    check(trace.latest() == nullptr, "empty trace must not show zero input");
    check(trace.append(0, 0, 1), "first sample");
    check(!trace[0].connected, "first point has no preceding segment");
    check(trace.append(.1, 1, 1), "overlapping inputs");
    check(trace.latest()->throttle == 1 && trace.latest()->brake == 1, "channels must be independent");
    check(!trace.append(.1, 0, 0) && trace.size() == 2, "duplicate timestamp");
    auto p = trace.point(*trace.latest(), true, 400, 100);
    check(near(p.x,400) && near(p.y,0), "full brake at top right");
    trace.pause();
    check(!trace.append(.2, 0, 0) && trace.time() == .1, "pause freezes history");
    trace.resume();
    trace.append(.2, 0, 0);
    check(!trace.latest()->connected, "resume breaks segment");
    trace.unavailable();
    check(!trace.latest(), "missing source has no live percentage");
    trace.append(.3, 0, 0);
    check(!trace.latest()->connected, "recovery cannot bridge source gap");
    trace.append(1, 1, 0);
    check(!trace.latest()->connected, "long interval cannot bridge gap");
    check(!trace.append(1.1, std::numeric_limits<float>::quiet_NaN(), 0), "NaN rejected");
    check(!trace.append(1.1, 0, 1.1f), "out of range rejected");
    check(!trace.append(std::numeric_limits<double>::infinity(), 0, 0), "bad time rejected");
    trace.append(.05, 1, 0);
    check(trace.size() == 1 && !trace[0].connected, "rewind clears old samples");
    Trace<3> small;
    for (int i=0;i<10;++i) small.append(i*.1, 1, 0);
    check(small.size() == 3 && near(small[0].time,.7), "capacity wraps in order");
    check(!small[0].connected && small[1].connected, "eviction preserves segment boundaries");
    small.append(10, 0, 0);
    check(small.size() == 1, "history expires by age");
    small.set_history(-1); check(small.history() == 2, "minimum history");
    small.set_history(100); check(small.history() == 15, "maximum history");
    small.set_history(std::numeric_limits<double>::quiet_NaN());
    check(small.history() == 5, "invalid history defaults");
    for (int fps : {30,60,144}) {
        Trace<> t;
        for (int i=0;i<=fps*8;++i) {
            double time = double(i)/fps;
            float brake = time < 1 ? 0.f : time < 3 ? float((3-time)/2) : 0.f;
            t.append(time, 1-brake, brake);
        }
        check(near(t.time(),8) && t[0].time >= 3, "history is time based at all frame rates");
        auto last=t.point(*t.latest(),true,400,100);
        check(near(last.x,400) && near(last.y,100), "released brake at bottom right");
        for (std::size_t i=0;i<t.size();++i) {
            auto point=t.point(t[i],true,400,100);
            check(point.x>=0 && point.x<=400 && point.y>=0 && point.y<=100, "points stay inside graph");
        }
    }
    std::cout << "Trace core checks passed\n";
}
