#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <unordered_map>
#include <deque>
#include <cstring>
#include <queue>

using namespace std;

struct ZoneCount { 
    string zone; 
    long long count; 
};

struct SlotCount { 
    string zone; 
    int hour; 
    long long count; 
};

class TripAnalyzer {
public:
    void ingestStdin();
    vector<ZoneCount> topZones();
    vector<SlotCount> topBusySlots();

private:
    struct ZoneStats {
        long long total;
        long long byHour[24];
        ZoneStats() : total(0) { 
            memset(byHour, 0, sizeof(byHour)); 
        }
    };

    unordered_map<string, ZoneStats> zones;
    
    static inline bool isSpace(unsigned char c) { 
        return c <= 32; 
    }
    
    static inline void trimSpan(const string& s, size_t& b, size_t& e) {
        while (b < e && isSpace((unsigned char)s[b])) ++b;
        while (e > b && isSpace((unsigned char)s[e - 1])) --e;
        if (e > b + 1 && s[b] == '"' && s[e - 1] == '"') { 
            ++b; --e; 
        }
        while (b < e && isSpace((unsigned char)s[b])) ++b;
        while (e > b && isSpace((unsigned char)s[e - 1])) --e;
    }
    
    static inline void stripBOM(string& line) {
        if (line.size() >= 3 &&
            (unsigned char)line[0] == 0xEF &&
            (unsigned char)line[1] == 0xBB &&
            (unsigned char)line[2] == 0xBF) {
            line.erase(0, 3);
        }
    }
    
    static inline bool splitFirst6Csv(const string& s, size_t b[6], size_t e[6]) {
        bool inQ = false;
        int col = 0;
        size_t start = 0;
        const size_t n = s.size();
        
        for (size_t i = 0; i <= n; ++i) {
            if (i < n && s[i] == '"') inQ = !inQ;
            if (!inQ && (i == n || s[i] == ',')) {
                if (col < 6) { 
                    b[col] = start; 
                    e[col] = i; 
                }
                ++col;
                start = i + 1;
                if (col >= 6) return true;
            }
        }
        return false;
    }
    
    static inline bool fastParseHour(const string& s, size_t b, size_t e, int& hourOut) {
        while (b < e && isSpace((unsigned char)s[b])) ++b;
        while (e > b && isSpace((unsigned char)s[e - 1])) --e;
        if (e > b + 1 && s[b] == '"' && s[e - 1] == '"') { 
            ++b; --e; 
        }
        while (b < e && isSpace((unsigned char)s[b])) ++b;
        while (e > b && isSpace((unsigned char)s[e - 1])) --e;
        
        if (e - b < 13) return false;
        
        size_t spacePos = b;
        bool foundSpace = false;
        for(; spacePos < e; ++spacePos) {
            if (s[spacePos] == ' ') {
                foundSpace = true;
                break;
            }
        }
        
        if (!foundSpace || spacePos + 3 > e) return false;
        
        char h1 = s[spacePos + 1];
        char h2 = s[spacePos + 2];
        
        if (!isdigit(h1) || !isdigit(h2)) return false;
        int h = (h1 - '0') * 10 + (h2 - '0');
        if ((unsigned)h > 23u) return false;
        hourOut = h;
        return true;
    }
    
    static inline bool betterZone(const ZoneCount& a, const ZoneCount& b) {
        if (a.count != b.count) return a.count > b.count;
        return a.zone < b.zone;
    }
    
    static inline bool betterSlot(const SlotCount& a, const SlotCount& b) {
        if (a.count != b.count) return a.count > b.count;
        if (a.zone != b.zone) return a.zone < b.zone;
        return a.hour < b.hour;
    }
    
