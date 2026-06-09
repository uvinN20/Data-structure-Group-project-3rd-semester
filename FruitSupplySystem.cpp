/*
======================================================
  Algorithm-Driven Smart Fruit Supply Chain System
  Language : C++17
  Compile  : g++ -std=c++17 FruitSupplySystem.cpp -o system
  Run      : ./system
 
  JSON OUTPUT PATH: Update JSON_OUTPUT_PATH below to match
  your Vue project's public folder location.
======================================================
*/
 
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <queue>
#include <stack>
#include <iomanip>
#include <algorithm>
#include <limits>
 
using namespace std;
 
// ======================================================
//  >>>  UPDATE THIS PATH to your Vue project's /public/
// ======================================================
const string JSON_OUTPUT_PATH = "C:/Users/uvinn/FruitProject/my-vue-app/public/";
 
// ── shortcut so we don't repeat this long call everywhere ──
void flushInput() {
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}
 
// ======================================================
//  COLOUR (VS Code terminal)
// ======================================================
#define RESET   "\033[0m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define CYAN    "\033[36m"
#define RED     "\033[31m"
#define BOLD    "\033[1m"
 
// ======================================================
//  DATA MODELS
// ======================================================
 
struct Supplier {
    string id;
    string name;
    string phone;
    float  distance;
    string password;
    string fruitType;
    float  unitPrice;
    int    quantity;
    string supplyDate;
    bool   doesDelivery;
    float  freshnessScore;
};
 
struct Seller {
    string id;
    string name;
    string phone;
    float  distance;
    string password;
    string productType;
    int    quantity;
    float  customerRating;
    float  yearlySales;
};
 
struct ProductionItem {
    string name;
    int    quantity;
    float  unitPrice;
};
 
struct RouteEdge {
    string from;
    string to;
    float  distance;
};
 
struct FruitBatch {
    string fruitType;
    int    quantity;
    string date;
};
 
// ======================================================
//  GLOBAL STORAGE
// ======================================================
 
map<string, Supplier>  supplierDB;
map<string, Seller>    sellerDB;
vector<ProductionItem> productionStock;
vector<RouteEdge>      routeGraph;
map<string, float>     fruitPriceLimit;
queue<FruitBatch>      inventoryQueue;
 
float vehicleMaintenanceCost = 0;
float monthlyIncome = 0;
float monthlyCost   = 0;
 
// Default admin and manager accounts
bool adminLoggedIn = false;
bool supplierMgrLoggedIn = false;
bool sellerMgrLoggedIn = false;
bool productionMgrLoggedIn = false;
 
// ======================================================
//  AUTO ID GENERATOR
// ======================================================
 
string makeSupplierID() {
    int n = (int)supplierDB.size() + 1;
    string num = to_string(n);
    while ((int)num.size() < 4) num = "0" + num;
    return "SP" + num;
}
 
string makeSellerID() {
    int n = (int)sellerDB.size() + 1;
    string num = to_string(n);
    while ((int)num.size() < 4) num = "0" + num;
    return "SE" + num;
}
 
// ======================================================
//  DSA: SUPPLIER RANKING SCORE
//  Score = (Freshness x 0.4) - (Distance x 0.3) - (Price x 0.3)
// ======================================================
 
float supplierScore(const Supplier& s) {
    return (0.4f * s.freshnessScore)
         - (0.3f * s.distance   / 10.0f)
         - (0.3f * s.unitPrice  / 100.0f);
}
 
// ======================================================
//  JSON EXPORT — saves to Vue /public/ folder
//  Frontend auto-refreshes every 3 seconds via axios
// ======================================================
 
void saveSuppliersToJSON() {
    string path = JSON_OUTPUT_PATH + "suppliers.json";
    ofstream file(path);
 
    if (!file.is_open()) {
        cout << RED << "ERROR: Cannot write to " << path << "\n"
             << "Check that JSON_OUTPUT_PATH is correct.\n" << RESET;
        return;
    }
 
    file << "[\n";
    bool first = true;
 
    for (auto& [id, s] : supplierDB) {
        if (!first) file << ",\n";
 
        file << "  {\n";
        file << "    \"id\": \""       << s.id           << "\",\n";
        file << "    \"name\": \""     << s.name         << "\",\n";
        file << "    \"phone\": \""    << s.phone        << "\",\n";
        file << "    \"fruit\": \""    << s.fruitType    << "\",\n";
        file << "    \"distance\": "   << s.distance     << ",\n";
        file << "    \"price\": "      << s.unitPrice     << ",\n";
        file << "    \"quantity\": "   << s.quantity      << ",\n";
        file << "    \"freshness\": "  << s.freshnessScore << ",\n";
        file << "    \"supplyDate\": \"" << s.supplyDate << "\",\n";
        file << "    \"delivery\": "   << (s.doesDelivery ? "true" : "false") << ",\n";
        file << "    \"score\": "      << fixed << setprecision(6) << supplierScore(s) << "\n";
        file << "  }";
 
        first = false;
    }
 
    file << "\n]\n";
    file.close();
 
    cout << GREEN << "[JSON] suppliers.json updated (" << supplierDB.size() << " records)\n" << RESET;
}
 
void saveInventoryToJSON() {
    string path = JSON_OUTPUT_PATH + "inventory.json";
    ofstream file(path);
 
    if (!file.is_open()) {
        cout << RED << "ERROR: Cannot write to " << path << "\n"
             << "Check that JSON_OUTPUT_PATH is correct.\n" << RESET;
        return;
    }
 
    queue<FruitBatch> temp = inventoryQueue;
 
    file << "[\n";
    bool first = true;
 
    while (!temp.empty()) {
        FruitBatch b = temp.front();
        temp.pop();
 
        if (!first) file << ",\n";
 
        file << "  {\n";
        file << "    \"fruit\": \"" << b.fruitType       << "\",\n";
        file << "    \"qty\": \""   << b.quantity << "kg" << "\",\n";
        file << "    \"date\": \""  << b.date             << "\"\n";
        file << "  }";
 
        first = false;
    }
 
    file << "\n]\n";
    file.close();
 
    cout << GREEN << "[JSON] inventory.json updated (" << inventoryQueue.size() << " records)\n" << RESET;
}
 
