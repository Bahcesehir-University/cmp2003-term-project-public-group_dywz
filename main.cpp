#include "analyzer.h"
#include <iostream>
#include <chrono>

using namespace std;
using namespace std::chrono;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    
    auto start = high_resolution_clock::now();
    
    TripAnalyzer analyzer;
    analyzer.ingestFile("SmallTrips.csv");
    
    auto ingestEnd = high_resolution_clock::now();
    
    vector<ZoneCount> top_zones = analyzer.topZones();
    vector<SlotCount> top_slots = analyzer.topBusySlots();
    
    auto end = high_resolution_clock::now();
    
    cout << "TOP_ZONES\n";
    for (const auto& z : top_zones) {
        cout << z.zone << "," << z.count << "\n";
    }
    
    cout << "TOP_SLOTS\n";
    for (const auto& s : top_slots) {
        cout << s.zone << "," << s.hour << "," << s.count << "\n";
    }
    
    auto totalTime = duration_cast<milliseconds>(end - start).count();
    auto ingestTime = duration_cast<milliseconds>(ingestEnd - start).count();
    
    cerr << "Ingest time: " << ingestTime << " ms\n";
    cerr << "Total time: " << totalTime << " ms\n";
    
    return 0;
}

