#include <iostream>
#include <fstream>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <sstream>
#include <filesystem>
#include <vector>
#include <algorithm>
#include <functional>

namespace fs = std::filesystem;

using namespace std;

string extractRootNamespaces(const string& content) {
    size_t start = content.find('<');
    if (start == string::npos) return "";
    
    size_t end = content.find('>', start);
    if (end == string::npos) return "";
    
    string rootTag = content.substr(start, end - start + 1);
    size_t xmlns_pos = rootTag.find("xmlns:");
    if (xmlns_pos == string::npos) return "";
    
    return rootTag.substr(xmlns_pos, rootTag.size() - xmlns_pos - 1);
}

string extractFirstThreeLines(const string& section) {
    istringstream iss(section);
    string line;
    string result;
    int count = 0;
    
    while (getline(iss, line)) {
        size_t first = line.find_first_not_of(" \t");
        if (first == string::npos) continue;
        size_t last = line.find_last_not_of(" \t");
        string trimmed = line.substr(first, last - first + 1);
        
        if (!trimmed.empty()) {
            result += trimmed + "\n";
            if (++count == 3) break;
        }
    }
    return result;
}

string extractFullSection(const string& tag, const string& content, size_t start_pos) {
    string openTag = "<" + tag + ">";
    string closeTag = "</" + tag + ">";
    
    size_t start = content.find(openTag, start_pos);
    if (start == string::npos) return "";
    
    size_t pos = start + openTag.length();
    int depth = 1;
    
    while (depth > 0 && pos < content.size()) {
        size_t next_open = content.find(openTag, pos);
        size_t next_close = content.find(closeTag, pos);
        
        if (next_close == string::npos) break;
        
        if (next_open != string::npos && next_open < next_close) {
            depth++;
            pos = next_open + openTag.length();
        } else {
            depth--;
            pos = next_close + closeTag.length();
        }
    }
    return (depth == 0) ? content.substr(start, pos - start) : "";
}

void processVehicleRacesFile(const string& baseFile, const string& modFile) {
    size_t underscore_pos = modFile.find('_');
    if (underscore_pos == string::npos) {
        cerr << "Invalid mod filename format: " << modFile << endl;
        return;
    }
    
    string outputFile = modFile.substr(0, underscore_pos) + "+" + modFile.substr(underscore_pos);
    
    ifstream inBase(baseFile);
    ifstream inMod(modFile);
    ofstream out(outputFile);
    
    if (!inBase || !inMod || !out) {
        cerr << "Error opening files!" << endl;
        return;
    }
    
    string contentBase((istreambuf_iterator<char>(inBase)), istreambuf_iterator<char>());
    string contentMod((istreambuf_iterator<char>(inMod)), istreambuf_iterator<char>());
    
    unordered_set<string> baseRaceTrackIDs;
    
    size_t track_pos = 0;
    while (true) {
        string trackSection = extractFullSection("VehicleRaceTrack", contentBase, track_pos);
        if (trackSection.empty()) break;
        
        size_t id_start = trackSection.find("<ID>");
        size_t id_end = trackSection.find("</ID>");
        if (id_start != string::npos && id_end != string::npos) {
            string id = trackSection.substr(id_start + 4, id_end - (id_start + 4));
            baseRaceTrackIDs.insert(id);
        }
        
        track_pos += trackSection.length();
    }
    
    string namespaces = extractRootNamespaces(contentMod);
    out << "<VehicleRaceTypeManager";
    if (!namespaces.empty()) out << " " << namespaces;
    out << ">\n";
    
    out << "<VehicleRaceTracks>\n";
    int totalKept = 0;
    track_pos = 0;
    
    while (true) {
        string trackSection = extractFullSection("VehicleRaceTrack", contentMod, track_pos);
        if (trackSection.empty()) break;
        
        size_t id_start = trackSection.find("<ID>");
        size_t id_end = trackSection.find("</ID>");
        if (id_start != string::npos && id_end != string::npos) {
            string id = trackSection.substr(id_start + 4, id_end - (id_start + 4));
            
            if (baseRaceTrackIDs.find(id) == baseRaceTrackIDs.end()) {
                out << trackSection << "\n";
                totalKept++;
                baseRaceTrackIDs.insert(id);
            }
        }
        
        track_pos += trackSection.length();
    }
    
    out << "</VehicleRaceTracks>\n";
    out << "</VehicleRaceTypeManager>";
    cout << "Created " << outputFile << " with " << totalKept << " unique VehicleRaceTrack entries" << endl;
}

void processSpawnBlocksFile(const string& baseFile, const string& modFile) {
    size_t underscore_pos = modFile.find('_');
    if (underscore_pos == string::npos) {
        cerr << "Invalid mod filename format: " << modFile << endl;
        return;
    }
    
    string outputFile = modFile.substr(0, underscore_pos) + "+" + modFile.substr(underscore_pos);
    
    ifstream inBase(baseFile);
    ifstream inMod(modFile);
    ofstream out(outputFile);
    
    if (!inBase || !inMod || !out) {
        cerr << "Error opening files!" << endl;
        return;
    }
    
    string contentBase((istreambuf_iterator<char>(inBase)), istreambuf_iterator<char>());
    string contentMod((istreambuf_iterator<char>(inMod)), istreambuf_iterator<char>());
    
    unordered_set<string> baseCarGenerators;
    unordered_set<string> baseScenarios;
    
    size_t carGen_pos = 0;
    while (true) {
        string carGenSection = extractFullSection("CarGeneratorBlock", contentBase, carGen_pos);
        if (carGenSection.empty()) break;
        
        string firstThree = extractFirstThreeLines(carGenSection);
        if (!firstThree.empty()) {
            baseCarGenerators.insert(firstThree);
        }
        carGen_pos += carGenSection.length();
    }
    
    size_t scenario_pos = 0;
    while (true) {
        string scenarioSection = extractFullSection("ScenarioBlock", contentBase, scenario_pos);
        if (scenarioSection.empty()) break;
        
        string firstThree = extractFirstThreeLines(scenarioSection);
        if (!firstThree.empty()) {
            baseScenarios.insert(firstThree);
        }
        scenario_pos += scenarioSection.length();
    }
    
    string namespaces = extractRootNamespaces(contentMod);
    out << "<PossibleSpawnBlocks";
    if (!namespaces.empty()) out << " " << namespaces;
    out << ">\n";
    
    int totalKept = 0;
    
    out << "<CarGeneratorBlock>\n";
    carGen_pos = 0;
    int keptCarGens = 0;
    while (true) {
        string carGenSection = extractFullSection("CarGeneratorBlock", contentMod, carGen_pos);
        if (carGenSection.empty()) break;
        
        string firstThree = extractFirstThreeLines(carGenSection);
        
        if (baseCarGenerators.find(firstThree) == baseCarGenerators.end()) {
            out << carGenSection << "\n";
            keptCarGens++;
            totalKept++;
        }
        carGen_pos += carGenSection.length();
    }
    out << "</CarGeneratorBlock>\n";
    if (keptCarGens > 0) {
        cout << "  Kept " << keptCarGens << " unique CarGeneratorBlock entries" << endl;
    }
    
    out << "<ScenarioBlocks>\n";
    scenario_pos = 0;
    int keptScenarios = 0;
    while (true) {
        string scenarioSection = extractFullSection("ScenarioBlock", contentMod, scenario_pos);
        if (scenarioSection.empty()) break;
        
        string firstThree = extractFirstThreeLines(scenarioSection);
        
        if (baseScenarios.find(firstThree) == baseScenarios.end()) {
            out << scenarioSection << "\n";
            keptScenarios++;
            totalKept++;
        }
        scenario_pos += scenarioSection.length();
    }
    out << "</ScenarioBlocks>\n";
    if (keptScenarios > 0) {
        cout << "  Kept " << keptScenarios << " unique ScenarioBlock entries" << endl;
    }
    
    out << "</PossibleSpawnBlocks>";
    cout << "Created " << outputFile << " with " << totalKept << " unique spawn block entries" << endl;
}

void processNamesFile(const string& baseFile, const string& modFile) {
    size_t underscore_pos = modFile.find('_');
    if (underscore_pos == string::npos) {
        cerr << "Invalid mod filename format: " << modFile << endl;
        return;
    }
    
    string outputFile = modFile.substr(0, underscore_pos) + "+" + modFile.substr(underscore_pos);
    
    ifstream inBase(baseFile);
    ifstream inMod(modFile);
    ofstream out(outputFile);
    
    if (!inBase || !inMod || !out) {
        cerr << "Error opening files!" << endl;
        return;
    }
    
    string contentBase((istreambuf_iterator<char>(inBase)), istreambuf_iterator<char>());
    string contentMod((istreambuf_iterator<char>(inMod)), istreambuf_iterator<char>());
    
    unordered_set<string> baseNames;
    vector<string> nameCategories = {"MaleNames", "FemaleNames", "UnisexNames", "LastNames"};
    
    for (const auto& category : nameCategories) {
        size_t cat_pos = contentBase.find("<" + category + ">");
        if (cat_pos == string::npos) continue;
        
        size_t end_pos = contentBase.find("</" + category + ">", cat_pos);
        if (end_pos == string::npos) continue;
        
        string categoryContent = contentBase.substr(cat_pos, end_pos - cat_pos + category.length() + 3);
        
        size_t name_pos = 0;
        while (true) {
            size_t start = categoryContent.find("<string>", name_pos);
            if (start == string::npos) break;
            
            size_t end = categoryContent.find("</string>", start);
            if (end == string::npos) break;
            
            string name = categoryContent.substr(start + 8, end - (start + 8));
            baseNames.insert(name);
            
            name_pos = end + 9;
        }
    }
    
    string namespaces = extractRootNamespaces(contentMod);
    out << "<PossibleNames";
    if (!namespaces.empty()) out << " " << namespaces;
    out << ">\n";
    
    int totalKept = 0;
    
    for (const auto& category : nameCategories) {
        size_t cat_pos = contentMod.find("<" + category + ">");
        if (cat_pos == string::npos) continue;
        
        size_t end_pos = contentMod.find("</" + category + ">", cat_pos);
        if (end_pos == string::npos) continue;
        
        string categoryContent = contentMod.substr(cat_pos, end_pos - cat_pos + category.length() + 3);
        
        out << "<" << category << ">\n";
        int keptInCategory = 0;
        
        size_t name_pos = 0;
        while (true) {
            size_t start = categoryContent.find("<string>", name_pos);
            if (start == string::npos) break;
            
            size_t end = categoryContent.find("</string>", start);
            if (end == string::npos) break;
            
            string name = categoryContent.substr(start + 8, end - (start + 8));
            
            if (baseNames.find(name) == baseNames.end()) {
                out << "    <string>" << name << "</string>\n";
                keptInCategory++;
                totalKept++;
            }
            
            name_pos = end + 9;
        }
        
        out << "</" << category << ">\n";
        if (keptInCategory > 0) {
            cout << "  Kept " << keptInCategory << " unique names in " << category << endl;
        }
    }
    
    out << "</PossibleNames>";
    cout << "Created " << outputFile << " with " << totalKept << " unique name entries" << endl;
}