// Convenience — always save both after any data change
void saveAllJSON() {
    saveSuppliersToJSON();
    saveInventoryToJSON();
}
 
// ======================================================
//  SUPPLIER RANKING (Max-Heap)
// ======================================================
 
struct SupplierCmp {
    bool operator()(const Supplier& a, const Supplier& b) {
        return supplierScore(a) < supplierScore(b);
    }
};
 
void showSupplierRanking() {
    if (supplierDB.empty()) {
        cout << RED << "No suppliers registered.\n" << RESET;
        return;
    }
 
    priority_queue<Supplier, vector<Supplier>, SupplierCmp> heap;
    for (auto& [id, s] : supplierDB) heap.push(s);
 
    cout << YELLOW << "\n--- Supplier Ranking (Best to Worst) ---\n" << RESET;
    cout << left
         << setw(8)  << "Rank"
         << setw(10) << "ID"
         << setw(16) << "Name"
         << setw(12) << "Fruit"
         << setw(10) << "Price"
         << setw(10) << "Dist(km)"
         << setw(11) << "Freshness"
         << "Score\n";
    cout << string(88, '-') << "\n";
 
    int rank = 1;
    while (!heap.empty()) {
        Supplier s = heap.top(); heap.pop();
        cout << left
             << setw(8)  << ("#" + to_string(rank++))
             << setw(10) << s.id
             << setw(16) << s.name
             << setw(12) << (s.fruitType.empty() ? "-" : s.fruitType)
             << setw(10) << s.unitPrice
             << setw(10) << s.distance
             << setw(11) << s.freshnessScore
             << fixed << setprecision(3) << supplierScore(s) << "\n";
    }
}
 
// ======================================================
//  SELLER RANKING (Max-Heap by yearly sales)
// ======================================================
 
struct SellerCmp {
    bool operator()(const Seller& a, const Seller& b) {
        return a.yearlySales < b.yearlySales;
    }
};
 
void showSellerRanking() {
    if (sellerDB.empty()) {
        cout << RED << "No sellers registered.\n" << RESET;
        return;
    }
 
    priority_queue<Seller, vector<Seller>, SellerCmp> heap;
    for (auto& [id, s] : sellerDB) heap.push(s);
 
    cout << YELLOW << "\n--- Seller Ranking (High to Low Sales) ---\n" << RESET;
    cout << left
         << setw(8)  << "Rank"
         << setw(10) << "ID"
         << setw(18) << "Name"
         << setw(14) << "Product"
         << setw(14) << "Yearly Sales"
         << "Rating\n";
    cout << string(75, '-') << "\n";
 
    int rank = 1;
    while (!heap.empty()) {
        Seller s = heap.top(); heap.pop();
        cout << left
             << setw(8)  << ("#" + to_string(rank++))
             << setw(10) << s.id
             << setw(18) << s.name
             << setw(14) << s.productType
             << setw(14) << fixed << setprecision(2) << s.yearlySales
             << s.customerRating << "/10\n";
    }
}
 
// ======================================================
//  GRAPH — Route map & Dijkstra's
// ======================================================
 
void showRouteMap() {
    if (routeGraph.empty()) {
        cout << RED << "No routes added yet.\n" << RESET;
        return;
    }
    cout << CYAN << "\n--- Route Map (Graph) ---\n" << RESET;
    for (auto& e : routeGraph) {
        cout << "  " << e.from << "  <-->  " << e.to
             << "  [" << e.distance << " km]\n";
    }
}
 
void findShortestRoute(const string& src, const string& dest) {
    map<string, vector<pair<string, float>>> adj;
    for (auto& e : routeGraph) {
        adj[e.from].push_back({e.to,   e.distance});
        adj[e.to  ].push_back({e.from, e.distance});
    }
 
    if (adj.find(src) == adj.end()) {
        cout << RED << "Source location not found in route map.\n" << RESET;
        return;
    }
 
    map<string, float>  dist;
    map<string, string> prev;
    for (auto& [node, _] : adj) dist[node] = 1e9f;
    dist[src] = 0;
 
    priority_queue<pair<float, string>,
                   vector<pair<float, string>>,
                   greater<>> pq;
    pq.push({0.0f, src});
 
    while (!pq.empty()) {
        auto [d, u] = pq.top(); pq.pop();
        if (d > dist[u]) continue;
        for (auto& [v, w] : adj[u]) {
            if (dist[u] + w < dist[v]) {
                dist[v] = dist[u] + w;
                prev[v] = u;
                pq.push({dist[v], v});
            }
        }
    }
 
    if (dist.find(dest) == dist.end() || dist[dest] >= 1e9f) {
        cout << RED << "No route found between " << src << " and " << dest << "\n" << RESET;
        return;
    }
 
    stack<string> path;
    string cur = dest;
    while (cur != src) {
        path.push(cur);
        if (prev.find(cur) == prev.end()) {
            cout << RED << "Route broken.\n" << RESET;
            return;
        }
        cur = prev[cur];
    }
    path.push(src);
 
    cout << GREEN << "\nShortest route: ";
    while (!path.empty()) {
        cout << path.top();
        path.pop();
        if (!path.empty()) cout << " -> ";
    }
    cout << "  [" << fixed << setprecision(1) << dist[dest] << " km]\n" << RESET;
}
 
