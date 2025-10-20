#include "common.h"
#include "core/Scene.h"
#include "core/Material.h"
#include "core/Sensor.h"
#include "core/Source.h"
#include "geometry/Box.h"
#include "geometry/Sphere.h"
#include "simulation/MonteCarloEngine.h"

// Exemple de configuration d'une salle de contrôle avec blindage
std::shared_ptr<Scene> createControlRoomScene() {
    auto scene = std::make_shared<Scene>();
    
    // Matériaux
    auto& materials = MaterialLibrary::getInstance();
    auto concrete = materials.getMaterial("Béton");
    auto lead = materials.getMaterial("Plomb");
    auto air = materials.getMaterial("Air");
    
    // === GÉOMÉTRIE ===
    
    // Salle principale (5m x 4m x 3m)
    auto room = Box::createWall("Room", 5.0f, 3.0f, 4.0f);
    room->setMaterial(concrete);
    room->setPosition(glm::vec3(0.0f, 1.5f, 0.0f));
    scene->addObject(room);
    
    // Mur de protection en plomb (épaisseur 5cm)
    auto leadWall = Box::createWall("LeadShield", 3.0f, 3.0f, 0.05f);
    leadWall->setMaterial(lead);
    leadWall->setPosition(glm::vec3(0.0f, 1.5f, -1.0f));
    scene->addObject(leadWall);
    
    // Porte (acier)
    auto door = std::make_shared<Box>("Door", glm::vec3(0.8f, 2.0f, 0.1f));
    door->setMaterial(materials.getMaterial("Acier"));
    door->setPosition(glm::vec3(-1.5f, 1.0f, 2.0f));
    scene->addObject(door);
    
    // === SOURCES DE RADIATION ===
    
    // Source gamma Cs-137 (662 keV) dans la zone de travail
    auto gammaSource = std::make_shared<IsotropicSource>("Cs-137", RadiationType::GAMMA);
    gammaSource->setPosition(glm::vec3(0.0f, 1.0f, -2.5f)); // Derrière le mur
    gammaSource->setIntensity(1e6); // 1 MBq
    
    EnergySpectrum spectrum;
    spectrum.type = EnergySpectrum::MONOENERGETIC;
    spectrum.energy = 662.0f; // keV
    gammaSource->setSpectrum(spectrum);
    
    scene->addSource(gammaSource);
    
    // Source neutron AmBe dans un coin
    auto neutronSource = std::make_shared<IsotropicSource>("AmBe", RadiationType::NEUTRON);
    neutronSource->setPosition(glm::vec3(2.0f, 1.0f, -2.0f));
    neutronSource->setIntensity(1e4); // 10 kBq
    
    EnergySpectrum neutronSpectrum;
    neutronSpectrum.type = EnergySpectrum::CONTINUOUS;
    // Spectre AmBe typique (approximatif)
    neutronSpectrum.spectrum = {
        {100.0f, 0.1f},   // 100 keV
        {1000.0f, 0.8f},  // 1 MeV
        {5000.0f, 1.0f},  // 5 MeV
        {10000.0f, 0.3f}  // 10 MeV
    };
    neutronSource->setSpectrum(neutronSpectrum);
    
    scene->addSource(neutronSource);
    
    // Fond cosmique
    auto cosmicBackground = std::make_shared<AmbientSource>("Cosmic", RadiationType::MUON);
    cosmicBackground->setBounds(glm::vec3(-10.0f, 5.0f, -10.0f), 
                               glm::vec3(10.0f, 10.0f, 10.0f));
    cosmicBackground->setIntensity(100.0f); // muons/s/m²
    
    EnergySpectrum cosmicSpectrum;
    cosmicSpectrum.type = EnergySpectrum::CONTINUOUS;
    cosmicSpectrum.spectrum = {
        {1000.0f, 0.2f},    // 1 MeV
        {10000.0f, 0.8f},   // 10 MeV
        {100000.0f, 1.0f},  // 100 MeV
        {1000000.0f, 0.5f}  // 1 GeV
    };
    cosmicBackground->setSpectrum(cosmicSpectrum);
    
    scene->addSource(cosmicBackground);
    
    // === CAPTEURS ===
    
    // Capteur au poste de travail
    auto workstationSensor = std::make_shared<Sensor>("Poste_Travail", 
                                                     SensorType::POINT, 
                                                     glm::vec3(0.0f, 1.0f, 1.5f));
    workstationSensor->setEnergyRange(50.0f, 3000.0f); // Gammas et X
    workstationSensor->setRadiationFilter({RadiationType::GAMMA, RadiationType::X_RAY});
    scene->addSensor(workstationSensor);
    
    // Capteur derrière le blindage
    auto shieldedSensor = std::make_shared<Sensor>("Zone_Blindee", 
                                                  SensorType::VOLUME, 
                                                  glm::vec3(0.0f, 1.0f, 0.5f));
    shieldedSensor->setSize(glm::vec3(0.5f, 0.5f, 0.5f)); // Volume 50cm³
    scene->addSensor(shieldedSensor);
    
    // Capteur neutrons près de la source AmBe
    auto neutronSensor = std::make_shared<Sensor>("Detecteur_Neutrons", 
                                                 SensorType::POINT, 
                                                 glm::vec3(1.5f, 1.0f, -1.5f));
    neutronSensor->setRadiationFilter({RadiationType::NEUTRON});
    neutronSensor->setEnergyRange(0.1f, 20000.0f); // Neutrons thermiques à rapides
    scene->addSensor(neutronSensor);
    
    // Capteur à l'entrée
    auto entranceSensor = std::make_shared<Sensor>("Entree", 
                                                  SensorType::SURFACE, 
                                                  glm::vec3(-1.5f, 1.0f, 2.5f));
    entranceSensor->setSize(glm::vec3(1.0f, 2.0f, 0.0f)); // Surface 2m²
    scene->addSensor(entranceSensor);
    
    // Capteur de fond (référence)
    auto backgroundSensor = std::make_shared<Sensor>("Fond_Exterieur", 
                                                    SensorType::POINT, 
                                                    glm::vec3(0.0f, 1.0f, 5.0f));
    scene->addSensor(backgroundSensor);
    
    // === CONFIGURATION AMBIANTE ===
    
    // Niveaux de radiation de fond
    scene->setBackgroundRadiation(RadiationType::GAMMA, 0.1f);   // 0.1 μSv/h
    scene->setBackgroundRadiation(RadiationType::NEUTRON, 0.01f); // Très faible
    scene->setBackgroundRadiation(RadiationType::MUON, 0.05f);   // Cosmiques
    
    return scene;
}