void processOrganizationsFile(const string& baseFile, const string& modFile) {
    size_t underscore_pos = modFile.find('_');
    if (underscore_pos == string::npos) {
        cerr << "Invalid mod filename format: " << modFile << endl;
        return;
    }
    
    string outputFile = modFile.substr(0, underscore_pos) + "+" + modFile.substr(underscore_pos);
    
    ifstream inBase(baseFile);
    ifstream inMod(modFile);
    ofstream out(outputFile);
    
    if (!inBase || !inMod || !out) {
        cerr << "Error opening files!" << endl;
        return;
    }
    
    string contentBase((istreambuf_iterator<char>(inBase)), istreambuf_iterator<char>());
    string contentMod((istreambuf_iterator<char>(inMod)), istreambuf_iterator<char>());

    unordered_set<string> baseOrgIDs;
    vector<string> orgTypes = {"GeneralOrganizations", "TaxiFirms"};
    vector<string> orgTags = {"Organization", "TaxiFirm"};
    
    for (size_t i = 0; i < orgTypes.size(); i++) {
        size_t container_pos = 0;
        while (true) {
            string container = extractFullSection(orgTypes[i], contentBase, container_pos);
            if (container.empty()) break;
            
            size_t org_pos = 0;
            while (true) {
                string orgSection = extractFullSection(orgTags[i], container, org_pos);
                if (orgSection.empty()) break;
                
                size_t id_start = orgSection.find("<ID>");
                size_t id_end = orgSection.find("</ID>");
                if (id_start != string::npos && id_end != string::npos) {
                    string id = orgSection.substr(id_start + 4, id_end - (id_start + 4));
                    baseOrgIDs.insert(id);
                }
                
                org_pos = container.find("<" + orgTags[i] + ">", org_pos + 1);
                if (org_pos == string::npos) break;
            }
            container_pos = contentBase.find("<" + orgTypes[i] + ">", container_pos + 1);
        }
    }
    
     string namespaces = extractRootNamespaces(contentMod);
    out << "<PossibleOrganizations";
    if (!namespaces.empty()) out << " " << namespaces;
    out << ">\n";
    int totalKept = 0;
    
    for (size_t i = 0; i < orgTypes.size(); i++) {
        out << "<" << orgTypes[i] << ">\n";
        int keptCount = 0;
        
        size_t container_pos = 0;
        while (true) {
            string container = extractFullSection(orgTypes[i], contentMod, container_pos);
            if (container.empty()) break;
            
            size_t org_pos = 0;
            while (true) {
                string orgSection = extractFullSection(orgTags[i], container, org_pos);
                if (orgSection.empty()) break;
                
                size_t id_start = orgSection.find("<ID>");
                size_t id_end = orgSection.find("</ID>");
                if (id_start != string::npos && id_end != string::npos) {
                    string id = orgSection.substr(id_start + 4, id_end - (id_start + 4));
                    
                    if (baseOrgIDs.find(id) == baseOrgIDs.end()) {
                        out << orgSection << "\n";
                        keptCount++;
                        totalKept++;
                        baseOrgIDs.insert(id); 
                    }
                }
                
                org_pos = container.find("<" + orgTags[i] + ">", org_pos + 1);
                if (org_pos == string::npos) break;
            }
            container_pos = contentMod.find("<" + orgTypes[i] + ">", container_pos + 1);
        }
        
        out << "</" << orgTypes[i] << ">\n";
        if (keptCount > 0) {
            cout << "  Kept " << keptCount << " unique " << orgTags[i] << " entries" << endl;
        }
    }
    
    out << "</PossibleOrganizations>";
    cout << "Created " << outputFile << " with " << totalKept << " unique organization entries" << endl;
}

void processLocationsFile(const string& baseFile, const string& modFile) {
    size_t underscore_pos = modFile.find('_');
    if (underscore_pos == string::npos) {
        cerr << "Invalid mod filename format: " << modFile << endl;
        return;
    }
    
    string outputFile = modFile.substr(0, underscore_pos) + "+" + modFile.substr(underscore_pos);
    
    ifstream inBase(baseFile);
    ifstream inMod(modFile);
    ofstream out(outputFile);
    
    if (!inBase || !inMod || !out) {
        cerr << "Error opening files!" << endl;
        return;
    }
    
    string contentBase((istreambuf_iterator<char>(inBase)), istreambuf_iterator<char>());
    string contentMod((istreambuf_iterator<char>(inMod)), istreambuf_iterator<char>());
    
    vector<string> locationTypes = {
        "DeadDrop", "ScrapYard", "BodyExport", "CarCrusher", "GangDen", "GunStore", 
        "Hotel", "ApartmentBuilding", "Residence", "CityHall", "VendingMachine", 
        "PoliceStation", "Hospital", "FireStation", "Restaurant", "Business", 
        "Pharmacy", "Dispensary", "HeadShop", "HardwareStore", "PawnShop", 
        "Landmark", "Bank", "ConvenienceStore", "GasStation", "LiquorStore", 
        "FoodStand", "Dealership", "VehicleExporter", "Forger", "GamblingDen", 
        "RepairGarage", "Bar", "DriveThru", "ClothingShop", "BusStop", 
        "SubwayStation", "Prison", "Morgue", "SportingGoodsStore", "Airport", 
        "IllicitMarketplace", "BlankLocation", "MilitaryBase", "ExteriorCraftingLocation", 
        "BarberShop", "PlasticSurgeryClinic", "TattooShop", "RaceMeetup", 
        "ATMMachine", "GasPump", "CashRegister", "StoredSpawn", "PedCustomizerLocation"
    };
    
    unordered_map<string, string> containerTags = {
        {"DeadDrop", "DeadDrops"},
        {"ScrapYard", "ScrapYards"},
        {"BodyExport", "BodyExports"},
        {"CarCrusher", "CarCrushers"},
        {"GangDen", "GangDens"},
        {"GunStore", "GunStores"},
        {"Hotel", "Hotels"},
        {"ApartmentBuilding", "ApartmentBuildings"},
        {"Residence", "Residences"},
        {"CityHall", "CityHalls"},
        {"VendingMachine", "VendingMachines"},
        {"PoliceStation", "PoliceStations"},
        {"Hospital", "Hospitals"},
        {"FireStation", "FireStations"},
        {"Restaurant", "Restaurants"},
        {"Business", "Businesses"},
        {"Pharmacy", "Pharmacies"},
        {"Dispensary", "Dispensaries"},
        {"HeadShop", "HeadShops"},
        {"HardwareStore", "HardwareStores"},
        {"PawnShop", "PawnShops"},
        {"Landmark", "Landmarks"},
        {"Bank", "Banks"},
        {"ConvenienceStore", "ConvenienceStores"},
        {"GasStation", "GasStations"},
        {"LiquorStore", "LiquorStores"},
        {"FoodStand", "FoodStands"},
        {"Dealership", "CarDealerships"},
        {"VehicleExporter", "VehicleExporters"},
        {"Forger", "Forgers"},
        {"GamblingDen", "GamblingDens"},
        {"RepairGarage", "RepairGarages"},
        {"Bar", "Bars"},
        {"DriveThru", "DriveThrus"},
        {"ClothingShop", "ClothingShops"},
        {"BusStop", "BusStops"},
        {"SubwayStation", "SubwayStations"},
        {"Prison", "Prisons"},
        {"Morgue", "Morgues"},
        {"SportingGoodsStore", "SportingGoodsStores"},
        {"Airport", "Airports"},
        {"IllicitMarketplace", "IllicitMarketplaces"},
        {"BlankLocation", "BlankLocations"},
        {"MilitaryBase", "MilitaryBases"},
        {"ExteriorCraftingLocation", "ExteriorCraftingLocations"},
        {"BarberShop", "BarberShops"},
        {"PlasticSurgeryClinic", "PlasticSurgeryClinics"},
        {"TattooShop", "TattooShops"},
        {"RaceMeetup", "RaceMeetups"},
        {"ATMMachine", "ATMMachines"},
        {"GasPump", "GasPumps"},
        {"CashRegister", "CashRegisters"},
        {"StoredSpawn", "StoredSpawns"},
        {"PedCustomizerLocation", "PedCustomizerLocations"}
    };
    
    unordered_map<string, unordered_set<string>> baseLocations;
    
    for (const auto& locType : locationTypes) {
        string containerTag = containerTags[locType];
        size_t container_pos = 0;
        
        while (true) {
            string container = extractFullSection(containerTag, contentBase, container_pos);
            if (container.empty()) break;
            
            size_t loc_pos = 0;
            while (true) {
                string section = extractFullSection(locType, container, loc_pos);
                if (section.empty()) break;
                
                string firstThree = extractFirstThreeLines(section);
                if (!firstThree.empty()) {
                    baseLocations[locType].insert(firstThree);
                }
                loc_pos = container.find("<" + locType + ">", loc_pos + 1);
                if (loc_pos == string::npos) break;
            }
            container_pos = contentBase.find("<" + containerTag + ">", container_pos + 1);
        }
    }
    
    string namespaces = extractRootNamespaces(contentMod);
    out << "<PossibleLocations";
    if (!namespaces.empty()) out << " " << namespaces;
    out << ">\n";
    int totalKept = 0;
    unordered_set<string> processedSections;
    
    for (const auto& locType : locationTypes) {
        const string& containerTag = containerTags[locType];
        int keptCount = 0;
        
        out << "<" << containerTag << ">\n";
        
        size_t container_pos = 0;
        while (true) {
            string container = extractFullSection(containerTag, contentMod, container_pos);
            if (container.empty()) break;
            
            size_t loc_pos = 0;
            while (true) {
                string section = extractFullSection(locType, container, loc_pos);
                if (section.empty()) break;
                
                string firstThree = extractFirstThreeLines(section);
                string sectionHash = to_string(hash<string>{}(section));
                
                if (baseLocations[locType].find(firstThree) == baseLocations[locType].end() &&
                    processedSections.find(sectionHash) == processedSections.end()) {
                    out << section << "\n";
                    keptCount++;
                    totalKept++;
                    processedSections.insert(sectionHash);
                }
                loc_pos = container.find("<" + locType + ">", loc_pos + 1);
                if (loc_pos == string::npos) break;
            }
            container_pos = contentMod.find("<" + containerTag + ">", container_pos + 1);
        }
        
        out << "</" << containerTag << ">\n";
        if (keptCount > 0) {
            cout << "  Kept " << keptCount << " unique " << locType << " entries" << endl;
        }
    }
    
    out << "</PossibleLocations>";
    cout << "Created " << outputFile << " with " << totalKept << " unique location entries" << endl;
}