// ======================================================
//  INVENTORY QUEUE (FIFO)
// ======================================================
 
void addInventory(const string& fruit, int qty, const string& date) {
    inventoryQueue.push({fruit, qty, date});
    cout << GREEN << "[Inventory] " << qty << " kg of " << fruit
         << " added (Date: " << date << ")\n" << RESET;
}
 
void processInventory() {
    if (inventoryQueue.empty()) {
        cout << RED << "Inventory is empty.\n" << RESET;
        return;
    }
    FruitBatch b = inventoryQueue.front();
    inventoryQueue.pop();
    cout << YELLOW << "[FIFO] Processing: " << b.quantity << " kg of "
         << b.fruitType << "  (received: " << b.date << ")\n" << RESET;
    saveInventoryToJSON();
}
 
void showInventory() {
    queue<FruitBatch> tmp = inventoryQueue;
    if (tmp.empty()) {
        cout << RED << "Inventory is empty.\n" << RESET;
        return;
    }
    cout << YELLOW << "\n--- Inventory (FIFO — oldest first) ---\n" << RESET;
    int i = 1;
    while (!tmp.empty()) {
        FruitBatch b = tmp.front(); tmp.pop();
        cout << "  " << i++ << ". " << left << setw(12) << b.fruitType
             << b.quantity << " kg   Date: " << b.date << "\n";
    }
}
 
// ======================================================
//  HELPER
// ======================================================
bool priceOk(const string& fruit, float price) {
    if (fruitPriceLimit.find(fruit) == fruitPriceLimit.end()) return true;
    return price <= fruitPriceLimit[fruit];
}
 
// ======================================================
//  INPUT HELPERS
// ======================================================
 
string inputLine(const string& prompt) {
    string val;
    cout << prompt;
    getline(cin, val);
    return val;
}
 
float inputFloat(const string& prompt) {
    float val;
    cout << prompt;
    while (!(cin >> val)) {
        cin.clear();
        flushInput();
        cout << RED << "Invalid input. Try again: " << RESET;
    }
    flushInput();
    return val;
}
 
int inputInt(const string& prompt) {
    int val;
    cout << prompt;
    while (!(cin >> val)) {
        cin.clear();
        flushInput();
        cout << RED << "Invalid input. Try again: " << RESET;
    }
    flushInput();
    return val;
}
 
// ======================================================
//  1. SUPPLIER PORTAL
// ======================================================
 
void supplierSignIn() {
    cout << BOLD << "\n=== Supplier Sign In ===\n" << RESET;
 
    Supplier s;
    s.id = makeSupplierID();
 
    s.name     = inputLine("Full Name                   : ");
    s.phone    = inputLine("Phone Number                : ");
    s.distance = inputFloat("Distance from factory (km) : ");
    s.password = inputLine("Password                    : ");
    string confirm = inputLine("Confirm Password            : ");
 
    if (s.password != confirm) {
        cout << RED << "Passwords do not match. Try again.\n" << RESET;
        return;
    }
 
    s.fruitType      = "";
    s.unitPrice      = 0;
    s.quantity       = 0;
    s.supplyDate     = "";
    s.doesDelivery   = false;
    s.freshnessScore = 7.0f;
 
    supplierDB[s.id] = s;
    saveAllJSON();
 
    cout << GREEN << "\nRegistered successfully!\n"
         << "Your Supplier ID : " << s.id << "\n" << RESET;
}
 
