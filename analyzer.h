#ifndef ANALYZER_H
#define ANALYZER_H

#include <string>
#include <vector>
#include <unordered_map>

struct ZoneCount {
    std::string zone;
    long long count;
};

struct SlotCount {
    std::string zone;
    int hour;
    long long count;
};

class TripAnalyzer {
public:
    void ingestFile(const std::string& csvPath);
    std::vector<ZoneCount> topZones(int k = 10) const;
    std::vector<SlotCount> topBusySlots(int k = 10) const;

private:
    struct ZoneStats {
        long long total;
        long long byHour[24];
        ZoneStats() : total(0) {
            for (int i = 0; i < 24; ++i) byHour[i] = 0;
        }
    };

    std::unordered_map<std::string, ZoneStats> zones;
    
    static bool isSpace(unsigned char c);
    static void trimSpan(const std::string& s, size_t& b, size_t& e);
    static void stripBOM(std::string& line);
    static bool splitCsv(const std::string& s, size_t b[3], size_t e[3]);
    static bool fastParseHour(const std::string& s, size_t b, size_t e, int& hourOut);
    static bool betterZone(const ZoneCount& a, const ZoneCount& b);
    static bool betterSlot(const SlotCount& a, const SlotCount& b);
};

#endif