void processIssuableWeaponsFile(const string& baseFile, const string& modFile) {
    size_t underscore_pos = modFile.find('_');
    if (underscore_pos == string::npos) {
        cerr << "Invalid mod filename format: " << modFile << endl;
        return;
    }
    
    string outputFile = modFile.substr(0, underscore_pos) + "+" + modFile.substr(underscore_pos);
    
    ifstream inBase(baseFile);
    ifstream inMod(modFile);
    ofstream out(outputFile);
    
    if (!inBase || !inMod || !out) {
        cerr << "Error opening files!" << endl;
        return;
    }
    
    string contentBase((istreambuf_iterator<char>(inBase)), istreambuf_iterator<char>());
    string contentMod((istreambuf_iterator<char>(inMod)), istreambuf_iterator<char>());
    
    unordered_set<string> firstThreeLinesInBase;
    
    size_t group_pos = 0;
    while (true) {
        string groupSection = extractFullSection("IssuableWeaponsGroup", contentBase, group_pos);
        if (groupSection.empty()) break;
        
        size_t weapon_pos = groupSection.find("<IssuableWeapon>");
        while (weapon_pos != string::npos) {
            string weaponSection = extractFullSection("IssuableWeapon", groupSection, weapon_pos);
            if (weaponSection.empty()) break;
            
            string firstThree = extractFirstThreeLines(weaponSection);
            if (!firstThree.empty()) {
                firstThreeLinesInBase.insert(firstThree);
            }
            
            weapon_pos = groupSection.find("<IssuableWeapon>", weapon_pos + weaponSection.length());
        }
        
        group_pos += groupSection.length();
    }
    
    string namespaces = extractRootNamespaces(contentMod);
    out << "<ArrayOfIssuableWeaponsGroup";
    if (!namespaces.empty()) out << " " << namespaces;
    out << ">\n";
    int totalKept = 0;
    group_pos = 0;
    
    while (true) {
        string groupSection = extractFullSection("IssuableWeaponsGroup", contentMod, group_pos);
        if (groupSection.empty()) break;
        
        size_t id_start = groupSection.find("<IssuableWeaponsID>");
        size_t id_end = groupSection.find("</IssuableWeaponsID>");
        if (id_start != string::npos && id_end != string::npos) {
            out << "<IssuableWeaponsGroup>\n";
            out << groupSection.substr(id_start, id_end - id_start + string("</IssuableWeaponsID>").length()) << "\n";
            out << "<IssuableWeapons>\n";
        } else {
            group_pos += groupSection.length();
            continue;
        }
        
        int keptInGroup = 0;
        size_t weapon_pos = groupSection.find("<IssuableWeapon>");
        while (weapon_pos != string::npos) {
            string weaponSection = extractFullSection("IssuableWeapon", groupSection, weapon_pos);
            if (weaponSection.empty()) break;
            
            string firstThree = extractFirstThreeLines(weaponSection);
            
            if (firstThreeLinesInBase.find(firstThree) == firstThreeLinesInBase.end()) {
                out << weaponSection << "\n";
                keptInGroup++;
                totalKept++;
            }
            
            weapon_pos = groupSection.find("<IssuableWeapon>", weapon_pos + weaponSection.length());
        }
        
        out << "</IssuableWeapons>\n";
        out << "</IssuableWeaponsGroup>\n";
        if (keptInGroup > 0) {
            cout << "  Kept " << keptInGroup << " unique IssuableWeapon entries in group" << endl;
        }
        
        group_pos += groupSection.length();
    }
    
    out << "</ArrayOfIssuableWeaponsGroup>";
    cout << "Created " << outputFile << " with " << totalKept << " unique IssuableWeapon entries" << endl;
}

void processDispatchablePeopleFile(const string& baseFile, const string& modFile) {
    size_t underscore_pos = modFile.find('_');
    if (underscore_pos == string::npos) {
        cerr << "Invalid mod filename format: " << modFile << endl;
        return;
    }
    
    string outputFile = modFile.substr(0, underscore_pos) + "+" + modFile.substr(underscore_pos);
    
    ifstream inBase(baseFile);
    ifstream inMod(modFile);
    ofstream out(outputFile);
    
    if (!inBase || !inMod || !out) {
        cerr << "Error opening files!" << endl;
        return;
    }
    
    string contentBase((istreambuf_iterator<char>(inBase)), istreambuf_iterator<char>());
    string contentMod((istreambuf_iterator<char>(inMod)), istreambuf_iterator<char>());
    
    unordered_set<string> firstThreeLinesInBase;
    
    size_t group_pos = 0;
    while (true) {
        string groupSection = extractFullSection("DispatchablePersonGroup", contentBase, group_pos);
        if (groupSection.empty()) break;
        
        size_t person_pos = groupSection.find("<DispatchablePerson>");
        while (person_pos != string::npos) {
            string personSection = extractFullSection("DispatchablePerson", groupSection, person_pos);
            if (personSection.empty()) break;
            
            string firstThree = extractFirstThreeLines(personSection);
            if (!firstThree.empty()) {
                firstThreeLinesInBase.insert(firstThree);
            }
            
            person_pos = groupSection.find("<DispatchablePerson>", person_pos + personSection.length());
        }
        
        group_pos += groupSection.length();
    }
    
    string namespaces = extractRootNamespaces(contentMod);
    out << "<ArrayOfDispatchableVehicleGroup";
    if (!namespaces.empty()) out << " " << namespaces;
    out << ">\n";
    int totalKept = 0;
    group_pos = 0;
    
    while (true) {
        string groupSection = extractFullSection("DispatchablePersonGroup", contentMod, group_pos);
        if (groupSection.empty()) break;
        
        size_t id_start = groupSection.find("<DispatchablePersonGroupID>");
        size_t id_end = groupSection.find("</DispatchablePersonGroupID>");
        if (id_start != string::npos && id_end != string::npos) {
            out << "<DispatchablePersonGroup>\n";
            out << groupSection.substr(id_start, id_end - id_start + string("</DispatchablePersonGroupID>").length()) << "\n";
            out << "<DispatchablePeople>\n";
        } else {
            group_pos += groupSection.length();
            continue;
        }
        
        int keptInGroup = 0;
        size_t person_pos = groupSection.find("<DispatchablePerson>");
        while (person_pos != string::npos) {
            string personSection = extractFullSection("DispatchablePerson", groupSection, person_pos);
            if (personSection.empty()) break;
            
            string firstThree = extractFirstThreeLines(personSection);
            
            if (firstThreeLinesInBase.find(firstThree) == firstThreeLinesInBase.end()) {
                out << personSection << "\n";
                keptInGroup++;
                totalKept++;
            }
            
            person_pos = groupSection.find("<DispatchablePerson>", person_pos + personSection.length());
        }
        
        out << "</DispatchablePeople>\n";
        out << "</DispatchablePersonGroup>\n";
        if (keptInGroup > 0) {
            cout << "  Kept " << keptInGroup << " unique DispatchablePerson entries in group" << endl;
        }
        
        group_pos += groupSection.length();
    }
    
    out << "</ArrayOfDispatchablePersonGroup>";
    cout << "Created " << outputFile << " with " << totalKept << " unique DispatchablePerson entries" << endl;
}

