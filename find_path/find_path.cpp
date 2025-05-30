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

//kante -> Verbindung zwischen Stationen
struct Connection {
    string target_station;
    int travel_time;
    string line_name;
};

//info zu Verbindung für retrace
struct StationPathInfo {
    int total_cost;
    string previous_station;
    string via_line;
};

//Adjazenzliste
unordered_map<string, vector<Connection>> transit_network;

//graph aus file laden
void loadTransitNetwork(const string& file_path)
{
    ifstream input_file(file_path);
    if (!input_file)
    {
        cerr << "Fehler beim Öffnen der Datei: " << file_path << endl;
        exit(1);
    }

    string line_data;
    while (getline(input_file, line_data))
    {
        stringstream line_stream(line_data);
        string line_name;
        getline(line_stream, line_name, ':');
        line_name = line_name.substr(0, line_name.find(':'));   //linienname steht von vor : einlesen

        vector<string> stations;
        vector<int> travel_times;

        string token;
        while (line_stream >> token)
        {
            if (token[0] == '"')
            {   //anfang von stationname gefunden
                string station_name = token.substr(1);
                while (station_name.back() != '"')
                {   //stationname einlesen bis schliesendes "
                    string next_part;
                    line_stream >> next_part;
                    station_name += " " + next_part;
                }
                station_name.pop_back(); //löschen von " am Ende
                stations.push_back(station_name);   //station einfuegen in stations
            }
            else
            {
                travel_times.push_back(stoi(token));
            }
        }

        for (size_t i = 0; i < travel_times.size(); ++i)
        {
            const string& from = stations[i];   //referenz auf aktuelle station
            const string& to = stations[i + 1]; //referenz auf naechste station
            int cost = travel_times[i];         //kosten der verbindung zwischen diesen stationen

            //bidirektional
            transit_network[from].push_back({to, cost, line_name});
            transit_network[to].push_back({from, cost, line_name});
        }
    }
}

bool checkForStation(const string& station)
{
    if(transit_network.find(station) == transit_network.end())
        return false;
    else return true;
}

void findShortestTransitPath(const string& start_station, const string& target_station)
{
    //pruefen ob gültige eingabe der stationen
    bool chkStart = checkForStation(start_station);
    bool chkTarget = checkForStation(target_station);

    if(!chkStart || !chkTarget)
    {
        string notFoundStations;
        notFoundStations += chkStart ? " " : start_station + " ";
        notFoundStations += chkTarget ? " " : target_station + " ";

        cout<<"Die gewaehlte/n Station/en "<<notFoundStations<<"existieren nicht!"<<endl;
        return;
    }

    unordered_map<string, int> min_cost_to_station;
    unordered_map<string, StationPathInfo> path_info;
    set<string> visited_stations;   //set weil keine duplicate vorkommen koennen

    //kosten fuer alle verbindungen auf maximum setzen fuer vergleiche
    for (const auto& entry : transit_network)
        min_cost_to_station[entry.first] = numeric_limits<int>::max();

    //kosten fuer verbindung zu startstation = 0
    min_cost_to_station[start_station] = 0;

    using QueueElement = pair<int, string>; //{gesamtkosten, stationsname} definiere typname
    //priority queue -> elemente sortiert nach gesamtkosten (aufsteigend)
    priority_queue<QueueElement, vector<QueueElement>, greater<QueueElement>> station_queue;
    station_queue.push({0, start_station});

    //dijkstra
    while (!station_queue.empty())
    {
        auto [current_cost, current_station] = station_queue.top();
        station_queue.pop();

        if (visited_stations.count(current_station))    //falls station schon besucht
            continue;
        visited_stations.insert(current_station);

        if (current_station == target_station)  //abbruch falls station = ziel
            break;

        for (const auto& connection : transit_network[current_station])
        {   //verbindungen aktueller station besuchen
            int new_cost = current_cost + connection.travel_time;   //update kosten
            if (new_cost < min_cost_to_station[connection.target_station])
            {   //falls kosten geringer als min
                min_cost_to_station[connection.target_station] = new_cost;
                path_info[connection.target_station] = {
                    new_cost, current_station, connection.line_name
                };
                station_queue.push({new_cost, connection.target_station});
            }
        }
    }

    if (min_cost_to_station[target_station] == numeric_limits<int>::max())
    {   //falls kein pfad gefunden
        cout << "Kein Pfad von " << start_station << " nach " << target_station << " gefunden." << endl;
        return;
    }

    //retrace pfad
    vector<pair<string, string>> path_sequence; //abfolge von stationen mit entsprechender linie
    string current_station = target_station;

    while (current_station != start_station)
    {   //zurueckgehen bis start erreicht
        StationPathInfo info = path_info[current_station];
        path_sequence.push_back({current_station, info.via_line});  //aktuelle station einfuegen
        current_station = info.previous_station;    //weiter zu naechster station
    }
    path_sequence.push_back({start_station, ""});   //startstation einfuegen ohne linie
    reverse(path_sequence.begin(), path_sequence.end());    //reihenfolge aendern von start zu ziel

    //ausgabe
    cout << "Kuerzester Pfad von \"" << start_station << "\" zu \"" << target_station << "\":" << endl;

    string current_line = path_sequence[1].second;
    cout << endl << path_sequence[0].first;

    for (size_t i = 1; i < path_sequence.size(); ++i)
    {
        const string& station = path_sequence[i].first;
        const string& line = path_sequence[i].second;

        if (line != current_line) {
            cout << " --[Umsteigen auf " << line << "]--> ";
            current_line = line;
        } else {
            cout << " -[" << line << "]-> ";
        }

        cout << station;
    }

    cout <<endl<< "\nFahrtzeit insgesamt: " << min_cost_to_station[target_station] << endl;
}

int main(int argc, char* argv[])
{

    string graph_filename = argv[1];
    string start_station;
    string goal_station;

    //start aus datei einlesen
    ifstream start_file(argv[2]);
    if (!start_file || !(getline(start_file, start_station)))
    {
        cerr << "Fehler beim Lesen der Datei "<<argv[2]<< endl;
        return 1;
    }

    //ziel aus datei einlesen
    ifstream ziel_file(argv[3]);
    if (!ziel_file || !(getline(ziel_file, goal_station)))
    {
        cerr << "Fehler beim Lesen der Datei "<<argv[3]<< endl;
        return 1;
    }

    //verkehrnetzwerk laden
    loadTransitNetwork(graph_filename);
    //kuerzesten pfad finden
    findShortestTransitPath(start_station, goal_station);

    return 0;
}