// Configuration de simulation optimisée pour cette scène
SimulationConfig getOptimizedConfig() {
    SimulationConfig config;
    
    config.maxParticles = 1000000;      // 1M particules
    config.maxBounces = 50;             // Limite de rebonds
    config.energyCutoff = 10.0f;        // 10 keV minimum
    config.timeCutoff = 1e8f;           // 100 ms max
    
    config.enableBackgroundSubtraction = true;
    config.enableVarianceReduction = true;
    
    config.useRussianRoulette = true;
    config.russianRouletteThreshold = 0.1f;
    
    config.useSplitting = false;        // Pas de splitting pour cette scène
    
    config.numThreads = std::min(8u, std::thread::hardware_concurrency());
    
    return config;
}

// Fonction d'analyse des résultats
void analyzeResults(std::shared_ptr<Scene> scene, const SimulationStats& stats) {
    std::cout << "\n=== RÉSULTATS DE SIMULATION ===" << std::endl;
    std::cout << "Particules émises: " << stats.particlesEmitted.load() << std::endl;
    std::cout << "Particules transportées: " << stats.particlesTransported.load() << std::endl;
    std::cout << "Temps de simulation: " << stats.getElapsedTime() << " s" << std::endl;
    std::cout << "Taux de simulation: " << stats.getParticleRate() << " part/s" << std::endl;
    
    std::cout << "\n--- Détections par capteur ---" << std::endl;
    
    auto sensors = scene->getAllSensors();
    for (auto& sensor : sensors) {
        const auto& sensorStats = sensor->getStats();
        
        std::cout << sensor->getName() << ":" << std::endl;
        std::cout << "  Total: " << sensorStats.totalCounts.load() << " counts" << std::endl;
        std::cout << "  Gamma: " << sensorStats.gammaCounts.load() << " counts" << std::endl;
        std::cout << "  Neutron: " << sensorStats.neutronCounts.load() << " counts" << std::endl;
        std::cout << "  Énergie: " << sensorStats.totalEnergy.load() << " keV" << std::endl;
        std::cout << "  Dose: " << sensor->getDoseRate() << " μSv/h" << std::endl;
        
        // Calcul du facteur d'atténuation si possible
        if (sensor->getName() == "Poste_Travail") {
            // Supposer que sans blindage, le débit serait 100x plus élevé
            double attenuationFactor = sensor->getAttenuationFactor(100.0 * sensor->getDoseRate());
            std::cout << "  Facteur d'atténuation estimé: " << attenuationFactor << std::endl;
        }
        
        std::cout << std::endl;
    }
}

// Exemple d'utilisation
int runSimulationExample() {
    try {
        // Initialisation
        MaterialLibrary::getInstance().loadDefaults();
        
        // Création de la scène
        auto scene = createControlRoomScene();
        scene->buildAccelerationStructure();
        
        std::cout << "Scène créée avec:" << std::endl;
        std::cout << "- " << scene->getObjectCount() << " objets" << std::endl;
        std::cout << "- " << scene->getSensorCount() << " capteurs" << std::endl;
        std::cout << "- " << scene->getSourceCount() << " sources" << std::endl;
        
        // Configuration du moteur
        MonteCarloEngine engine(scene);
        engine.setConfig(getOptimizedConfig());
        
        std::cout << "\nDémarrage de la simulation..." << std::endl;
        
        // Simulation
        auto startTime = std::chrono::steady_clock::now();
        engine.startSimulation();
        
        // Attendre la fin (ou utiliser des callbacks en mode interactif)
        while (engine.isRunning()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            const auto& stats = engine.getStats();
            float progress = engine.getProgress();
            
            std::cout << "\rProgrès: " << std::fixed << std::setprecision(1) 
                      << (progress * 100.0f) << "% (" 
                      << stats.particlesTransported.load() << " particules)" << std::flush;
        }
        
        std::cout << std::endl;
        
        // Analyse des résultats
        analyzeResults(scene, engine.getStats());
        
        // Sauvegarde du projet
        scene->saveToFile("control_room_simulation.radsim");
        std::cout << "Projet sauvegardé." << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Erreur: " << e.what() << std::endl;
        return -1;
    }
}