void processDispatchableVehiclesFile(const string& baseFile, const string& modFile) {
    size_t underscore_pos = modFile.find('_');
    if (underscore_pos == string::npos) {
        cerr << "Invalid mod filename format: " << modFile << endl;
        return;
    }
    
    string outputFile = modFile.substr(0, underscore_pos) + "+" + modFile.substr(underscore_pos);
    
    ifstream inBase(baseFile);
    ifstream inMod(modFile);
    ofstream out(outputFile);
    
    if (!inBase || !inMod || !out) {
        cerr << "Error opening files!" << endl;
        return;
    }
    
    string contentBase((istreambuf_iterator<char>(inBase)), istreambuf_iterator<char>());
    string contentMod((istreambuf_iterator<char>(inMod)), istreambuf_iterator<char>());
    
    unordered_set<string> firstThreeLinesInBase;
    
    size_t group_pos = 0;
    while (true) {
        string groupSection = extractFullSection("DispatchableVehicleGroup", contentBase, group_pos);
        if (groupSection.empty()) break;
        
        size_t vehicle_pos = groupSection.find("<DispatchableVehicle>");
        while (vehicle_pos != string::npos) {
            string vehicleSection = extractFullSection("DispatchableVehicle", groupSection, vehicle_pos);
            if (vehicleSection.empty()) break;
            
            string firstThree = extractFirstThreeLines(vehicleSection);
            if (!firstThree.empty()) {
                firstThreeLinesInBase.insert(firstThree);
            }
            
            vehicle_pos = groupSection.find("<DispatchableVehicle>", vehicle_pos + vehicleSection.length());
        }
        
        group_pos += groupSection.length();
    }
    
     string namespaces = extractRootNamespaces(contentMod);
    out << "<ArrayOfDispatchableVehicleGroup";
    if (!namespaces.empty()) out << " " << namespaces;
    out << ">\n";
    int totalKept = 0;
    group_pos = 0;
    
    while (true) {
        string groupSection = extractFullSection("DispatchableVehicleGroup", contentMod, group_pos);
        if (groupSection.empty()) break;
        
        size_t id_start = groupSection.find("<DispatchableVehicleGroupID>");
        size_t id_end = groupSection.find("</DispatchableVehicleGroupID>");
        if (id_start != string::npos && id_end != string::npos) {
            out << "<DispatchableVehicleGroup>\n";
            out << groupSection.substr(id_start, id_end - id_start + string("</DispatchableVehicleGroupID>").length()) << "\n";
            out << "<DispatchableVehicles>\n";
        } else {
            group_pos += groupSection.length();
            continue;
        }
        
        int keptInGroup = 0;
        size_t vehicle_pos = groupSection.find("<DispatchableVehicle>");
        while (vehicle_pos != string::npos) {
            string vehicleSection = extractFullSection("DispatchableVehicle", groupSection, vehicle_pos);
            if (vehicleSection.empty()) break;
            
            string firstThree = extractFirstThreeLines(vehicleSection);
            
            if (firstThreeLinesInBase.find(firstThree) == firstThreeLinesInBase.end()) {
                out << vehicleSection << "\n";
                keptInGroup++;
                totalKept++;
            }
            
            vehicle_pos = groupSection.find("<DispatchableVehicle>", vehicle_pos + vehicleSection.length());
        }
        
        out << "</DispatchableVehicles>\n";
        out << "</DispatchableVehicleGroup>\n";
        if (keptInGroup > 0) {
            cout << "  Kept " << keptInGroup << " unique DispatchableVehicle entries in group" << endl;
        }
        
        group_pos += groupSection.length();
    }
    
    out << "</ArrayOfDispatchableVehicleGroup>";
    cout << "Created " << outputFile << " with " << totalKept << " unique DispatchableVehicle entries" << endl;
}

void processHeadsFile(const string& baseFile, const string& modFile) {
    size_t underscore_pos = modFile.find('_');
    if (underscore_pos == string::npos) {
        cerr << "Invalid mod filename format: " << modFile << endl;
        return;
    }
    
    string outputFile = modFile.substr(0, underscore_pos) + "+" + modFile.substr(underscore_pos);
    
    ifstream inBase(baseFile);
    ifstream inMod(modFile);
    ofstream out(outputFile);
    
    if (!inBase || !inMod || !out) {
        cerr << "Error opening files!" << endl;
        return;
    }
    
    string contentBase((istreambuf_iterator<char>(inBase)), istreambuf_iterator<char>());
    string contentMod((istreambuf_iterator<char>(inMod)), istreambuf_iterator<char>());
    
    unordered_set<string> firstThreeLinesInBase;
    
    size_t group_pos = 0;
    while (true) {
        string groupSection = extractFullSection("HeadDataGroup", contentBase, group_pos);
        if (groupSection.empty()) break;
        
        size_t head_pos = groupSection.find("<RandomHeadData>");
        while (head_pos != string::npos) {
            string headSection = extractFullSection("RandomHeadData", groupSection, head_pos);
            if (headSection.empty()) break;
            
            string firstThree = extractFirstThreeLines(headSection);
            if (!firstThree.empty()) {
                firstThreeLinesInBase.insert(firstThree);
            }
            
            head_pos = groupSection.find("<RandomHeadData>", head_pos + headSection.length());
        }
        
        group_pos += groupSection.length();
    }
    
     string namespaces = extractRootNamespaces(contentMod);
    out << "<ArrayOfHeadDataGroup";
    if (!namespaces.empty()) out << " " << namespaces;
    out << ">\n";
    int totalKept = 0;
    group_pos = 0;
    
    while (true) {
        string groupSection = extractFullSection("HeadDataGroup", contentMod, group_pos);
        if (groupSection.empty()) break;
        
        size_t id_start = groupSection.find("<HeadDataGroupID>");
        size_t id_end = groupSection.find("</HeadDataGroupID>");
        if (id_start != string::npos && id_end != string::npos) {
            out << "<HeadDataGroup>\n";
            out << groupSection.substr(id_start, id_end - id_start + string("</HeadDataGroupID>").length()) << "\n";
            out << "<HeadList>\n";
        } else {
            group_pos += groupSection.length();
            continue;
        }
        
        int keptInGroup = 0;
        size_t head_pos = groupSection.find("<RandomHeadData>");
        while (head_pos != string::npos) {
            string headSection = extractFullSection("RandomHeadData", groupSection, head_pos);
            if (headSection.empty()) break;
            
            string firstThree = extractFirstThreeLines(headSection);
            
            if (firstThreeLinesInBase.find(firstThree) == firstThreeLinesInBase.end()) {
                out << headSection << "\n";
                keptInGroup++;
                totalKept++;
            }
            
            head_pos = groupSection.find("<RandomHeadData>", head_pos + headSection.length());
        }
        
        out << "</HeadList>\n";
        out << "</HeadDataGroup>\n";
        if (keptInGroup > 0) {
            cout << "  Kept " << keptInGroup << " unique RandomHeadData entries in group" << endl;
        }
        
        group_pos += groupSection.length();
    }
    
    out << "</ArrayOfHeadDataGroup>";
    cout << "Created " << outputFile << " with " << totalKept << " unique RandomHeadData entries" << endl;
}

void processInteriorsFile(const string& baseFile, const string& modFile) {
    size_t underscore_pos = modFile.find('_');
    if (underscore_pos == string::npos) {
        cerr << "Invalid mod filename format: " << modFile << endl;
        return;
    }
    
    string outputFile = modFile.substr(0, underscore_pos) + "+" + modFile.substr(underscore_pos);
    
    ifstream inBase(baseFile);
    ifstream inMod(modFile);
    ofstream out(outputFile);
    
    if (!inBase || !inMod || !out) {
        cerr << "Error opening files!" << endl;
        return;
    }
    
    string contentBase((istreambuf_iterator<char>(inBase)), istreambuf_iterator<char>());
    string contentMod((istreambuf_iterator<char>(inMod)), istreambuf_iterator<char>());
    
    vector<string> interiorTypes = {
        "Interior",
        "ResidenceInterior",
        "GangDenInterior", 
        "BarberShopInterior",
        "BusinessInterior"
    };
    
    unordered_set<string> firstThreeLinesInBase;
    
    for (const auto& interiorType : interiorTypes) {
        size_t pos = 0;
        while (true) {
            string section = extractFullSection(interiorType, contentBase, pos);
            if (section.empty()) break;
            
            string firstThree = extractFirstThreeLines(section);
            if (!firstThree.empty()) {
                firstThreeLinesInBase.insert(firstThree);
            }
            pos += section.length();
        }
    }
    
    string namespaces = extractRootNamespaces(contentMod);
    out << "<PossibleInteriors";
    if (!namespaces.empty()) out << " " << namespaces;
    out << ">\n";
    int totalKept = 0;
    
    out << "<GeneralInteriors>\n";
    size_t pos = 0;
    int keptCount = 0;
    while (true) {
        string section = extractFullSection("Interior", contentMod, pos);
        if (section.empty()) break;
        
        string firstThree = extractFirstThreeLines(section);
        
        if (firstThreeLinesInBase.find(firstThree) == firstThreeLinesInBase.end()) {
            out << section << "\n";
            keptCount++;
            totalKept++;
        }
        pos += section.length();
    }
    out << "</GeneralInteriors>\n";
    if (keptCount > 0) {
        cout << "  Kept " << keptCount << " unique GeneralInterior sections" << endl;
    }
    
    out << "<ResidenceInteriors>\n";
    pos = 0;
    keptCount = 0;
    while (true) {
        string section = extractFullSection("ResidenceInterior", contentMod, pos);
        if (section.empty()) break;
        
        string firstThree = extractFirstThreeLines(section);
        
        if (firstThreeLinesInBase.find(firstThree) == firstThreeLinesInBase.end()) {
            out << section << "\n";
            keptCount++;
            totalKept++;
        }
        pos += section.length();
    }
    out << "</ResidenceInteriors>\n";
    if (keptCount > 0) {
        cout << "  Kept " << keptCount << " unique ResidenceInterior sections" << endl;
    }
    
    out << "<GangDenInteriors>\n";
    pos = 0;
    keptCount = 0;
    while (true) {
        string section = extractFullSection("GangDenInterior", contentMod, pos);
        if (section.empty()) break;
        
        string firstThree = extractFirstThreeLines(section);
        
        if (firstThreeLinesInBase.find(firstThree) == firstThreeLinesInBase.end()) {
            out << section << "\n";
            keptCount++;
            totalKept++;
        }
        pos += section.length();
    }
    out << "</GangDenInteriors>\n";
    if (keptCount > 0) {
        cout << "  Kept " << keptCount << " unique GangDenInterior sections" << endl;
    }
    
    out << "<BarberShopInteriors>\n";
    pos = 0;
    keptCount = 0;
    while (true) {
        string section = extractFullSection("BarberShopInterior", contentMod, pos);
        if (section.empty()) break;
        
        string firstThree = extractFirstThreeLines(section);
        
        if (firstThreeLinesInBase.find(firstThree) == firstThreeLinesInBase.end()) {
            out << section << "\n";
            keptCount++;
            totalKept++;
        }
        pos += section.length();
    }
    out << "</BarberShopInteriors>\n";
    if (keptCount > 0) {
        cout << "  Kept " << keptCount << " unique BarberShopInterior sections" << endl;
    }
    
    out << "<BusinessInteriors>\n";
    pos = 0;
    keptCount = 0;
    while (true) {
        string section = extractFullSection("BusinessInterior", contentMod, pos);
        if (section.empty()) break;
        
        string firstThree = extractFirstThreeLines(section);
        
        if (firstThreeLinesInBase.find(firstThree) == firstThreeLinesInBase.end()) {
            out << section << "\n";
            keptCount++;
            totalKept++;
        }
        pos += section.length();
    }
    out << "</BusinessInteriors>\n";
    if (keptCount > 0) {
        cout << "  Kept " << keptCount << " unique BusinessInterior sections" << endl;
    }
    
    out << "</PossibleInteriors>";
    cout << "Created " << outputFile << " with " << totalKept << " unique interior sections" << endl;
}

