#pragma once
#include <bits/stdc++.h>
using namespace std;

// Exception hierarchy
class BasicException {
protected:
    std::string msg;
public:
    explicit BasicException(const char *_message) : msg(_message ? _message : "") {}
    virtual const char *what() const { return msg.c_str(); }
    virtual ~BasicException() = default;
};

class ArgumentException : public BasicException {
public:
    explicit ArgumentException(const char *_message) : BasicException(_message) {}
};

class IteratorException : public BasicException {
public:
    explicit IteratorException(const char *_message) : BasicException(_message) {}
};

struct Pokemon {
    char name[12];
    int id;
    // type mask uses 7 bits: fire, water, grass, electric, ground, flying, dragon
    unsigned int type_mask;
};

class Pokedex {
private:
    std::string fileName;
    std::map<int, Pokemon> by_id;               // id -> Pokemon
    std::unordered_map<std::string, int> by_name; // name -> id

    // Allowed types and mapping
    const std::vector<std::string> type_list = {
        "fire", "water", "grass", "electric", "ground", "flying", "dragon"
    };
    std::unordered_map<std::string, int> type_index;
    // attack multiplier matrix [atk][def]
    double mult[7][7]{};

    void init_types() {
        for (int i = 0; i < (int)type_list.size(); ++i) type_index[type_list[i]] = i;
        // Initialize all to 1.0
        for (int i = 0; i < 7; ++i)
            for (int j = 0; j < 7; ++j) mult[i][j] = 1.0;
        // Based on simplified Pokemon chart described in README
        // Fire
        mult[idx("fire")][idx("grass")] = 2.0;
        mult[idx("fire")][idx("water")] = 0.5;
        mult[idx("fire")][idx("fire")] = 0.5;
        mult[idx("fire")][idx("dragon")] = 0.5;
        // Water
        mult[idx("water")][idx("fire")] = 2.0;
        mult[idx("water")][idx("ground")] = 2.0;
        mult[idx("water")][idx("water")] = 0.5;
        mult[idx("water")][idx("grass")] = 0.5;
        mult[idx("water")][idx("dragon")] = 0.5;
        // Grass
        mult[idx("grass")][idx("water")] = 2.0;
        mult[idx("grass")][idx("ground")] = 2.0;
        mult[idx("grass")][idx("fire")] = 0.5;
        mult[idx("grass")][idx("grass")] = 0.5;
        mult[idx("grass")][idx("flying")] = 0.5;
        mult[idx("grass")][idx("dragon")] = 0.5;
        // Electric
        mult[idx("electric")][idx("water")] = 2.0;
        mult[idx("electric")][idx("flying")] = 2.0;
        mult[idx("electric")][idx("electric")] = 0.5;
        mult[idx("electric")][idx("grass")] = 0.5;
        mult[idx("electric")][idx("dragon")] = 0.5;
        mult[idx("electric")][idx("ground")] = 0.0; // no effect
        // Ground
        mult[idx("ground")][idx("fire")] = 2.0;
        mult[idx("ground")][idx("electric")] = 2.0;
        mult[idx("ground")][idx("grass")] = 0.5;
        mult[idx("ground")][idx("flying")] = 0.0; // no effect
        // Flying
        mult[idx("flying")][idx("grass")] = 2.0;
        // Flying vs Electric is not very effective
        mult[idx("flying")][idx("electric")] = 0.5;
        // Dragon
        mult[idx("dragon")][idx("dragon")] = 2.0;
    }

    int idx(const std::string &t) const {
        auto it = type_index.find(t);
        if (it == type_index.end()) return -1;
        return it->second;
    }

    bool is_valid_name(const char *name) const {
        if (name == nullptr || *name == '\0') return false;
        for (const char *p = name; *p; ++p) if (!isalpha((unsigned char)*p)) return false;
        return true;
    }

    bool is_valid_id(int id) const { return id > 0; }

    unsigned int parse_types_mask(const char *types, std::string &invalid) const {
        if (types == nullptr || *types == '\0') { invalid = ""; return 0; }
        unsigned int mask = 0;
        std::string s(types);
        std::string token;
        std::stringstream ss(s);
        while (std::getline(ss, token, '#')) {
            if (token.empty()) continue;
            auto it = type_index.find(token);
            if (it == type_index.end()) { invalid = token; return 0; }
            mask |= (1u << it->second); // de-duplicate implicitly
        }
        if (mask == 0) invalid = ""; // no valid types
        return mask;
    }