void supplierLogin() {
    cout << BOLD << "\n=== Supplier Login ===\n" << RESET;
 
    string id = inputLine("Supplier ID : ");
    string pw  = inputLine("Password    : ");
 
    if (supplierDB.find(id) == supplierDB.end() || supplierDB[id].password != pw) {
        cout << RED << "Invalid ID or password.\n" << RESET;
        return;
    }
 
    Supplier& s = supplierDB[id];
    cout << GREEN << "Welcome, " << s.name << "!\n" << RESET;
 
    int ch;
    do {
        cout << CYAN << "\n--- Your Account (" << s.id << ") ---\n" << RESET;
        cout << "Name          : " << s.name          << "\n";
        cout << "Phone         : " << s.phone         << "\n";
        cout << "Distance      : " << s.distance      << " km\n";
        cout << "Fruit         : " << (s.fruitType.empty() ? "Not set" : s.fruitType) << "\n";
        cout << "Unit Price    : " << s.unitPrice      << " LKR/kg\n";
        cout << "Quantity      : " << s.quantity       << " kg\n";
        cout << "Freshness     : " << s.freshnessScore << "/10\n";
        cout << "Rank Score    : " << fixed << setprecision(3) << supplierScore(s) << "\n";
 
        cout << "\n1. Update fruit supply details\n"
             << "2. View my rank\n"
             << "3. Change password\n"
             << "0. Logout\n";
        ch = inputInt("Choice: ");
 
        if (ch == 1) {
            s.fruitType      = inputLine("Fruit type              : ");
            s.unitPrice      = inputFloat("Unit price (LKR/kg)    : ");
            s.quantity       = inputInt("Quantity (kg)           : ");
            s.supplyDate     = inputLine("Supply date             : ");
            s.freshnessScore = inputFloat("Freshness score (0-10) : ");
 
            if (!priceOk(s.fruitType, s.unitPrice)) {
                cout << RED << "Your price is too high for " << s.fruitType
                     << ". Max accepted: " << fruitPriceLimit[s.fruitType]
                     << " LKR/kg. Supply rejected.\n" << RESET;
                s.unitPrice = 0;
                supplierDB[id] = s;
                saveAllJSON();
                continue;
            }
 
            int deli = inputInt("Do you do delivery? (1=Yes / 0=No): ");
            s.doesDelivery = (deli == 1);
 
            if (!s.doesDelivery) {
                float charge = s.unitPrice * s.quantity * 0.05f;
                cout << YELLOW << "Pickup charge (5%): " << fixed << setprecision(2)
                     << charge << " LKR\n" << RESET;
                int agree = inputInt("Do you agree? (1=Yes / 0=No): ");
 
                if (agree == 1) {
                    cout << GREEN << "Alert sent to manager. Pickup date will be arranged.\n" << RESET;
                    addInventory(s.fruitType, s.quantity, s.supplyDate);
                } else {
                    cout << YELLOW << "Comparing with other suppliers...\n" << RESET;
                    float myScore    = supplierScore(s);
                    bool  betterFound = false;
 
                    for (auto& [oid, other] : supplierDB) {
                        if (oid == id || other.fruitType != s.fruitType) continue;
                        if (supplierScore(other) > myScore) {
                            betterFound = true;
                            cout << "  Better option found: " << other.name
                                 << " (Score: " << fixed << setprecision(3)
                                 << supplierScore(other) << ")\n";
                        }
                    }
 
                    if (!betterFound) {
                        cout << GREEN << "No better supplier found. System will use your supply.\n" << RESET;
                        addInventory(s.fruitType, s.quantity, s.supplyDate);
                    } else {
                        cout << RED << "System selected a better supplier automatically.\n" << RESET;
                    }
                }
            } else {
                cout << GREEN << "Delivery confirmed. Supply saved.\n" << RESET;
                addInventory(s.fruitType, s.quantity, s.supplyDate);
            }
 
            supplierDB[id] = s;
            saveAllJSON();
 
        } else if (ch == 2) {
            showSupplierRanking();
 
        } else if (ch == 3) {
            string old = inputLine("Current password: ");
            if (old != s.password) {
                cout << RED << "Wrong password.\n" << RESET;
            } else {
                string nw = inputLine("New password: ");
                s.password = nw;
                supplierDB[id] = s;
                saveAllJSON();
                cout << GREEN << "Password changed.\n" << RESET;
            }
        }
 
    } while (ch != 0);
}
 
// ======================================================
//  2. SELLER PORTAL
// ======================================================
 
void sellerSignIn() {
    cout << BOLD << "\n=== Seller Sign In ===\n" << RESET;
 
    Seller s;
    s.id = makeSellerID();
 
    s.name        = inputLine("Full Name                   : ");
    s.phone       = inputLine("Phone Number                : ");
    s.distance    = inputFloat("Distance from factory (km) : ");
    s.productType = inputLine("Product type                : ");
    s.password    = inputLine("Password                    : ");
    string confirm = inputLine("Confirm Password            : ");
 
    if (s.password != confirm) {
        cout << RED << "Passwords do not match. Try again.\n" << RESET;
        return;
    }
 
    s.quantity       = 0;
    s.customerRating = 0;
    s.yearlySales    = 0;
 
    sellerDB[s.id] = s;
    cout << GREEN << "\nRegistered successfully!\n"
         << "Your Seller ID : " << s.id << "\n" << RESET;
}
 
void sellerLogin() {
    cout << BOLD << "\n=== Seller Login ===\n" << RESET;
 
    string id = inputLine("Seller ID : ");
    string pw  = inputLine("Password  : ");
 
    if (sellerDB.find(id) == sellerDB.end() || sellerDB[id].password != pw) {
        cout << RED << "Invalid ID or password.\n" << RESET;
        return;
    }
 
    Seller& s = sellerDB[id];
    cout << GREEN << "Welcome, " << s.name << "!\n" << RESET;
 
    int ch;
    do {
        cout << CYAN << "\n--- Your Account (" << s.id << ") ---\n" << RESET;
        cout << "Name           : " << s.name           << "\n";
        cout << "Phone          : " << s.phone          << "\n";
        cout << "Product        : " << s.productType    << "\n";
        cout << "Quantity       : " << s.quantity       << "\n";
        cout << "Customer Rating: " << s.customerRating << "/10\n";
        cout << "Yearly Sales   : " << fixed << setprecision(2) << s.yearlySales << " LKR\n";
 
        cout << "\n1. Update product & quantity\n"
             << "2. Update customer rating\n"
             << "3. Update yearly sales\n"
             << "4. View seller ranking\n"
             << "5. Change password\n"
             << "0. Logout\n";
        ch = inputInt("Choice: ");
 
        if (ch == 1) {
            s.productType = inputLine("Product type       : ");
            s.quantity    = inputInt("Quantity           : ");
            float price   = inputFloat("Unit price (LKR)  : ");
            cout << YELLOW << "Total order price: "
                 << fixed << setprecision(2) << s.quantity * price << " LKR\n" << RESET;
            sellerDB[id] = s;
            cout << GREEN << "Updated.\n" << RESET;
 
        } else if (ch == 2) {
            s.customerRating = inputFloat("Customer rating (0-10): ");
            sellerDB[id] = s;
            cout << GREEN << "Rating updated.\n" << RESET;
 
        } else if (ch == 3) {
            s.yearlySales = inputFloat("Yearly sales (LKR): ");
            sellerDB[id] = s;
            cout << GREEN << "Sales updated.\n" << RESET;
 
        } else if (ch == 4) {
            showSellerRanking();
 
        } else if (ch == 5) {
            string old = inputLine("Current password: ");
            if (old != s.password) {
                cout << RED << "Wrong password.\n" << RESET;
            } else {
                string nw = inputLine("New password: ");
                s.password = nw;
                sellerDB[id] = s;
                cout << GREEN << "Password changed.\n" << RESET;
            }
        }
 
    } while (ch != 0);
}
 