void processContactsFile(const string& baseFile, const string& modFile) {
    size_t underscore_pos = modFile.find('_');
    if (underscore_pos == string::npos) {
        cerr << "Invalid mod filename format: " << modFile << endl;
        return;
    }
    
    string outputFile = modFile.substr(0, underscore_pos) + "+" + modFile.substr(underscore_pos);
    
    ifstream inBase(baseFile);
    ifstream inMod(modFile);
    ofstream out(outputFile);
    
    if (!inBase || !inMod || !out) {
        cerr << "Error opening files!" << endl;
        return;
    }
    
    string contentBase((istreambuf_iterator<char>(inBase)), istreambuf_iterator<char>());
    string contentMod((istreambuf_iterator<char>(inMod)), istreambuf_iterator<char>());
    
    vector<string> contactTypes = {
        "GangContact",
        "GunDealerContact",
        "InformantContact",
        "ServiceContact",
        "SupplierContact",
        "EmergencyServicesContact",
        "CorruptCopContact",
        "VehicleExporterContact",
        "TaxiServiceContact",
        "KillerContact"
    };
    
    unordered_set<string> firstThreeLinesInBase;
    
    for (const auto& contactType : contactTypes) {
        size_t pos = 0;
        while (true) {
            string section = extractFullSection(contactType, contentBase, pos);
            if (section.empty()) break;
            
            string firstThree = extractFirstThreeLines(section);
            if (!firstThree.empty()) {
                firstThreeLinesInBase.insert(firstThree);
            }
            pos += section.length();
        }
    }
    
    string namespaces = extractRootNamespaces(contentMod);
    out << "<PossibleContacts";
    if (!namespaces.empty()) out << " " << namespaces;
    out << ">\n";
    int totalKept = 0;
    
    for (const auto& contactType : contactTypes) {
        size_t pos = 0;
        int keptCount = 0;
        
        string parentTag;
        if (contactType == "EmergencyServicesContact" || contactType == "CorruptCopContact") {
            parentTag = contactType;
        } else {
            parentTag = contactType.substr(0, contactType.size() - 7) + "Contacts";
        }
        
        out << "<" << parentTag << ">\n";
        
        while (true) {
            string section = extractFullSection(contactType, contentMod, pos);
            if (section.empty()) break;
            
            string firstThree = extractFirstThreeLines(section);
            
            if (firstThreeLinesInBase.find(firstThree) == firstThreeLinesInBase.end()) {
                out << section << "\n";
                keptCount++;
                totalKept++;
            }
            pos += section.length();
        }
        
        out << "</" << parentTag << ">\n";
        if (keptCount > 0) {
            cout << "  Kept " << keptCount << " unique " << contactType << " sections" << endl;
        }
    }
    
    out << "</PossibleContacts>";
    cout << "Created " << outputFile << " with " << totalKept << " unique contact sections" << endl;
}

void processLocationTypesFile(const string& baseFile, const string& modFile) {
    size_t underscore_pos = modFile.find('_');
    if (underscore_pos == string::npos) {
        cerr << "Invalid mod filename format: " << modFile << endl;
        return;
    }
    
    string outputFile = modFile.substr(0, underscore_pos) + "+" + modFile.substr(underscore_pos);
    
    ifstream inBase(baseFile);
    ifstream inMod(modFile);
    ofstream out(outputFile);
    
    if (!inBase || !inMod || !out) {
        cerr << "Error opening files!" << endl;
        return;
    }
    
    string contentBase((istreambuf_iterator<char>(inBase)), istreambuf_iterator<char>());
    string contentMod((istreambuf_iterator<char>(inMod)), istreambuf_iterator<char>());
    
    unordered_set<string> baseCounties;
    size_t county_pos = 0;
    while (true) {
        string countySection = extractFullSection("GameCounty", contentBase, county_pos);
        if (countySection.empty()) break;
        
        string firstThree = extractFirstThreeLines(countySection);
        if (!firstThree.empty()) {
            baseCounties.insert(firstThree);
        }
        county_pos += countySection.length();
    }
    
    string namespaces = extractRootNamespaces(contentMod);
    out << "<LocationTypeManager";
    if (!namespaces.empty()) out << " " << namespaces;
    out << ">\n";
    out << "<CountyList>\n";
    
    county_pos = 0;
    int keptCount = 0;
    while (true) {
        string countySection = extractFullSection("GameCounty", contentMod, county_pos);
        if (countySection.empty()) break;
        
        string firstThree = extractFirstThreeLines(countySection);
        
        if (baseCounties.find(firstThree) == baseCounties.end()) {
            out << countySection << "\n";
            keptCount++;
        }
        county_pos += countySection.length();
    }
    
    out << "</CountyList>\n";
    out << "</LocationTypeManager>";
    cout << "Created " << outputFile << " with " << keptCount << " unique GameCounty sections" << endl;
}