    static std::string join_types(unsigned int mask, const std::vector<std::string> &type_list) {
        std::vector<std::string> ts;
        for (int i = 0; i < (int)type_list.size(); ++i) if (mask & (1u << i)) ts.push_back(type_list[i]);
        std::sort(ts.begin(), ts.end());
        std::string out;
        for (size_t i = 0; i < ts.size(); ++i) {
            if (i) out.push_back('#');
            out += ts[i];
        }
        return out;
    }

    void load_from_file() {
        by_id.clear();
        by_name.clear();
        std::ifstream fin(fileName);
        if (!fin.is_open()) {
            // create empty file
            std::ofstream fout(fileName); (void)fout;
            return;
        }
        std::string line;
        while (std::getline(fin, line)) {
            if (line.empty()) continue;
            std::stringstream ss(line);
            long long idll; ss >> idll; int id = (int)idll;
            std::string name, types;
            ss >> name; ss >> types;
            if (name.empty()) continue;
            Pokemon p{}; p.id = id; p.type_mask = 0;
            std::string invalid;
            // Parse types, tolerate any invalid leftover silently on load (should not happen if our save was used)
            unsigned int mask = 0; if (!types.empty()) {
                std::stringstream ss2(types);
                std::string tk; while (std::getline(ss2, tk, '#')) {
                    auto it = type_index.find(tk);
                    if (it != type_index.end()) mask |= (1u << it->second);
                }
            }
            p.type_mask = mask;
            std::snprintf(p.name, sizeof(p.name), "%s", name.c_str());
            by_id[id] = p;
            by_name[name] = id;
        }
    }

    void save_to_file() const {
        std::ofstream fout(fileName, std::ios::trunc);
        for (const auto &kv : by_id) {
            const auto &p = kv.second;
            fout << p.id << ' ' << p.name << ' ' << join_types(p.type_mask, type_list) << '\n';
        }
    }

public:
    explicit Pokedex(const char *_fileName) : fileName(_fileName ? _fileName : "data.txt") {
        init_types();
        load_from_file();
    }

    ~Pokedex() { save_to_file(); }

    bool pokeAdd(const char *name, int id, const char *types) {
        if (!is_valid_name(name)) {
            std::string m = std::string("Argument Error: PM Name Invalid (") + (name ? name : "") + ")";
            throw ArgumentException(m.c_str());
        }
        if (!is_valid_id(id)) {
            std::string m = std::string("Argument Error: PM ID Invalid (") + std::to_string(id) + ")";
            throw ArgumentException(m.c_str());
        }
        std::string invalid;
        unsigned int mask = parse_types_mask(types, invalid);
        if (!invalid.empty()) {
            std::string m = std::string("Argument Error: PM Type Invalid (") + invalid + ")";
            throw ArgumentException(m.c_str());
        }
        if (mask == 0) {
            std::string m = std::string("Argument Error: PM Type Invalid (");
            m += ")";
            throw ArgumentException(m.c_str());
        }
        if (by_id.count(id) || by_name.count(name)) return false;
        Pokemon p{}; p.id = id; p.type_mask = mask; std::snprintf(p.name, sizeof(p.name), "%s", name);
        by_id[id] = p;
        by_name[name] = id;
        return true;
    }

    bool pokeDel(int id) {
        auto it = by_id.find(id);
        if (it == by_id.end()) return false;
        by_name.erase(std::string(it->second.name));
        by_id.erase(it);
        return true;
    }

    std::string pokeFind(int id) const {
        auto it = by_id.find(id);
        if (it == by_id.end()) return "None";
        return std::string(it->second.name);
    }