// ======================================================
//  3. SUPPLIER MANAGER  (Default ID: SUP_MGR, password: manager123)
// ======================================================
 
void supplierManagerMenu() {
    if (!supplierMgrLoggedIn) {
        cout << BOLD << "\n=== Supplier Manager Login ===\n" << RESET;
        string id = inputLine("Manager ID: ");
        string pw = inputLine("Password: ");
        
        if (id != "SUP_MGR" || pw != "manager123") { 
            cout << RED << "Access denied.\n" << RESET; 
            return; 
        }
        supplierMgrLoggedIn = true;
        cout << GREEN << "Welcome, Supplier Manager!\n" << RESET;
    }
 
    int ch;
    do {
        cout << CYAN << "\n--- Supplier Manager Panel ---\n" << RESET;
        cout << "1. View all suppliers\n"
             << "2. Supplier ranking (Max-Heap)\n"
             << "3. View route map (Graph)\n"
             << "4. Find shortest route (Dijkstra's)\n"
             << "5. View inventory (FIFO Queue)\n"
             << "6. Process next inventory batch\n"
             << "7. Add route to map\n"
             << "8. Set fruit max price\n"
             << "9. Update supplier freshness score\n"
             << "10. Add new supplier manually\n"
             << "11. Remove supplier\n"
             << "12. View monthly financial summary\n"
             << "13. Export all data to JSON\n"
             << "0. Logout\n";
        ch = inputInt("Choice: ");
 
        if (ch == 1) {
            if (supplierDB.empty()) { cout << RED << "No suppliers yet.\n" << RESET; continue; }
            cout << YELLOW << "\n--- All Suppliers ---\n" << RESET;
            cout << left
                 << setw(10) << "ID"
                 << setw(16) << "Name"
                 << setw(14) << "Phone"
                 << setw(10) << "Dist"
                 << setw(12) << "Fruit"
                 << setw(10) << "Price"
                 << "Qty\n";
            cout << string(84, '-') << "\n";
            for (auto& [id, s] : supplierDB) {
                cout << left
                     << setw(10) << s.id
                     << setw(16) << s.name
                     << setw(14) << s.phone
                     << setw(10) << s.distance
                     << setw(12) << (s.fruitType.empty() ? "-" : s.fruitType)
                     << setw(10) << s.unitPrice
                     << s.quantity << "\n";
            }
 
        } else if (ch == 2) {
            showSupplierRanking();
 
        } else if (ch == 3) {
            showRouteMap();
 
        } else if (ch == 4) {
            string from = inputLine("From: ");
            string to   = inputLine("To  : ");
            findShortestRoute(from, to);
 
        } else if (ch == 5) {
            showInventory();
 
        } else if (ch == 6) {
            processInventory();
 
        } else if (ch == 7) {
            RouteEdge e;
            e.from     = inputLine("From location  : ");
            e.to       = inputLine("To location    : ");
            e.distance = inputFloat("Distance (km) : ");
            routeGraph.push_back(e);
            cout << GREEN << "Route added.\n" << RESET;
 
        } else if (ch == 8) {
            string fruit = inputLine("Fruit type       : ");
            float  maxP  = inputFloat("Max price LKR/kg: ");
            fruitPriceLimit[fruit] = maxP;
            cout << GREEN << "Price limit set.\n" << RESET;
 
        } else if (ch == 9) {
            string sid = inputLine("Supplier ID: ");
            if (supplierDB.find(sid) != supplierDB.end()) {
                float score = inputFloat("Freshness score (0-10): ");
                supplierDB[sid].freshnessScore = score;
                saveAllJSON();
                cout << GREEN << "Updated.\n" << RESET;
            } else {
                cout << RED << "Supplier not found.\n" << RESET;
            }
        } else if (ch == 10) {
            Supplier s;
            s.id = makeSupplierID();
            s.name = inputLine("Supplier Name: ");
            s.phone = inputLine("Phone: ");
            s.distance = inputFloat("Distance (km): ");
            s.password = inputLine("Password: ");
            s.fruitType = inputLine("Fruit Type: ");
            s.unitPrice = inputFloat("Unit Price: ");
            s.quantity = inputInt("Quantity: ");
            s.supplyDate = inputLine("Supply Date: ");
            s.freshnessScore = inputFloat("Freshness Score (0-10): ");
            int deli = inputInt("Does delivery? (1=Yes/0=No): ");
            s.doesDelivery = (deli == 1);
            supplierDB[s.id] = s;
            saveAllJSON();
            cout << GREEN << "Supplier added with ID: " << s.id << "\n" << RESET;
        } else if (ch == 11) {
            string sid = inputLine("Supplier ID to remove: ");
            if (supplierDB.erase(sid)) {
                saveAllJSON();
                cout << GREEN << "Supplier removed.\n" << RESET;
            } else {
                cout << RED << "Supplier not found.\n" << RESET;
            }
        } else if (ch == 12) {
            cout << YELLOW << "\n--- Monthly Financial Summary ---\n" << RESET;
            cout << "  Income  : " << fixed << setprecision(2) << monthlyIncome << " LKR\n";
            cout << "  Cost    : " << fixed << setprecision(2) << monthlyCost   << " LKR\n";
            float profit = monthlyIncome - monthlyCost;
            cout << (profit >= 0 ? GREEN : RED)
                 << "  Profit  : " << profit << " LKR\n" << RESET;
        } else if (ch == 13) {
            saveAllJSON();
            cout << GREEN << "Data exported to JSON.\n" << RESET;
        }
 
    } while (ch != 0);
    
    if (ch == 0) supplierMgrLoggedIn = false;
}
 