void processShopMenusFile(const string& baseFile, const string& modFile) {
    size_t underscore_pos = modFile.find('_');
    if (underscore_pos == string::npos) {
        cerr << "Invalid mod filename format: " << modFile << endl;
        return;
    }
    
    string outputFile = modFile.substr(0, underscore_pos) + "+" + modFile.substr(underscore_pos);
    
    ifstream inBase(baseFile);
    ifstream inMod(modFile);
    ofstream out(outputFile);
    
    if (!inBase || !inMod || !out) {
        cerr << "Error opening files!" << endl;
        return;
    }
    
    string contentBase((istreambuf_iterator<char>(inBase)), istreambuf_iterator<char>());
    string contentMod((istreambuf_iterator<char>(inMod)), istreambuf_iterator<char>());

    unordered_set<string> baseShopMenus;
    unordered_set<string> baseShopMenuGroups;
    unordered_set<string> baseShopMenuGroupContainers;
    unordered_set<string> basePercentageSelects;
    unordered_set<string> basePercentageSelectGroupContainers;
    unordered_set<string> basePropShopMenus;
    unordered_set<string> baseTreatmentOptions;

    size_t basePos = contentBase.find("<PropShopMenus>");
    if (basePos != string::npos) {
        string propShopMenusSection = extractFullSection("PropShopMenus", contentBase, basePos);
        size_t propMenuPos = 0;
        while (true) {
            string propMenu = extractFullSection("PropShopMenu", propShopMenusSection, propMenuPos);
            if (propMenu.empty()) break;
            
            string signature = extractFirstThreeLines(propMenu);
            if (!signature.empty()) basePropShopMenus.insert(signature);
            propMenuPos = propShopMenusSection.find("<PropShopMenu>", propMenuPos + 1);
        }
    }

    unordered_set<string> baseMedicalTreatments;
basePos = contentBase.find("<TreatmentOptionsList>");
if (basePos != string::npos) {
    string treatmentOptionsSection = extractFullSection("TreatmentOptionsList", contentBase, basePos);
    size_t treatmentPos = 0;
    while (true) {
        string treatmentOption = extractFullSection("TreatmentOptions", treatmentOptionsSection, treatmentPos);
        if (treatmentOption.empty()) break;
        
        size_t medicalPos = treatmentOption.find("<MedicalTreatment>");
        while (medicalPos != string::npos) {
            string medicalTreatment = extractFullSection("MedicalTreatment", treatmentOption, medicalPos);
            if (medicalTreatment.empty()) break;
            
            string medicalSignature = extractFirstThreeLines(medicalTreatment);
            if (!medicalSignature.empty()) {
                baseMedicalTreatments.insert(medicalSignature);
            }
            medicalPos = treatmentOption.find("<MedicalTreatment>", medicalPos + medicalTreatment.length());
        }
        
        treatmentPos = treatmentOptionsSection.find("<TreatmentOptions>", treatmentPos + 1);
    }
}

    basePos = 0;
    while (true) {
        string shopMenu = extractFullSection("ShopMenu", contentBase, basePos);
        if (shopMenu.empty()) break;
        string signature = extractFirstThreeLines(shopMenu);
        if (!signature.empty()) baseShopMenus.insert(signature);
        basePos = contentBase.find("<ShopMenu>", basePos + 1);
    }

    basePos = 0;
    while (true) {
        string groupList = extractFullSection("ShopMenuGroupList", contentBase, basePos);
        if (groupList.empty()) break;
        
        size_t groupPos = 0;
        while (true) {
            string menuGroup = extractFullSection("ShopMenuGroup", groupList, groupPos);
            if (menuGroup.empty()) break;
            
            string groupSignature = extractFirstThreeLines(menuGroup);
            if (!groupSignature.empty()) baseShopMenuGroups.insert(groupSignature);
            
            size_t possibleMenusPos = menuGroup.find("<PossibleShopMenus>");
            if (possibleMenusPos != string::npos) {
                size_t possibleMenusEnd = menuGroup.find("</PossibleShopMenus>", possibleMenusPos);
                if (possibleMenusEnd != string::npos) {
                    string possibleMenusSection = menuGroup.substr(possibleMenusPos, possibleMenusEnd - possibleMenusPos + 20);
                    
                    size_t selectPos = 0;
                    while (true) {
                        string percentageSelect = extractFullSection("PercentageSelectShopMenu", possibleMenusSection, selectPos);
                        if (percentageSelect.empty()) break;
                        
                        string selectSignature = extractFirstThreeLines(percentageSelect);
                        if (!selectSignature.empty()) basePercentageSelects.insert(selectSignature);
                        
                        size_t shopMenuPos = percentageSelect.find("<ShopMenu>");
                        if (shopMenuPos != string::npos) {
                            string shopMenu = extractFullSection("ShopMenu", percentageSelect, shopMenuPos);
                            string menuSignature = extractFirstThreeLines(shopMenu);
                            if (!menuSignature.empty()) baseShopMenus.insert(menuSignature);
                        }
                        selectPos = possibleMenusSection.find("<PercentageSelectShopMenu>", selectPos + 1);
                    }
                }
            }
            groupPos = groupList.find("<ShopMenuGroup>", groupPos + 1);
        }
        basePos = contentBase.find("<ShopMenuGroupList>", basePos + 1);
    }

    basePos = 0;
    while (true) {
        string containers = extractFullSection("ShopMenuGroupContainers", contentBase, basePos);
        if (containers.empty()) break;
        
        size_t containerPos = 0;
        while (true) {
            string groupContainer = extractFullSection("ShopMenuGroupContainer", containers, containerPos);
            if (groupContainer.empty()) break;
            
            string containerSignature = extractFirstThreeLines(groupContainer);
            if (!containerSignature.empty()) baseShopMenuGroupContainers.insert(containerSignature);
            
            size_t percentageSelectsPos = groupContainer.find("<PercentageSelectGropMenuGroups>");
            if (percentageSelectsPos != string::npos) {
                size_t percentageSelectsEnd = groupContainer.find("</PercentageSelectGropMenuGroups>", percentageSelectsPos);
                if (percentageSelectsEnd != string::npos) {
                    string percentageSelectsSection = groupContainer.substr(percentageSelectsPos, percentageSelectsEnd - percentageSelectsPos + 33);
                    
                    size_t selectPos = 0;
                    while (true) {
                        string percentageSelect = extractFullSection("PercentageSelectGroupMenuContainer", percentageSelectsSection, selectPos);
                        if (percentageSelect.empty()) break;
                        
                        string selectSignature = extractFirstThreeLines(percentageSelect);
                        if (!selectSignature.empty()) basePercentageSelectGroupContainers.insert(selectSignature);
                        
                        selectPos = percentageSelectsSection.find("<PercentageSelectGroupMenuContainer>", selectPos + 1);
                    }
                }
            }
            containerPos = containers.find("<ShopMenuGroupContainer>", containerPos + 1);
        }
        basePos = contentBase.find("<ShopMenuGroupContainers>", basePos + 1);
    }

    string namespaces = extractRootNamespaces(contentMod);
    out << "<ShopMenuTypes";
    if (!namespaces.empty()) out << " " << namespaces;
    out << ">\n";
    
    // 1. ShopMenuList
    out << "<ShopMenuList>\n";
    size_t modPos = contentMod.find("<ShopMenuList>");
    if (modPos != string::npos) {
        string listSection = extractFullSection("ShopMenuList", contentMod, modPos);
        size_t shopMenuPos = 0;
        int keptMenus = 0;

        while (true) {
            string shopMenu = extractFullSection("ShopMenu", listSection, shopMenuPos);
            if (shopMenu.empty()) break;

            string signature = extractFirstThreeLines(shopMenu);
            if (baseShopMenus.find(signature) == baseShopMenus.end()) {
                out << shopMenu << "\n";
                keptMenus++;
                baseShopMenus.insert(signature);
            }
            shopMenuPos = listSection.find("<ShopMenu>", shopMenuPos + 1);
        }
        cout << "  Kept " << keptMenus << " unique ShopMenus\n";
    }
    out << "</ShopMenuList>\n";

    // 2. ShopMenuGroupList
    out << "<ShopMenuGroupList>\n";
    modPos = contentMod.find("<ShopMenuGroupList>");
    if (modPos != string::npos) {
        string groupListSection = extractFullSection("ShopMenuGroupList", contentMod, modPos);
        size_t groupPos = 0;
        int keptGroups = 0, keptSelects = 0;

        while (true) {
            string menuGroup = extractFullSection("ShopMenuGroup", groupListSection, groupPos);
            if (menuGroup.empty()) break;

            string groupSignature = extractFirstThreeLines(menuGroup);
            if (baseShopMenuGroups.find(groupSignature) == baseShopMenuGroups.end()) {
                out << menuGroup << "\n";
                keptGroups++;
                baseShopMenuGroups.insert(groupSignature);
                groupPos = groupListSection.find("<ShopMenuGroup>", groupPos + 1);
                continue;
            }

            string outputGroup;
            bool hasUniqueContent = false;
            size_t possibleMenusStart = menuGroup.find("<PossibleShopMenus>");
            if (possibleMenusStart != string::npos) {
                outputGroup = menuGroup.substr(0, possibleMenusStart);
                size_t possibleMenusEnd = menuGroup.find("</PossibleShopMenus>", possibleMenusStart);
                if (possibleMenusEnd != string::npos) {
                    string possibleMenusSection = menuGroup.substr(possibleMenusStart, possibleMenusEnd - possibleMenusStart + 20);
                    string newPossibleMenus = "<PossibleShopMenus>\n";
                    
                    size_t selectPos = 0;
                    while (true) {
                        string percentageSelect = extractFullSection("PercentageSelectShopMenu", possibleMenusSection, selectPos);
                        if (percentageSelect.empty()) break;
                        
                        string selectSignature = extractFirstThreeLines(percentageSelect);
                        if (basePercentageSelects.find(selectSignature) == basePercentageSelects.end()) {
                            newPossibleMenus += percentageSelect + "\n";
                            hasUniqueContent = true;
                            keptSelects++;
                            basePercentageSelects.insert(selectSignature);
                        }
                        selectPos = possibleMenusSection.find("<PercentageSelectShopMenu>", selectPos + 1);
                    }
                    
                    newPossibleMenus += "</PossibleShopMenus>";
                    outputGroup += newPossibleMenus;
                    size_t groupEnd = menuGroup.find("</ShopMenuGroup>", possibleMenusEnd);
                    if (groupEnd != string::npos) {
                        outputGroup += menuGroup.substr(possibleMenusEnd + 20, groupEnd - (possibleMenusEnd + 20) + 16);
                    }
                    
                    if (hasUniqueContent) {
                        out << outputGroup << "\n";
                        keptGroups++;
                    }
                }
            }
            groupPos = groupListSection.find("<ShopMenuGroup>", groupPos + 1);
        }
        cout << "  Kept " << keptGroups << " ShopMenuGroups with " << keptSelects << " unique PercentageSelectShopMenus\n";
    }
    out << "</ShopMenuGroupList>\n";

    // 3. ShopMenuGroupContainers 
    out << "<ShopMenuGroupContainers>\n";
    modPos = contentMod.find("<ShopMenuGroupContainers>");
    if (modPos != string::npos) {
        string containersSection = extractFullSection("ShopMenuGroupContainers", contentMod, modPos);
        size_t containerPos = 0;
        int keptContainers = 0, keptSelects = 0;
        unordered_set<string> processedContainers;

        while (true) {
            string groupContainer = extractFullSection("ShopMenuGroupContainer", containersSection, containerPos);
            if (groupContainer.empty()) break;

            string containerSignature = extractFirstThreeLines(groupContainer);
            
            if (processedContainers.find(containerSignature) != processedContainers.end()) {
                containerPos = containersSection.find("<ShopMenuGroupContainer>", containerPos + 1);
                continue;
            }

            if (baseShopMenuGroupContainers.find(containerSignature) == baseShopMenuGroupContainers.end()) {
                out << groupContainer << "\n";
                keptContainers++;
                processedContainers.insert(containerSignature);
                containerPos = containersSection.find("<ShopMenuGroupContainer>", containerPos + 1);
                continue;
            }

            string outputContainer;
            bool hasUniqueContent = false;
            size_t percentageSelectsStart = groupContainer.find("<PercentageSelectGropMenuGroups>");
            if (percentageSelectsStart != string::npos) {
                outputContainer = groupContainer.substr(0, percentageSelectsStart);
                size_t percentageSelectsEnd = groupContainer.find("</PercentageSelectGropMenuGroups>", percentageSelectsStart);
                if (percentageSelectsEnd != string::npos) {
                    string percentageSelectsSection = groupContainer.substr(percentageSelectsStart, percentageSelectsEnd - percentageSelectsStart + 33);
                    string newPercentageSelects = "<PercentageSelectGropMenuGroups>\n";
                    
                    size_t selectPos = 0;
                    while (true) {
                        string percentageSelect = extractFullSection("PercentageSelectGroupMenuContainer", percentageSelectsSection, selectPos);
                        if (percentageSelect.empty()) break;
                        
                        string selectSignature = extractFirstThreeLines(percentageSelect);
                        if (basePercentageSelectGroupContainers.find(selectSignature) == basePercentageSelectGroupContainers.end()) {
                            newPercentageSelects += percentageSelect + "\n";
                            hasUniqueContent = true;
                            keptSelects++;
                            basePercentageSelectGroupContainers.insert(selectSignature);
                        }
                        selectPos = percentageSelectsSection.find("<PercentageSelectGroupMenuContainer>", selectPos + 1);
                    }
                    
                    newPercentageSelects += "</PercentageSelectGropMenuGroups>";
                    outputContainer += newPercentageSelects;
                    size_t containerEnd = groupContainer.find("</ShopMenuGroupContainer>", percentageSelectsEnd);
                    if (containerEnd != string::npos) {
                        outputContainer += groupContainer.substr(percentageSelectsEnd + 33, containerEnd - (percentageSelectsEnd + 33) + 25);
                    }
                    
                    if (hasUniqueContent) {
                        out << outputContainer << "\n";
                        keptContainers++;
                        processedContainers.insert(containerSignature);
                    }
                }
            }
            containerPos = containersSection.find("<ShopMenuGroupContainer>", containerPos + 1);
        }
        cout << "  Kept " << keptContainers << " ShopMenuGroupContainers with " << keptSelects << " unique PercentageSelectGroupMenuContainers\n";
    }
    out << "</ShopMenuGroupContainers>\n";
    
    // 4. Process PropShopMenus
    out << "<PropShopMenus>\n";
    modPos = contentMod.find("<PropShopMenus>");
    if (modPos != string::npos) {
        string propShopMenusSection = extractFullSection("PropShopMenus", contentMod, modPos);
        size_t propMenuPos = 0;
        int keptPropMenus = 0;
        
        while (true) {
            string propMenu = extractFullSection("PropShopMenu", propShopMenusSection, propMenuPos);
            if (propMenu.empty()) break;
            
            string signature = extractFirstThreeLines(propMenu);
            if (basePropShopMenus.find(signature) == basePropShopMenus.end()) {
                out << propMenu << "\n";
                keptPropMenus++;
                basePropShopMenus.insert(signature);
            }
            propMenuPos = propShopMenusSection.find("<PropShopMenu>", propMenuPos + 1);
        }
        cout << "  Kept " << keptPropMenus << " unique PropShopMenu entries\n";
    }
    out << "</PropShopMenus>\n";
    
    // 5. Process TreatmentOptionsList
    out << "<TreatmentOptionsList>\n";
modPos = contentMod.find("<TreatmentOptionsList>");
if (modPos != string::npos) {
    string treatmentOptionsSection = extractFullSection("TreatmentOptionsList", contentMod, modPos);
    size_t treatmentPos = 0;
    int keptTreatments = 0;
    
    while (true) {
        string treatmentOption = extractFullSection("TreatmentOptions", treatmentOptionsSection, treatmentPos);
        if (treatmentOption.empty()) break;
        
        bool hasUniqueMedicalTreatment = false;
        
        size_t medicalPos = treatmentOption.find("<MedicalTreatment>");
        while (medicalPos != string::npos) {
            string medicalTreatment = extractFullSection("MedicalTreatment", treatmentOption, medicalPos);
            if (medicalTreatment.empty()) break;
            
            string medicalSignature = extractFirstThreeLines(medicalTreatment);
            if (baseMedicalTreatments.find(medicalSignature) == baseMedicalTreatments.end()) {
                hasUniqueMedicalTreatment = true;
                break;  
            }
            medicalPos = treatmentOption.find("<MedicalTreatment>", medicalPos + medicalTreatment.length());
        }
        
        if (hasUniqueMedicalTreatment) {
            out << treatmentOption << "\n";
            keptTreatments++;
            
            medicalPos = treatmentOption.find("<MedicalTreatment>");
            while (medicalPos != string::npos) {
                string medicalTreatment = extractFullSection("MedicalTreatment", treatmentOption, medicalPos);
                if (medicalTreatment.empty()) break;
                
                string medicalSignature = extractFirstThreeLines(medicalTreatment);
                if (!medicalSignature.empty()) {
                    baseMedicalTreatments.insert(medicalSignature);
                }
                medicalPos = treatmentOption.find("<MedicalTreatment>", medicalPos + medicalTreatment.length());
            }
        }
        
        treatmentPos = treatmentOptionsSection.find("<TreatmentOptions>", treatmentPos + 1);
    }
    cout << "  Kept " << keptTreatments << " TreatmentOptions with unique MedicalTreatments\n";
}
out << "</TreatmentOptionsList>\n";



    
    out << "</ShopMenuTypes>\n";
    
    cout << "Created " << outputFile << endl;
}

