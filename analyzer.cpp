#include "analyzer.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <unordered_map>
#include <cstring>
#include <cctype>

using namespace std;

bool TripAnalyzer::isSpace(unsigned char c) {
    return c <= 32;
}

void TripAnalyzer::trimSpan(const string& s, size_t& b, size_t& e) {
    while (b < e && isSpace((unsigned char)s[b])) ++b;
    while (e > b && isSpace((unsigned char)s[e - 1])) --e;
    if (e > b + 1 && s[b] == '"' && s[e - 1] == '"') {
        ++b; --e;
    }
    while (b < e && isSpace((unsigned char)s[b])) ++b;
    while (e > b && isSpace((unsigned char)s[e - 1])) --e;
}

void TripAnalyzer::stripBOM(string& line) {
    if (line.size() >= 3 &&
        (unsigned char)line[0] == 0xEF &&
        (unsigned char)line[1] == 0xBB &&
        (unsigned char)line[2] == 0xBF) {
        line.erase(0, 3);
    }
}

bool TripAnalyzer::splitCsv(const string& s, size_t b[3], size_t e[3]) {
    bool inQ = false;
    int col = 0;
    size_t start = 0;
    const size_t n = s.size();
    
    for (size_t i = 0; i <= n; ++i) {
        if (i < n && s[i] == '"') inQ = !inQ;
        if (!inQ && (i == n || s[i] == ',')) {
            if (col < 3) {
                b[col] = start;
                e[col] = i;
            }
            ++col;
            start = i + 1;
            if (col >= 3) return true;
        }
    }
    return col >= 3;
}

bool TripAnalyzer::fastParseHour(const string& s, size_t b, size_t e, int& hourOut) {
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

bool TripAnalyzer::betterZone(const ZoneCount& a, const ZoneCount& b) {
    if (a.count != b.count) return a.count > b.count;
    return a.zone < b.zone;
}

bool TripAnalyzer::betterSlot(const SlotCount& a, const SlotCount& b) {
    if (a.count != b.count) return a.count > b.count;
    if (a.zone != b.zone) return a.zone < b.zone;
    return a.hour < b.hour;
}

void TripAnalyzer::ingestFile(const string& csvPath) {
    ifstream file(csvPath);
    if (!file.is_open()) {
        return;
    }
    
    zones.clear();
    zones.reserve(200000);
    zones.max_load_factor(0.7f);
    
    string line;
    line.reserve(512);
    size_t b[3], e[3];
    bool firstLine = true;
    
    while (getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        
        stripBOM(line);
        if (line.empty()) continue;
        
        // Skip header line
        if (firstLine) {
            firstLine = false;
            if (line.find("TripID") != string::npos || 
                line.find("PickupZoneID") != string::npos ||
                line.find("PickupTime") != string::npos) {
                continue;
            }
        }
        
        if (!splitCsv(line, b, e)) continue;
        
        // Column 1: PickupZoneID
        size_t zoneB = b[1], zoneE = e[1];
        trimSpan(line, zoneB, zoneE);
        if (zoneB >= zoneE) continue;
        
        // Column 2: PickupTime
        size_t dtB = b[2], dtE = e[2];
        int hour = -1;
        if (!fastParseHour(line, dtB, dtE, hour)) continue;
        
        string zone(line, zoneB, zoneE - zoneB);
        
        ZoneStats& stats = zones[zone];
        ++stats.total;
        ++stats.byHour[hour];
    }
    
    file.close();
}

vector<ZoneCount> TripAnalyzer::topZones(int k) const {
    vector<ZoneCount> out;
    if (zones.empty() || k <= 0) return out;
    
    out.reserve(zones.size());
    for (const auto& kv : zones) {
        out.push_back({ kv.first, kv.second.total });
    }
    
    if (out.size() > static_cast<size_t>(k)) {
        partial_sort(out.begin(), out.begin() + k, out.end(), betterZone);
        out.resize(k);
    } else {
        sort(out.begin(), out.end(), betterZone);
    }
    
    return out;
}

vector<SlotCount> TripAnalyzer::topBusySlots(int k) const {
    vector<SlotCount> out;
    if (zones.empty() || k <= 0) return out;
    
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
    
    if (out.size() > static_cast<size_t>(k)) {
        partial_sort(out.begin(), out.begin() + k, out.end(), betterSlot);
        out.resize(k);
    } else {
        sort(out.begin(), out.end(), betterSlot);
    }
    
    return out;
}