// ======================================================
//  4. SELLER MANAGER  (Default ID: SEL_MGR, password: sellmgr123)
// ======================================================
 
void sellerManagerMenu() {
    if (!sellerMgrLoggedIn) {
        cout << BOLD << "\n=== Seller Manager Login ===\n" << RESET;
        string id = inputLine("Manager ID: ");
        string pw = inputLine("Password: ");
        
        if (id != "SEL_MGR" || pw != "sellmgr123") { 
            cout << RED << "Access denied.\n" << RESET; 
            return; 
        }
        sellerMgrLoggedIn = true;
        cout << GREEN << "Welcome, Seller Manager!\n" << RESET;
    }
 
    int ch;
    do {
        cout << CYAN << "\n--- Seller Manager Panel ---\n" << RESET;
        cout << "1. View all sellers\n"
             << "2. Seller ranking (Max-Heap)\n"
             << "3. Customer ratings\n"
             << "4. View route map (Graph)\n"
             << "5. Find shortest route (Dijkstra's)\n"
             << "6. Update seller stock\n"
             << "7. Add new seller manually\n"
             << "8. Remove seller\n"
             << "9. View monthly financial summary\n"
             << "10. View inventory\n"
             << "11. Export all data to JSON\n"
             << "0. Logout\n";
        ch = inputInt("Choice: ");
 
        if (ch == 1) {
            if (sellerDB.empty()) { cout << RED << "No sellers yet.\n" << RESET; continue; }
            cout << YELLOW << "\n--- All Sellers ---\n" << RESET;
            cout << left
                 << setw(10) << "ID"
                 << setw(16) << "Name"
                 << setw(14) << "Product"
                 << setw(10) << "Qty"
                 << setw(10) << "Rating"
                 << "Yearly Sales\n";
            cout << string(72, '-') << "\n";
            for (auto& [id, s] : sellerDB) {
                cout << left
                     << setw(10) << s.id
                     << setw(16) << s.name
                     << setw(14) << s.productType
                     << setw(10) << s.quantity
                     << setw(10) << s.customerRating
                     << fixed << setprecision(2) << s.yearlySales << "\n";
            }
 
        } else if (ch == 2) {
            showSellerRanking();
 
        } else if (ch == 3) {
            cout << YELLOW << "\n--- Customer Ratings ---\n" << RESET;
            for (auto& [id, s] : sellerDB) {
                cout << "  " << left << setw(18) << s.name
                     << "(" << s.productType << "): "
                     << s.customerRating << "/10\n";
            }
 
        } else if (ch == 4) {
            showRouteMap();
 
        } else if (ch == 5) {
            string from = inputLine("From: ");
            string to   = inputLine("To  : ");
            findShortestRoute(from, to);
 
        } else if (ch == 6) {
            string sid = inputLine("Seller ID: ");
            if (sellerDB.find(sid) != sellerDB.end()) {
                sellerDB[sid].quantity = inputInt("New stock qty: ");
                cout << GREEN << "Stock updated.\n" << RESET;
            } else {
                cout << RED << "Seller not found.\n" << RESET;
            }
        } else if (ch == 7) {
            Seller s;
            s.id = makeSellerID();
            s.name = inputLine("Seller Name: ");
            s.phone = inputLine("Phone: ");
            s.distance = inputFloat("Distance (km): ");
            s.productType = inputLine("Product Type: ");
            s.password = inputLine("Password: ");
            s.quantity = inputInt("Initial Stock: ");
            s.customerRating = inputFloat("Customer Rating (0-10): ");
            s.yearlySales = inputFloat("Yearly Sales: ");
            sellerDB[s.id] = s;
            cout << GREEN << "Seller added with ID: " << s.id << "\n" << RESET;
        } else if (ch == 8) {
            string sid = inputLine("Seller ID to remove: ");
            if (sellerDB.erase(sid)) {
                cout << GREEN << "Seller removed.\n" << RESET;
            } else {
                cout << RED << "Seller not found.\n" << RESET;
            }
        } else if (ch == 9) {
            cout << YELLOW << "\n--- Monthly Financial Summary ---\n" << RESET;
            cout << "  Income  : " << fixed << setprecision(2) << monthlyIncome << " LKR\n";
            cout << "  Cost    : " << fixed << setprecision(2) << monthlyCost   << " LKR\n";
            float profit = monthlyIncome - monthlyCost;
            cout << (profit >= 0 ? GREEN : RED)
                 << "  Profit  : " << profit << " LKR\n" << RESET;
        } else if (ch == 10) {
            showInventory();
        } else if (ch == 11) {
            saveAllJSON();
            cout << GREEN << "Data exported to JSON.\n" << RESET;
        }
 
    } while (ch != 0);
    
    if (ch == 0) sellerMgrLoggedIn = false;
}
 
// ======================================================
//  5. PRODUCTION MANAGER  (Default ID: PRO_MGR, password: prod123)
// ======================================================
 