    std::string typeFind(const char *types) const {
        std::string invalid;
        unsigned int query = parse_types_mask(types, invalid);
        if (!invalid.empty()) {
            std::string m = std::string("Argument Error: PM Type Invalid (") + invalid + ")";
            throw ArgumentException(m.c_str());
        }
        if (query == 0) return std::string("None");
        std::vector<std::pair<int, std::string>> hits; // (id, name)
        for (const auto &kv : by_id) {
            const auto &p = kv.second;
            // Must contain all query types
            if ( (p.type_mask & query) == query ) hits.emplace_back(p.id, std::string(p.name));
        }
        if (hits.empty()) return std::string("None");
        std::sort(hits.begin(), hits.end()); // id ascending
        std::ostringstream out;
        out << hits.size() << '\n';
        for (auto &x : hits) out << x.second << '\n';
        std::string s = out.str();
        if (!s.empty() && s.back() == '\n') s.pop_back();
        return s;
    }

    float attack(const char *type, int id) const {
        // Exception priority: validate type first
        std::string st = type ? std::string(type) : std::string();
        auto tid = type_index.find(st);
        if (tid == type_index.end()) {
            std::string m = std::string("Argument Error: PM Type Invalid (") + st + ")";
            throw ArgumentException(m.c_str());
        }
        auto it = by_id.find(id);
        if (it == by_id.end()) return -1.0f;
        const Pokemon &p = it->second;
        double ans = 1.0;
        // multiply effectiveness for each defending type bit
        for (int i = 0; i < 7; ++i) if (p.type_mask & (1u << i)) {
            ans *= mult[tid->second][i];
            if (ans == 0.0) break;
        }
        return static_cast<float>(ans);
    }

    int catchTry() const {
        if (by_id.empty()) return 0;
        // Start with min id pokemon
        auto it0 = by_id.begin();
        int owned_count = 1;
        unsigned int avail_types = it0->second.type_mask;
        std::set<int> owned_ids; owned_ids.insert(it0->first);
        bool changed = true;
        while (changed) {
            changed = false;
            for (const auto &kv : by_id) {
                int pid = kv.first; const Pokemon &p = kv.second;
                if (owned_ids.count(pid)) continue;
                // can any available type deal >= 2x?
                bool ok = false;
                for (int t = 0; t < 7 && !ok; ++t) if (avail_types & (1u << t)) {
                    double dmg = 1.0;
                    for (int i = 0; i < 7; ++i) if (p.type_mask & (1u << i)) {
                        dmg *= mult[t][i];
                        if (dmg == 0.0) break;
                    }
                    if (dmg >= 2.0 - 1e-9) ok = true;
                }
                if (ok) {
                    owned_ids.insert(pid);
                    owned_count++;
                    avail_types |= p.type_mask;
                    changed = true;
                }
            }
        }
        return owned_count;
    }

    struct iterator {
        using MapIt = std::map<int, Pokemon>::iterator;
        Pokedex *owner = nullptr;
        MapIt it;

        iterator() = default;
        iterator(Pokedex *o, MapIt i) : owner(o), it(i) {}

        iterator &operator++() {
            if (!owner) throw IteratorException("Iterator Error: Invalid Iterator");
            if (it == owner->by_id.end()) throw IteratorException("Iterator Error: Increment Past End");
            ++it; return *this;
        }
        iterator &operator--() {
            if (!owner) throw IteratorException("Iterator Error: Invalid Iterator");
            if (it == owner->by_id.begin()) throw IteratorException("Iterator Error: Decrement Past Begin");
            --it; return *this;
        }
        iterator operator++(int) { iterator tmp = *this; ++(*this); return tmp; }
        iterator operator--(int) { iterator tmp = *this; --(*this); return tmp; }
        iterator & operator = (const iterator &rhs) { owner = rhs.owner; it = rhs.it; return *this; }
        bool operator == (const iterator &rhs) const { return owner == rhs.owner && it == rhs.it; }
        bool operator != (const iterator &rhs) const { return !(*this == rhs); }
        Pokemon & operator*() const {
            if (!owner || it == owner->by_id.end()) throw IteratorException("Iterator Error: Dereference Invalid Iterator");
            return it->second;
        }
        Pokemon *operator->() const {
            if (!owner || it == owner->by_id.end()) throw IteratorException("Iterator Error: Dereference Invalid Iterator");
            return &(it->second);
        }
    };

    iterator begin() { return iterator(this, by_id.begin()); }
    iterator end() { return iterator(this, by_id.end()); }
};
