#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <queue>
#include <limits>
#include <set>
#include <algorithm>

using namespace std;

// Kante im Graphen (Verbindung zwischen zwei Stationen)
struct Connection {
    string target_station;
    int travel_time;
    string line_name;
};

// Für Rückverfolgung: wie kam man zu einer Station
struct StationPathInfo {
    int total_cost;
    string previous_station;
    string via_line;
};

// Adjazenzliste zur Darstellung des Graphen
unordered_map<string, vector<Connection>> transit_network;

void loadTransitNetwork(const string& file_path) {
    ifstream input_file(file_path);
    if (!input_file) {
        cerr << "Fehler beim Öffnen der Datei: " << file_path << endl;
        exit(1);
    }

    string line_data;
    while (getline(input_file, line_data)) {
        stringstream line_stream(line_data);
        string line_name;
        getline(line_stream, line_name, ':');
        line_name = line_name.substr(0, line_name.find(':'));

        vector<string> stations;
        vector<int> travel_times;

        string token;
        while (line_stream >> token) {
            if (token[0] == '"') {
                string station_name = token.substr(1);
                while (station_name.back() != '"') {
                    string next_part;
                    line_stream >> next_part;
                    station_name += " " + next_part;
                }
                station_name.pop_back(); // Entferne das schließende "
                stations.push_back(station_name);
            } else {
                travel_times.push_back(stoi(token));
            }
        }

        for (size_t i = 0; i < travel_times.size(); ++i) {
            const string& from = stations[i];
            const string& to = stations[i + 1];
            int cost = travel_times[i];

            // Bidirektionale Verbindung
            transit_network[from].push_back({to, cost, line_name});
            transit_network[to].push_back({from, cost, line_name});
        }
    }
}

void findShortestTransitPath(const string& start_station, const string& target_station) {
    unordered_map<string, int> min_cost_to_station;
    unordered_map<string, StationPathInfo> path_info;
    set<string> visited_stations;

    for (const auto& entry : transit_network)
        min_cost_to_station[entry.first] = numeric_limits<int>::max();

    min_cost_to_station[start_station] = 0;

    using QueueElement = pair<int, string>; // {Gesamtkosten, Stationsname}
    priority_queue<QueueElement, vector<QueueElement>, greater<QueueElement>> station_queue;
    station_queue.push({0, start_station});

    while (!station_queue.empty()) {
        auto [current_cost, current_station] = station_queue.top();
        station_queue.pop();

        if (visited_stations.count(current_station))
            continue;
        visited_stations.insert(current_station);

        if (current_station == target_station)
            break;

        for (const auto& connection : transit_network[current_station]) {
            int new_cost = current_cost + connection.travel_time;
            if (new_cost < min_cost_to_station[connection.target_station]) {
                min_cost_to_station[connection.target_station] = new_cost;
                path_info[connection.target_station] = {
                    new_cost, current_station, connection.line_name
                };
                station_queue.push({new_cost, connection.target_station});
            }
        }
    }

    if (min_cost_to_station[target_station] == numeric_limits<int>::max()) {
        cout << "Kein Pfad von " << start_station << " nach " << target_station << " gefunden." << endl;
        return;
    }

    // Pfad rückverfolgen
    vector<pair<string, string>> path_sequence; // {Station, Linie}
    string current_station = target_station;

    while (current_station != start_station) {
        StationPathInfo info = path_info[current_station];
        path_sequence.push_back({current_station, info.via_line});
        current_station = info.previous_station;
    }
    path_sequence.push_back({start_station, ""});
    reverse(path_sequence.begin(), path_sequence.end());

    // Ausgabe
    cout << "Kürzester Pfad von \"" << start_station << "\" nach \"" << target_station << "\":" << endl;

    string current_line = path_sequence[1].second;
    cout << "   " << path_sequence[0].first;

    for (size_t i = 1; i < path_sequence.size(); ++i) {
        const string& station = path_sequence[i].first;
        const string& line = path_sequence[i].second;

        if (line != current_line) {
            cout << " --[Umsteigen auf " << line << "]--> ";
            current_line = line;
        } else {
            cout << " --[" << line << "]--> ";
        }

        cout << station;
    }

    cout << "\nGesamtkosten (Fahrzeit): " << min_cost_to_station[target_station] << endl;
}

int main() {
    string graph_filename = "fileGraph.txt";
    string start_station;
    string goal_station;

    // Startstation aus Datei lesen
    ifstream start_file("start.txt");
    if (!start_file || !(getline(start_file, start_station))) {
        cerr << "Fehler beim Lesen der Datei start.txt" << endl;
        return 1;
    }

    // Zielstation aus Datei lesen
    ifstream ziel_file("ziel.txt");
    if (!ziel_file || !(getline(ziel_file, goal_station))) {
        cerr << "Fehler beim Lesen der Datei ziel.txt" << endl;
        return 1;
    }

    // Verkehrsnetz laden und Pfad finden
    loadTransitNetwork(graph_filename);
    findShortestTransitPath(start_station, goal_station);

    return 0;
}