    // Optimized top-K using min-heap (faster than partial_sort for large K)
    template<typename T, typename Compare>
    static void topKHeap(vector<T>& vec, int k, Compare comp) {
        if (vec.size() <= static_cast<size_t>(k)) {
            sort(vec.begin(), vec.end(), comp);
            return;
        }
        
        // Use max-heap to keep top K elements
        priority_queue<T, vector<T>, Compare> heap(comp);
        
        for (size_t i = 0; i < vec.size(); ++i) {
            if (heap.size() < static_cast<size_t>(k)) {
                heap.push(vec[i]);
            } else if (comp(vec[i], heap.top())) {
                heap.pop();
                heap.push(vec[i]);
            }
        }
        
        vec.clear();
        vec.reserve(heap.size());
        while (!heap.empty()) {
            vec.push_back(heap.top());
            heap.pop();
        }
        reverse(vec.begin(), vec.end());
    }
};

void TripAnalyzer::ingestStdin() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    
    zones.clear();
    zones.reserve(200000);
    zones.max_load_factor(0.7f);
    
    string line;
    line.reserve(512);
    size_t b[6], e[6];
    
    while (getline(cin, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        
        stripBOM(line);
        if (line.empty()) continue;
        
        // Skip header line
        if (line.find("TripID") != string::npos && line.find("PickupZoneID") != string::npos) {
            continue;
        }
        
        if (!splitFirst6Csv(line, b, e)) continue;
        
        size_t zoneB = b[1], zoneE = e[1];
        trimSpan(line, zoneB, zoneE);
        if (zoneB >= zoneE) continue;
        
        size_t dtB = b[3], dtE = e[3];
        int hour = -1;
        if (!fastParseHour(line, dtB, dtE, hour)) continue;
        
        string zone(line, zoneB, zoneE - zoneB);
        
        ZoneStats& stats = zones[zone];
        ++stats.total;
        ++stats.byHour[hour];
    }
}

vector<ZoneCount> TripAnalyzer::topZones() {
    vector<ZoneCount> out;
    if (zones.empty()) return out;
    
    out.reserve(zones.size());
    for (const auto& kv : zones) {
        out.push_back({ kv.first, kv.second.total });
    }
    
    // Use partial_sort for top-10 (faster than full sort)
    const int k = 10;
    if (out.size() > static_cast<size_t>(k)) {
        partial_sort(out.begin(), out.begin() + k, out.end(), betterZone);
        out.resize(k);
    } else {
        sort(out.begin(), out.end(), betterZone);
    }
    
    return out;
}

vector<SlotCount> TripAnalyzer::topBusySlots() {
    vector<SlotCount> out;
    if (zones.empty()) return out;
    
    // Pre-allocate with reasonable estimate
    out.reserve(min(zones.size() * 24, static_cast<size_t>(100000)));
    
    for (const auto& kv : zones) {
        const ZoneStats& st = kv.second;
        for (int h = 0; h < 24; ++h) {
            long long c = st.byHour[h];
            if (c > 0) {
                out.push_back({ kv.first, h, c });
            }
        }
    }
    
    // Use partial_sort for top-10
    const int k = 10;
    if (out.size() > static_cast<size_t>(k)) {
        partial_sort(out.begin(), out.begin() + k, out.end(), betterSlot);
        out.resize(k);
    } else {
        sort(out.begin(), out.end(), betterSlot);
    }
    
    return out;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    
    TripAnalyzer analyzer;
    analyzer.ingestStdin();
    
    cout << "TOP_ZONES\n";
    vector<ZoneCount> top_zones = analyzer.topZones();
    for (size_t i = 0; i < top_zones.size(); i++) {
        cout << top_zones[i].zone << "," << top_zones[i].count << "\n";
    }
    
    cout << "TOP_SLOTS\n";
    vector<SlotCount> top_slots = analyzer.topBusySlots();
    for (size_t i = 0; i < top_slots.size(); i++) {
        cout << top_slots[i].zone << "," << top_slots[i].hour << "," << top_slots[i].count << "\n";
    }
    
    return 0;
}