void productionManagerMenu() {
    if (!productionMgrLoggedIn) {
        cout << BOLD << "\n=== Production Manager Login ===\n" << RESET;
        string id = inputLine("Manager ID: ");
        string pw = inputLine("Password: ");
        
        if (id != "PRO_MGR" || pw != "prod123") { 
            cout << RED << "Access denied.\n" << RESET; 
            return; 
        }
        productionMgrLoggedIn = true;
        cout << GREEN << "Welcome, Production Manager!\n" << RESET;
    }
 
    int ch;
    do {
        cout << CYAN << "\n--- Production Manager Panel ---\n" << RESET;
        cout << "1. View production stock\n"
             << "2. Add / update production item\n"
             << "3. Update vehicle maintenance cost\n"
             << "4. View all suppliers\n"
             << "5. View all sellers\n"
             << "6. View inventory\n"
             << "7. Process inventory batch\n"
             << "8. Add route to map\n"
             << "9. Find shortest route\n"
             << "10. View monthly financial summary\n"
             << "11. Export all data to JSON\n"
             << "0. Logout\n";
        ch = inputInt("Choice: ");
 
        if (ch == 1) {
            if (productionStock.empty()) {
                cout << RED << "No production items yet.\n" << RESET;
            } else {
                cout << YELLOW << "\n--- Production Stock ---\n" << RESET;
                cout << left << setw(24) << "Item" << setw(10) << "Qty" << "Unit Price\n";
                cout << string(46, '-') << "\n";
                for (auto& item : productionStock) {
                    cout << left
                         << setw(24) << item.name
                         << setw(10) << item.quantity
                         << fixed << setprecision(2) << item.unitPrice << " LKR\n";
                }
            }
 
        } else if (ch == 2) {
            ProductionItem item;
            item.name      = inputLine("Item name (bottles/straws/labels/boxes/sugar/lemon): ");
            item.quantity  = inputInt("Quantity  : ");
            item.unitPrice = inputFloat("Unit price (LKR): ");
 
            bool found = false;
            for (auto& p : productionStock) {
                if (p.name == item.name) {
                    p     = item;
                    found = true;
                    cout << GREEN << "Item updated.\n" << RESET;
                    break;
                }
            }
            if (!found) {
                productionStock.push_back(item);
                cout << GREEN << "Item added.\n" << RESET;
            }
 
        } else if (ch == 3) {
            vehicleMaintenanceCost = inputFloat("Vehicle maintenance cost (LKR): ");
            monthlyCost += vehicleMaintenanceCost;
            cout << GREEN << "Updated: "
                 << fixed << setprecision(2) << vehicleMaintenanceCost << " LKR\n" << RESET;
        } else if (ch == 4) {
            if (supplierDB.empty()) { cout << RED << "No suppliers yet.\n" << RESET; continue; }
            cout << YELLOW << "\n--- All Suppliers ---\n" << RESET;
            for (auto& [id, s] : supplierDB) {
                cout << "  " << s.id << " | " << s.name << " | " << s.fruitType 
                     << " | " << s.unitPrice << " LKR/kg\n";
            }
        } else if (ch == 5) {
            if (sellerDB.empty()) { cout << RED << "No sellers yet.\n" << RESET; continue; }
            cout << YELLOW << "\n--- All Sellers ---\n" << RESET;
            for (auto& [id, s] : sellerDB) {
                cout << "  " << s.id << " | " << s.name << " | " << s.productType 
                     << " | Stock: " << s.quantity << "\n";
            }
        } else if (ch == 6) {
            showInventory();
        } else if (ch == 7) {
            processInventory();
        } else if (ch == 8) {
            RouteEdge e;
            e.from     = inputLine("From location  : ");
            e.to       = inputLine("To location    : ");
            e.distance = inputFloat("Distance (km) : ");
            routeGraph.push_back(e);
            cout << GREEN << "Route added.\n" << RESET;
        } else if (ch == 9) {
            string from = inputLine("From: ");
            string to   = inputLine("To  : ");
            findShortestRoute(from, to);
        } else if (ch == 10) {
            cout << YELLOW << "\n--- Monthly Financial Summary ---\n" << RESET;
            cout << "  Income  : " << fixed << setprecision(2) << monthlyIncome << " LKR\n";
            cout << "  Cost    : " << fixed << setprecision(2) << monthlyCost   << " LKR\n";
            float profit = monthlyIncome - monthlyCost;
            cout << (profit >= 0 ? GREEN : RED)
                 << "  Profit  : " << profit << " LKR\n" << RESET;
        } else if (ch == 11) {
            saveAllJSON();
            cout << GREEN << "Data exported to JSON.\n" << RESET;
        }
 
    } while (ch != 0);
    
    if (ch == 0) productionMgrLoggedIn = false;
}
 
// ======================================================
//  6. ADMIN  (Default ID: ADMIN, password: admin123)
// ======================================================
 