void processModItemsFile(const string& baseFile, const string& modFile) {
    size_t underscore_pos = modFile.find('_');
    if (underscore_pos == string::npos) {
        cerr << "Invalid mod filename format: " << modFile << endl;
        return;
    }
    
    string outputFile = modFile.substr(0, underscore_pos) + "+" + modFile.substr(underscore_pos);
    
    ifstream inBase(baseFile);
    ifstream inMod(modFile);
    ofstream out(outputFile);
    
    if (!inBase || !inMod || !out) {
        cerr << "Error opening files!" << endl;
        return;
    }
    
    string contentBase((istreambuf_iterator<char>(inBase)), istreambuf_iterator<char>());
    string contentMod((istreambuf_iterator<char>(inMod)), istreambuf_iterator<char>());
    
    vector<string> itemTypes = {
        "FlashlightItems",
        "CellphoneItems",
        "ModItems",
        "ShovelItems",
        "UmbrellaItems",
        "LicensePlateItems",
        "LighterItems",
        "ScrewdriverItems",
        "TapeItems",
        "DrillItems",
        "HammerItems",
        "PliersItems",
        "BongItems",
        "PipeItems",
        "RollingPapersItems",
        "FoodItems",
        "SmokeItems",
        "PipeSmokeItems",
        "DrinkItems",
        "InhaleItems",
        "IngestItems",
        "InjectItems",
        "HotelStayItems",
        "WeaponItems",
        "VehicleItems",
        "BinocularsItems",
        "RadioItems",
        "RadarDetectorItems",
        "ValuableItems",
        "EquipmentItems",
        "BodyArmorItems",
        "HardwareItems"
    };
    
    unordered_map<string, unordered_set<string>> baseItems;
    
    for (const auto& itemType : itemTypes) {
        string itemTag = itemType.substr(0, itemType.size() - 1); 
        size_t container_pos = 0;
        
        while (true) {
            string container = extractFullSection(itemType, contentBase, container_pos);
            if (container.empty()) break;
            
            size_t item_pos = 0;
            while (true) {
                string section = extractFullSection(itemTag, container, item_pos);
                if (section.empty()) break;
                
                string firstThree = extractFirstThreeLines(section);
                if (!firstThree.empty()) {
                    baseItems[itemType].insert(firstThree);
                }
                item_pos = container.find("<" + itemTag + ">", item_pos + 1);
                if (item_pos == string::npos) break;
            }
            container_pos = contentBase.find("<" + itemType + ">", container_pos + 1);
        }
    }
    string namespaces = extractRootNamespaces(contentMod);
    out << "<PossibleItems";
    if (!namespaces.empty()) out << " " << namespaces;
    out << ">\n";
    int totalKept = 0;
    unordered_set<string> processedSections;
    
    for (const auto& itemType : itemTypes) {
        string itemTag = itemType.substr(0, itemType.size() - 1); 
        int keptCount = 0;
        
        out << "<" << itemType << ">\n";
        
        size_t container_pos = 0;
        while (true) {
            string container = extractFullSection(itemType, contentMod, container_pos);
            if (container.empty()) break;
            
            size_t item_pos = 0;
            while (true) {
                string section = extractFullSection(itemTag, container, item_pos);
                if (section.empty()) break;
                
                string firstThree = extractFirstThreeLines(section);
                string sectionHash = to_string(hash<string>{}(section));
                
                if (baseItems[itemType].find(firstThree) == baseItems[itemType].end() &&
                    processedSections.find(sectionHash) == processedSections.end()) {
                    out << section << "\n";
                    keptCount++;
                    totalKept++;
                    processedSections.insert(sectionHash);
                }
                item_pos = container.find("<" + itemTag + ">", item_pos + 1);
                if (item_pos == string::npos) break;
            }
            container_pos = contentMod.find("<" + itemType + ">", container_pos + 1);
        }
        
        out << "</" << itemType << ">\n";
        if (keptCount > 0) {
            cout << "  Kept " << keptCount << " unique " << itemTag << " entries" << endl;
        }
    }
    
    out << "</PossibleItems>";
    cout << "Created " << outputFile << " with " << totalKept << " unique item entries" << endl;
}