void adminMenu() {
    if (!adminLoggedIn) {
        cout << BOLD << "\n=== Admin Login ===\n" << RESET;
        string id = inputLine("Admin ID: ");
        string pw = inputLine("Password: ");
        
        if (id != "ADMIN" || pw != "admin123") { 
            cout << RED << "Access denied.\n" << RESET; 
            return; 
        }
        adminLoggedIn = true;
        cout << GREEN << "Welcome, Administrator!\n" << RESET;
    }
 
    int ch;
    do {
        cout << CYAN << "\n--- Admin Panel ---\n" << RESET;
        cout << "1. Monthly income & cost summary\n"
             << "2. Update monthly income\n"
             << "3. Update monthly cost\n"
             << "4. All supplier ranks\n"
             << "5. All seller ranks\n"
             << "6. All customer ratings\n"
             << "7. Inventory stock\n"
             << "8. Production stock\n"
             << "9. View all routes\n"
             << "10. Add new route\n"
             << "11. Find shortest route\n"
             << "12. Force export JSON files\n"
             << "13. View all suppliers\n"
             << "14. View all sellers\n"
             << "15. Update vehicle maintenance cost\n"
             << "0. Logout\n";
        ch = inputInt("Choice: ");
 
        if (ch == 1) {
            cout << YELLOW << "\n--- Monthly Financial Summary ---\n" << RESET;
            cout << "  Income  : " << fixed << setprecision(2) << monthlyIncome << " LKR\n";
            cout << "  Cost    : " << fixed << setprecision(2) << monthlyCost   << " LKR\n";
            float profit = monthlyIncome - monthlyCost;
            cout << (profit >= 0 ? GREEN : RED)
                 << "  Profit  : " << profit << " LKR\n" << RESET;
 
        } else if (ch == 2) {
            monthlyIncome = inputFloat("Monthly income (LKR): ");
            cout << GREEN << "Updated.\n" << RESET;
 
        } else if (ch == 3) {
            monthlyCost = inputFloat("Monthly cost (LKR): ");
            cout << GREEN << "Updated.\n" << RESET;
 
        } else if (ch == 4) {
            showSupplierRanking();
 
        } else if (ch == 5) {
            showSellerRanking();
 
        } else if (ch == 6) {
            cout << YELLOW << "\n--- Customer Ratings ---\n" << RESET;
            for (auto& [id, s] : sellerDB) {
                cout << "  " << left << setw(18) << s.name
                     << "(" << s.productType << "): "
                     << s.customerRating << "/10\n";
            }
 
        } else if (ch == 7) {
            showInventory();
 
        } else if (ch == 8) {
            cout << YELLOW << "\n--- Production Stock ---\n" << RESET;
            if (productionStock.empty()) {
                cout << "  No production items.\n";
            } else {
                for (auto& item : productionStock) {
                    cout << "  " << left << setw(24) << item.name
                         << "Qty: " << item.quantity
                         << "  Price: " << fixed << setprecision(2) << item.unitPrice << " LKR\n";
                }
            }
        } else if (ch == 9) {
            showRouteMap();
        } else if (ch == 10) {
            RouteEdge e;
            e.from     = inputLine("From location  : ");
            e.to       = inputLine("To location    : ");
            e.distance = inputFloat("Distance (km) : ");
            routeGraph.push_back(e);
            cout << GREEN << "Route added.\n" << RESET;
        } else if (ch == 11) {
            string from = inputLine("From: ");
            string to   = inputLine("To  : ");
            findShortestRoute(from, to);
        } else if (ch == 12) {
            saveAllJSON();
            cout << GREEN << "Both JSON files force-exported.\n" << RESET;
        } else if (ch == 13) {
            if (supplierDB.empty()) { cout << RED << "No suppliers yet.\n" << RESET; continue; }
            cout << YELLOW << "\n--- All Suppliers ---\n" << RESET;
            for (auto& [id, s] : supplierDB) {
                cout << "  " << s.id << " | " << s.name << " | " << s.phone 
                     << " | " << s.fruitType << " | " << s.unitPrice << " LKR/kg\n";
            }
        } else if (ch == 14) {
            if (sellerDB.empty()) { cout << RED << "No sellers yet.\n" << RESET; continue; }
            cout << YELLOW << "\n--- All Sellers ---\n" << RESET;
            for (auto& [id, s] : sellerDB) {
                cout << "  " << s.id << " | " << s.name << " | " << s.phone 
                     << " | " << s.productType << " | Stock: " << s.quantity << "\n";
            }
        } else if (ch == 15) {
            vehicleMaintenanceCost = inputFloat("Vehicle maintenance cost (LKR): ");
            monthlyCost += vehicleMaintenanceCost;
            cout << GREEN << "Updated.\n" << RESET;
        }
 
    } while (ch != 0);
    
    if (ch == 0) adminLoggedIn = false;
}
 
// ======================================================
//  MAIN
// ======================================================
 
int main() {
 
    // Default fruit price limits
    fruitPriceLimit["Mango"]     = 150.0f;
    fruitPriceLimit["Pineapple"] = 120.0f;
    fruitPriceLimit["Papaya"]    = 100.0f;
    fruitPriceLimit["Orange"]    = 130.0f;
 
    // Sample route graph
    routeGraph.push_back({"Factory", "Galle",   20.0f});
    routeGraph.push_back({"Factory", "Matara",  35.0f});
    routeGraph.push_back({"Galle",   "Matara",  18.0f});
    routeGraph.push_back({"Factory", "Colombo", 10.0f});
    routeGraph.push_back({"Colombo", "Kandy",  115.0f});
 
    // Export initial empty JSON so Vue doesn't get 404
    saveAllJSON();
 
    /*
    =================== DEFAULT LOGIN CREDENTIALS ===================
      Admin            : ID: ADMIN      | Password: admin123
      Supplier Manager : ID: SUP_MGR    | Password: manager123
      Seller Manager   : ID: SEL_MGR    | Password: sellmgr123
      Production Mgr   : ID: PRO_MGR    | Password: prod123
    =================================================================
    */
 
    int choice;
    do {
        cout << BOLD
             << "\n+-----------------------------------------------+\n"
             << "|  Smart Fruit Supply Chain Optimization System |\n"
             << "+-----------------------------------------------+\n" << RESET;
        cout << "  1. Supplier  - Sign In (New)\n"
             << "  2. Supplier  - Login\n"
             << "  3. Seller    - Sign In (New)\n"
             << "  4. Seller    - Login\n"
             << "  5. Supplier Manager\n"
             << "  6. Seller Manager\n"
             << "  7. Production Manager\n"
             << "  8. Admin\n"
             << "  0. Exit\n";
        choice = inputInt("Choice: ");
 
        if      (choice == 1) supplierSignIn();
        else if (choice == 2) supplierLogin();
        else if (choice == 3) sellerSignIn();
        else if (choice == 4) sellerLogin();
        else if (choice == 5) supplierManagerMenu();
        else if (choice == 6) sellerManagerMenu();
        else if (choice == 7) productionManagerMenu();
        else if (choice == 8) adminMenu();
        else if (choice != 0) cout << RED << "Invalid choice.\n" << RESET;
 
    } while (choice != 0);
 
    cout << CYAN << "\nGoodbye!\n" << RESET;
    return 0;
}