void processPlateTypesFile(const string& baseFile, const string& modFile) {
    size_t underscore_pos = modFile.find('_');
    if (underscore_pos == string::npos) {
        cerr << "Invalid mod filename format: " << modFile << endl;
        return;
    }
    
    string outputFile = modFile.substr(0, underscore_pos) + "+" + modFile.substr(underscore_pos);
    
    ifstream inBase(baseFile);
    ifstream inMod(modFile);
    ofstream out(outputFile);
    
    if (!inBase || !inMod || !out) {
        cerr << "Error opening files!" << endl;
        return;
    }
    
    string contentBase((istreambuf_iterator<char>(inBase)), istreambuf_iterator<char>());
    string contentMod((istreambuf_iterator<char>(inMod)), istreambuf_iterator<char>());
    
    unordered_set<string> basePlateTypes;
    size_t plateType_pos = 0;
    while (true) {
        string plateTypeSection = extractFullSection("PlateType", contentBase, plateType_pos);
        if (plateTypeSection.empty()) break;
        
        string firstThree = extractFirstThreeLines(plateTypeSection);
        if (!firstThree.empty()) {
            basePlateTypes.insert(firstThree);
        }
        plateType_pos += plateTypeSection.length();
    }
    
    unordered_set<string> baseVanityPlates;
    size_t vanityPlate_pos = 0;
    while (true) {
        string vanityPlateSection = extractFullSection("VanityPlate", contentBase, vanityPlate_pos);
        if (vanityPlateSection.empty()) break;
        
        string firstThree = extractFirstThreeLines(vanityPlateSection);
        if (!firstThree.empty()) {
            baseVanityPlates.insert(firstThree);
        }
        vanityPlate_pos += vanityPlateSection.length();
    }
    
    string namespaces = extractRootNamespaces(contentMod);
    out << "<PlateTypeManager";
    if (!namespaces.empty()) out << " " << namespaces;
    out << ">\n";
    
    out << "<PlateTypeList>\n";
    plateType_pos = 0;
    int keptPlateTypes = 0;
    while (true) {
        string plateTypeSection = extractFullSection("PlateType", contentMod, plateType_pos);
        if (plateTypeSection.empty()) break;
        
        string firstThree = extractFirstThreeLines(plateTypeSection);
        
        if (basePlateTypes.find(firstThree) == basePlateTypes.end()) {
            out << plateTypeSection << "\n";
            keptPlateTypes++;
        }
        plateType_pos += plateTypeSection.length();
    }
    out << "</PlateTypeList>\n";
    if (keptPlateTypes > 0) {
        cout << "  Kept " << keptPlateTypes << " unique PlateType entries" << endl;
    }
    
    out << "<VanityPlates>\n";
    vanityPlate_pos = 0;
    int keptVanityPlates = 0;
    while (true) {
        string vanityPlateSection = extractFullSection("VanityPlate", contentMod, vanityPlate_pos);
        if (vanityPlateSection.empty()) break;
        
        string firstThree = extractFirstThreeLines(vanityPlateSection);
        
        if (baseVanityPlates.find(firstThree) == baseVanityPlates.end()) {
            out << vanityPlateSection << "\n";
            keptVanityPlates++;
        }
        vanityPlate_pos += vanityPlateSection.length();
    }
    out << "</VanityPlates>\n";
    if (keptVanityPlates > 0) {
        cout << "  Kept " << keptVanityPlates << " unique VanityPlate entries" << endl;
    }
    
    out << "</PlateTypeManager>";
    cout << "Created " << outputFile << " with " << (keptPlateTypes + keptVanityPlates) << " unique entries" << endl;
}

void processFilePair(const string& baseFile, const string& modFile, const string& sectionTag) {
    size_t underscore_pos = modFile.find('_');
    if (underscore_pos == string::npos) {
        cerr << "Invalid mod filename format: " << modFile << endl;
        return;
    }
    
    string outputFile = modFile.substr(0, underscore_pos) + "+" + modFile.substr(underscore_pos);
    
    ifstream inBase(baseFile);
    ifstream inMod(modFile);
    ofstream out(outputFile);
    
    if (!inBase || !inMod || !out) {
        cerr << "Error opening files!" << endl;
        return;
    }
    
    string contentBase((istreambuf_iterator<char>(inBase)), istreambuf_iterator<char>());
    string contentMod((istreambuf_iterator<char>(inMod)), istreambuf_iterator<char>());
    
    unordered_set<string> firstThreeLinesInBase;
    size_t pos = 0;
    
    while (true) {
        string section = extractFullSection(sectionTag, contentBase, pos);
        if (section.empty()) break;
        
        string firstThree = extractFirstThreeLines(section);
        if (!firstThree.empty()) {
            firstThreeLinesInBase.insert(firstThree);
        }
        pos += section.length();
    }
    
    string rootTag = "ArrayOf" + sectionTag;
    string namespaces = extractRootNamespaces(contentMod);
    out << "<" << rootTag;
    if (!namespaces.empty()) out << " " << namespaces;
    out << ">\n";
    pos = 0;
    int keptCount = 0;
    
    while (true) {
        string section = extractFullSection(sectionTag, contentMod, pos);
        if (section.empty()) break;
        
        string firstThree = extractFirstThreeLines(section);
        
        if (firstThreeLinesInBase.find(firstThree) == firstThreeLinesInBase.end()) {
            out << section << "\n";
            keptCount++;
        }
        pos += section.length();
    }
    
    out << "</" << rootTag << ">";
    cout << "Created " << outputFile << " with " << keptCount << " unique " << sectionTag << " sections" << endl;
}

int main() {
    vector<pair<string, string>> fileTypes = {
        {"Gangs.xml", "Gang"},
        {"Crimes.xml", "Crime"},
        {"Agencies.xml", "Agency"},
        {"Cellphones.xml", "CellphoneData"},
        {"CountyJurisdictions.xml", "CountyJurisdiction"},
        {"CraftableItems.xml", "CraftableItem"},
        {"Dances.xml", "DanceData"},
        {"GangTerritories.xml", "GangTerritory"},
        {"Gestures.xml", "GestureData"},
        {"Locations.xml", "Locations"},
		{"Itoxicants.xml", "Intoxicant"},
		{"PedGroups.xml", "PedGroup"},
		{"PhysicalItems.xml", "PhysicalItem"},
		{"Speeches.xml", "SpeechData"},
		{"Streets.xml", "Street"},
		{"Weapons.xml", "WeaponInformation"},
		{"ZoneJurisdictions.xml", "ZoneJurisdiction"},
		{"Zones.xml", "Zone"}
    };
    
    vector<string> processedFiles;
    
    vector<pair<string, string>> specialFiles = {
        {"Contacts.xml", "Contacts"},
        {"DispatchablePeople.xml", "DispatchablePeople"},
        {"DispatchableVehicles.xml", "DispatchableVehicles"},
        {"Heads.xml", "Heads"},
        {"Interiors.xml", "Interiors"},
        {"IssuableWeapons.xml", "IssuableWeapons"},
        {"LocationTypes.xml", "LocationTypes"},
        {"ModItems.xml", "ModItems"},
        {"Names.xml", "Names"},
        {"Organizations.xml", "Organizations"},
		{"PlateTypes.xml", "PlateTypes"},
		{"ShopMenus.xml", "ShopMenus"},
		{"SpawnBlocks.xml", "SpawnBlocks"},
		{"VehicleRaceTypeManager.xml", "VehicleRaces"}
    };
    
    for (const auto& [baseFile, type] : specialFiles) {
        if (fs::exists(baseFile)) {
            for (const auto& entry : fs::directory_iterator(".")) {
                string filename = entry.path().filename().string();
                
                if (filename.find(type + "_") == 0 && filename.find('+') == string::npos) {
                    cout << "Processing " << filename << " against " << baseFile << endl;
                    
                    if (type == "Contacts") processContactsFile(baseFile, filename);
                    else if (type == "DispatchablePeople") processDispatchablePeopleFile(baseFile, filename);
                    else if (type == "DispatchableVehicles") processDispatchableVehiclesFile(baseFile, filename);
                    else if (type == "Heads") processHeadsFile(baseFile, filename);
                    else if (type == "Interiors") processInteriorsFile(baseFile, filename);
                    else if (type == "IssuableWeapons") processIssuableWeaponsFile(baseFile, filename);
                    else if (type == "LocationTypes") processLocationTypesFile(baseFile, filename);
                    else if (type == "ModItems") processModItemsFile(baseFile, filename);
                    else if (type == "Names") processNamesFile(baseFile, filename);
                    else if (type == "Organizations") processOrganizationsFile(baseFile, filename);
					else if (type == "PlateTypes") processPlateTypesFile(baseFile, filename);
					else if (type == "ShopMenus") processShopMenusFile(baseFile, filename);
					else if (type == "SpawnBlocks") processSpawnBlocksFile(baseFile, filename);  
					else if (type == "VehicleRaces") processVehicleRacesFile(baseFile, filename);  
                    
                    processedFiles.push_back(filename);
                }
            }
        } else {
            cerr << "Base file not found: " << baseFile << endl;
        }
    }
    
    for (const auto& [baseFile, sectionTag] : fileTypes) {
        if (!fs::exists(baseFile)) {
            cerr << "Base file not found: " << baseFile << endl;
            continue;
        }
        
        string prefix = baseFile.substr(0, baseFile.find('.'));
        
        for (const auto& entry : fs::directory_iterator(".")) {
            string filename = entry.path().filename().string();
            
            if (filename.find(prefix + "_") == 0 && filename.find('+') == string::npos) {
                cout << "Processing " << filename << " against " << baseFile << endl;
                
                if (sectionTag == "Locations") {
                    processLocationsFile(baseFile, filename);
                } else {
                    processFilePair(baseFile, filename, sectionTag);
                }
                
                processedFiles.push_back(filename);
            }
        }
    }
    
    cout << "\nConversion complete! Processed " << processedFiles.size() << " files:" << endl;
    for (const auto& file : processedFiles) {
        cout << "  " << file << endl;
    }
    
    return 0;
}